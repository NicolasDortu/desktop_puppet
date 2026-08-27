#include "e_menu.h"
#include "renderer.h"

#include <stdio.h>

#include "raylib.h"

// =============================================================================
//  ITEM TABLE
// =============================================================================

// Single source of truth for the shop, indexed by ItemType.
const MenuItem MENU_ITEMS[ITEM_TYPE_COUNT] = {
    [ITEM_BALL]    = { .label = "BOWLING BALL",   .price = 10 },
    [ITEM_BAT]     = { .label = "BAT",            .price = 20 },
    [ITEM_BOMB]    = { .label = "BOMB",           .price = 30 },
    [ITEM_MISSILE] = { .label = "GUIDED MISSILE", .price = 35 },
};

static const Color MENU_BG_COLOR     = {  40,  40,  40, 230 };
static const Color MENU_HEADER_COLOR = {  25,  25,  25, 245 };
static const Color MENU_DISABLED     = { 130, 130, 130, 255 };

// =============================================================================
//  LAYOUT
// =============================================================================
//
// One column, one row per item.
// ponytail: bring back multi-column layout if the shop ever outgrows the screen.

void MenuWindowSize(int *width, int *height)
{
    *width  = MENU_WIDTH;
    *height = MENU_HEADER + MENU_ITEM_HEIGHT * ITEM_TYPE_COUNT;
}

// Map a window-local cursor position to the ItemType under it, or -1.
int MenuPick(Vector2 mouseLocal)
{
    if (mouseLocal.y < MENU_HEADER) // rows start below the header
        return -1;

    int row = (int)((mouseLocal.y - MENU_HEADER) / MENU_ITEM_HEIGHT);
    if (row >= ITEM_TYPE_COUNT || mouseLocal.x < 0 || mouseLocal.x >= MENU_WIDTH)
        return -1;
    return row;
}

// =============================================================================
//  RENDERING
// =============================================================================

// Small coin glyph (texture if the asset is there, gold disc otherwise).
static void DrawCoinIcon(Texture2D tex, int x, int y, int size)
{
    if (tex.id)
        DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                       (Rectangle){ (float)x, (float)y, (float)size, (float)size },
                       (Vector2){ 0, 0 }, 0.0f, WHITE);
    else
        DrawCircle(x + size / 2, y + size / 2, size / 2.0f, GOLD);
}

// The shop: header with title + balance, one row per item with its price.
// Rows the balance can't cover are grayed out (display only; the parent
// re-checks the price before spawning).
void DrawMenu(int coins)
{
    // Coin icon, loaded on first draw (LoadTexture needs the window open).
    static Texture2D texCoin;
    static bool texLoaded = false;
    if (!texLoaded)
    {
        texCoin   = LoadAssetTexture("coin.png");
        texLoaded = true;
    }

    int width, height;
    MenuWindowSize(&width, &height);

    ClearBackground(MENU_BG_COLOR);

    // -- Header: title left, balance (coin + count) right --
    DrawRectangle(0, 0, width, MENU_HEADER, MENU_HEADER_COLOR);
    DrawText("SHOP", MENU_PADDING, (MENU_HEADER - 20) / 2, 20, GOLD);

    char balance[8];
    snprintf(balance, sizeof balance, "%d", coins);
    int bw = MeasureText(balance, MENU_FONT_SIZE);
    int bx = width - MENU_PADDING - bw;
    DrawText(balance, bx, (MENU_HEADER - MENU_FONT_SIZE) / 2, MENU_FONT_SIZE, RAYWHITE);
    DrawCoinIcon(texCoin, bx - 16 - 4, (MENU_HEADER - 16) / 2, 16);

    // -- Item rows --
    int hovered = MenuPick(GetMousePosition());

    for (int i = 0; i < ITEM_TYPE_COUNT; i++)
    {
        int  y          = MENU_HEADER + i * MENU_ITEM_HEIGHT;
        int  textY      = y + (MENU_ITEM_HEIGHT - MENU_FONT_SIZE) / 2;
        bool affordable = coins >= MENU_ITEMS[i].price;

        if (i == hovered && affordable)
            DrawRectangle(0, y, MENU_WIDTH, MENU_ITEM_HEIGHT, (Color){ 255, 255, 255, 30 });

        DrawText(MENU_ITEMS[i].label, MENU_PADDING, textY, MENU_FONT_SIZE,
                 affordable ? RAYWHITE : MENU_DISABLED);

        // Price + coin glyph, right-aligned.
        char price[8];
        snprintf(price, sizeof price, "%d", MENU_ITEMS[i].price);
        int pw = MeasureText(price, MENU_FONT_SIZE);
        int px = MENU_WIDTH - MENU_PADDING - 14 - 4 - pw;
        DrawText(price, px, textY, MENU_FONT_SIZE, affordable ? GOLD : MENU_DISABLED);
        DrawCoinIcon(texCoin, px + pw + 4, y + (MENU_ITEM_HEIGHT - 14) / 2, 14);
    }

    // -- Gold frame around the whole shop --
    DrawRectangleLinesEx((Rectangle){ 0, 0, (float)width, (float)height }, 2.0f, GOLD);
}
