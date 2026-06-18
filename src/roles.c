#include "roles.h"
#include "e_puppet.h"
#include "e_item.h"
#include "e_menu.h"
#include "sync.h"
#include "ipc.h"
#include "input.h"
#include "physics.h"
#include "renderer.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  PROCESS ROLES
// =============================================================================
//
//  Each role runs in its own transparent, undecorated, topmost window:
//
//      main.exe                                  -> puppet (owns the shm region)
//      main.exe menu <x> <y> <parentPid>         -> menu popup
//      main.exe item <type> <x> <y> <pid> <slot> -> draggable ball
//
//  Parent and children share one memory region (SharedState, see sync.h): the
//  puppet publishes its limbs, each child publishes its own particle/choice.
//  There is no message protocol -- the struct IS the synced state. This file
//  owns all window-loop, child-process and shared-memory wiring so the e_*
//  entity files stay pure.
// =============================================================================

// =============================================================================
//  PARENT-SIDE BOOKKEEPING
// =============================================================================

// One live item child: its process handle and kind. The item's physical state
// lives in SharedState.items[i]; this only tracks the child process.
typedef struct
{
    ChildProc proc;
    ItemType  type;
    bool      active;
} ItemSlotMeta;

typedef struct
{
    ItemSlotMeta items[MAX_ITEMS];
} ItemRegistry;

typedef struct
{
    bool      isOpen;
    ChildProc proc;
} Menu;

// =============================================================================
//  ITEM HOST  (parent side)
// =============================================================================

// Spawn a new item child of the requested kind. Returns false if all slots
// are busy or spawning failed.
static bool SpawnItem(ItemRegistry *reg, SharedState *shared, unsigned long parentPid,
                      ItemType type, int posX, int posY)
{
    for (int slot = 0; slot < MAX_ITEMS; slot++)
    {
        ItemSlotMeta *meta = &reg->items[slot];
        if (meta->active)
            continue;

        // Clear any stale state left by a previous child in this slot before the
        // new child can publish (we read `active` to know it's valid).
        shared->items[slot] = (ItemSlot){0};

        // Command line: main.exe item <type> <x> <y> <parentPid> <slot>
        char args[96];
        snprintf(args, sizeof args, "%d %d %d %lu %d",
                 (int)type, posX, posY, parentPid, slot);

        if (!IpcSpawnChild(&meta->proc, "item", args))
            return false;

        meta->type   = type;
        meta->active = true;
        return true;
    }
    return false;
}

// Push every limb away from `item` if they overlap. We collide a LOCAL copy of
// the item (owned by the child via shared memory) so only the limb side of the
// resolution is kept; the item's own correction is discarded and will be redone
// authoritatively by the child next frame.
static void CollideItemAgainstPuppet(Particle item, Puppet *pup)
{
    for (int j = 0; j < LIMB_COUNT; j++)
        ResolveCirclesCollisions(&item, &pup->limbs[j]);
}

// Publish the puppet limbs, collide each live item against them, reap children
// that have exited.
static void UpdateItems(ItemRegistry *reg, SharedState *shared, Puppet *pup)
{
    // -- Publish the puppet limbs for every child to read --
    shared->limbCount = LIMB_COUNT;
    for (int i = 0; i < LIMB_COUNT; i++)
        shared->limbs[i] = pup->limbs[i];

    bool puppetTouched = false;

    for (int slot = 0; slot < MAX_ITEMS; slot++)
    {
        ItemSlotMeta *meta = &reg->items[slot];
        if (!meta->active)
            continue;

        // Reap a child that has exited (closed its window).
        if (!IpcChildRunning(&meta->proc))
        {
            IpcKillChild(&meta->proc);
            meta->active               = false;
            shared->items[slot].active = false;
            continue;
        }

        // Collide the puppet against the item's latest published state.
        if (shared->items[slot].active)
        {
            CollideItemAgainstPuppet(shared->items[slot].particle, pup);
            puppetTouched = true;
        }
    }

    // Limb displacements above invalidate the cached bounds used for window sizing.
    if (puppetTouched)
        pup->body.bounds = ComputeBoundBox(&pup->body);
}

// Terminate every live item child (called on shutdown).
static void CloseAllItems(ItemRegistry *reg)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        if (reg->items[i].active)
            IpcKillChild(&reg->items[i].proc);
    }
}

// =============================================================================
//  MENU HOST  (parent side)
// =============================================================================

