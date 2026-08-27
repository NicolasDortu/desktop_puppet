#include "r_coin.h"
#include "e_puppet.h" // limb sizing: the coin is half a hand
#include "renderer.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  COIN ROLE
// =============================================================================

// Coin diameter: half the size of a hand/foot (limb radius = puppet R * 0.25,
// so a 100px puppet has 50px limbs and 25px coins). Hardcoded to the default
// puppet size; the coin child has no access to the live puppet.
#define COIN_DIAMETER  26
#define COIN_RISE_PX   35.0f // how far the coin floats up
#define COIN_LIFE_SECS 1.0

// =============================================================================
//  PARENT SIDE
// =============================================================================

void SpawnCoinPopup(CoinPopups *ring, unsigned long parentPid, int x, int y)
{
    ChildProc *slot = &ring->procs[ring->next];
    ring->next = (ring->next + 1) % COIN_RING_SIZE;

    // Coins live ~1s; by the time the ring wraps this child is long gone.
    // IpcKillChild also closes the old handle, so nothing leaks.
    IpcKillChild(slot);

    char args[64];
    snprintf(args, sizeof args, "%d %d %lu", x, y, parentPid);
    IpcSpawnChild(slot, "coin", args); // fire and forget
}

void CloseCoinPopups(CoinPopups *ring)
{
    for (int i = 0; i < COIN_RING_SIZE; i++)
        IpcKillChild(&ring->procs[i]);
}

// =============================================================================
//  CHILD SIDE
// =============================================================================

int RunCoin(int argc, char **argv)
{
    // -- Parse argv: main.exe coin <x> <y> <parentPid> --
    int           posX      = (argc > 2) ? atoi(argv[2]) : 100;
    int           posY      = (argc > 3) ? atoi(argv[3]) : 100;
    unsigned long parentPid = (argc > 4) ? strtoul(argv[4], NULL, 10) : 0;

    SetConfigFlags(FLAG_WINDOW_UNFOCUSED);
    InitOverlayWindow(COIN_DIAMETER, COIN_DIAMETER);
    SetTargetFPS(TARGET_FPS);

    void *parentH = IpcOpenProcess(parentPid);

    Texture2D tex = LoadAssetTexture("coin.png");

    double t0 = GetTime();
    while (!WindowShouldClose() && IpcProcessAlive(parentH))
    {
        float progress = (float)((GetTime() - t0) / COIN_LIFE_SECS);
        if (progress >= 1.0f)
            break;

        // Float up, then fade out over the last third of the ride.
        float rise = COIN_RISE_PX * progress;
        float fade = (progress < 0.66f) ? 1.0f : (1.0f - progress) / 0.34f;
        SetWindowPosition(posX - COIN_DIAMETER / 2, (int)(posY - COIN_DIAMETER / 2 - rise));

        BeginDrawing();
            ClearBackground(BLANK);
            if (tex.id)
            {
                Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
                Rectangle dst = { 0, 0, COIN_DIAMETER, COIN_DIAMETER };
                DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f,
                               (Color){ 255, 255, 255, (unsigned char)(255 * fade) });
            }
            else // asset missing: plain gold disc
                DrawCircle(COIN_DIAMETER / 2, COIN_DIAMETER / 2, COIN_DIAMETER / 2.0f,
                           (Color){ 255, 200, 40, (unsigned char)(255 * fade) });
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