// Spawn the menu child window at the given screen position.
static void OpenMenu(Menu *menu, SharedState *shared, unsigned long parentPid,
                     int screenX, int screenY)
{
    if (menu->isOpen)
        return;

    // Clear any stale result before the child can publish a new one.
    shared->menu = (MenuSlot){0};

    char args[64];
    snprintf(args, sizeof args, "%d %d %lu", screenX, screenY, parentPid);
    if (IpcSpawnChild(&menu->proc, "menu", args))
        menu->isOpen = true;
}

// Terminate the child window (if any) and reset state.
static void CloseMenu(Menu *menu)
{
    if (!menu->isOpen)
        return;
    IpcKillChild(&menu->proc);
    menu->isOpen = false;
}

// Poll the shared MenuSlot for a click result and dispatch the chosen action.
static void MenuActions(Puppet *pup, Menu *menu, ItemRegistry *reg,
                        SharedState *shared, unsigned long parentPid)
{
    if (!menu->isOpen)
        return;

    // The menu child writes its choice into the shared slot then exits. Read it
    // (if any) and mirror the child's exit into our isOpen flag.
    bool gotClick = shared->menu.done;
    int  id       = gotClick ? shared->menu.chosenId : -1;

    if (!IpcChildRunning(&menu->proc))
        menu->isOpen = false;

    if (!gotClick)
        return;

    switch (id)
    {
    case MENU_ITEM_RED:
        pup->color = RED;
        break;
    case MENU_ITEM_GREEN:
        pup->color = GREEN;
        break;
    case MENU_ITEM_BLUE:
        pup->color = BLUE;
        break;
    case MENU_ITEM_ITEM:
    {
        // Spawn the ball just to the right of the puppet's bounding box.
        int x = (int)(pup->body.bounds.x + pup->body.bounds.w + 50);
        int y = (int)(pup->body.bounds.y);
        SpawnItem(reg, shared, parentPid, ITEM_BALL, x, y);
        break;
    }
    // case ...
    default:
        break;
    }
}

// Right-click the puppet to open (or close) the menu in its own window.
static void ToggleMenu(Puppet *pup, Menu *menu, SharedState *shared, unsigned long parentPid)
{
    if (!IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        return;

    if (menu->isOpen)
    {
        CloseMenu(menu);
        return;
    }

    // Location of the menu, next to the puppet.
    int x = (int)(pup->body.bounds.x + pup->body.bounds.w + MENU_PADDING);
    int y = (int)pup->body.bounds.y;
    OpenMenu(menu, shared, parentPid, x, y);
}

// =============================================================================
//  CHILD HELPERS
// =============================================================================

// Map the puppet's shared region (named after its PID) and grab a handle used
// to detect when the parent dies. Returns false if the region can't be mapped.
static bool ChildAttach(unsigned long parentPid, ShmRegion *shm,
                        SharedState **shared, void **parentH)
{
    char name[64];
    IpcShmName(parentPid, name, sizeof name);
    if (!IpcShmOpen(shm, name, sizeof(SharedState)))
        return false;
    *shared  = (SharedState *)shm->view;
    *parentH = IpcOpenProcess(parentPid);
    return true;
}

// =============================================================================
//  ITEM ROLE  (child)
// =============================================================================

int RunItem(int argc, char **argv)
{
    // -- Parse argv: main.exe item <type> <x> <y> <parentPid> <slot> --
    ItemType      type      = (argc > 2) ? (ItemType)atoi(argv[2]) : ITEM_BALL;
    if (type < 0 || type >= ITEM_TYPE_COUNT) type = ITEM_BALL;
    float         startX    = (argc > 3) ? (float)atoi(argv[3]) : 300.0f;
    float         startY    = (argc > 4) ? (float)atoi(argv[4]) : 300.0f;
    unsigned long parentPid = (argc > 5) ? strtoul(argv[5], NULL, 10) : 0;
    int           slot      = (argc > 6) ? atoi(argv[6]) : 0;

    int   winSize = ItemWindowSize(type);
    float half    = winSize / 2.0f;

    // -- Setup --
    InitOverlayWindow(winSize, winSize);
    SetTargetFPS(TARGET_FPS);

    ShmRegion    shm;
    SharedState *shared;
    void        *parentH;
    if (!ChildAttach(parentPid, &shm, &shared, &parentH))
    {
        CloseWindow();
        return 1;
    }

    ScreenWidthHeight screen = GetScreenSize();

    Particle particle;
    Body     body;
    CreateItem(&particle, &body, type, (Vector2){ startX, startY });

    // -- Main loop --
    while (!WindowShouldClose() && IpcProcessAlive(parentH))
    {
        // Input
        DragBody(&body);

        // Simulation: collide against a local copy of each limb (we must not
        // write into shared->limbs, which the puppet owns).
        ApplyPhysics(&body, screen.screenWidth, screen.screenHeight);
        for (int i = 0; i < shared->limbCount; i++)
        {
            Particle limb = shared->limbs[i];
            ResolveCirclesCollisions(&particle, &limb);
        }

        // Publish our state for the puppet to collide against.
        shared->items[slot].particle = particle;
        shared->items[slot].active   = true;

        // Move the OS window so the particle stays centered in it.
        SetWindowPosition((int)(particle.pos.x - half),
                          (int)(particle.pos.y - half));

        // Render
        BeginDrawing();
            DrawItem(type);
        EndDrawing();
    }

    // -- Teardown --
    shared->items[slot].active = false;
    IpcShmClose(&shm);
    CloseWindow();
    return 0;
}

// =============================================================================
//  MENU ROLE  (child)
// =============================================================================

// Frames to wait before reacting to focus loss, so the window does not close
// itself before the OS has finished promoting it to foreground.
#define MENU_FOCUS_GRACE_FRAMES 10

int RunMenu(int argc, char **argv)
{
    int           posX      = (argc > 2) ? atoi(argv[2]) : 100;
    int           posY      = (argc > 3) ? atoi(argv[3]) : 100;
    unsigned long parentPid = (argc > 4) ? strtoul(argv[4], NULL, 10) : 0;

    int width, height;
    MenuWindowSize(&width, &height);

    // -- Setup --
    InitOverlayWindow(width, height);
    SetWindowPosition(posX, posY);
    SetTargetFPS(TARGET_FPS);

    ShmRegion    shm;
    SharedState *shared;
    void        *parentH;
    if (!ChildAttach(parentPid, &shm, &shared, &parentH))
    {
        CloseWindow();
        return 1;
    }

    int chosen = -1;
    int frame  = 0;

    while (!WindowShouldClose() && IpcProcessAlive(parentH) && chosen == -1)
    {
        // -- Input: click selects an item --
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            chosen = MenuPick(GetMousePosition());

        // -- Auto-close when the user clicks outside the menu window --
        if (frame > MENU_FOCUS_GRACE_FRAMES && !IsWindowFocused())
            break;

        // -- Render --
        BeginDrawing();
            DrawMenu();
        EndDrawing();

        frame++;
    }

    // Publish the choice (if any) into the shared slot before exiting.
    if (chosen != -1)
    {
        shared->menu.chosenId = chosen;
        shared->menu.done     = true;
    }

    IpcShmClose(&shm);
    CloseWindow();
    return (chosen != -1) ? 0 : 1; // 1 = closed without a selection
}

// =============================================================================
//  PUPPET ROLE  (parent)
// =============================================================================

int RunPuppet(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // -- Setup --
    float radius = 100.0f;
    InitOverlayWindow((int)(2 * radius), (int)(2 * radius));

    ScreenWidthHeight win = GetScreenSize();

    // Shared memory: this process owns the region; children map it by our PID.
    unsigned long selfPid = IpcSelfPid();
    char          shmName[64];
    IpcShmName(selfPid, shmName, sizeof shmName);
    ShmRegion shm;
    if (!IpcShmCreate(&shm, shmName, sizeof(SharedState)))
    {
        CloseWindow();
        return 1;
    }
    SharedState *shared = (SharedState *)shm.view;

    Vector2      startPos = {win.screenWidth / 2.0f, win.screenHeight / 2.0f};
    Puppet       pup;
    CreatePuppet(&pup, "buddy", YELLOW, radius, startPos);
    Menu         menu  = {0};
    ItemRegistry items = {0};

    SetTargetFPS(TARGET_FPS);

    // -- Main loop --
    while (!WindowShouldClose())
    {
        // Input
        DragBody(&pup.body);
        ToggleMenu(&pup, &menu, shared, selfPid);
        MenuActions(&pup, &menu, &items, shared, selfPid);

        // Simulation
        ApplyPhysics(&pup.body, win.screenWidth, win.screenHeight);
        UpdateItems(&items, shared, &pup);
        UpdateWindow(pup.body.bounds);

        // Render
        BeginDrawing();
            DrawPuppet(&pup);
        EndDrawing();
    }

    // -- Teardown --
    CloseAllItems(&items);
    CloseMenu(&menu);
    IpcShmClose(&shm);
    CloseWindow();
    return 0;
}
