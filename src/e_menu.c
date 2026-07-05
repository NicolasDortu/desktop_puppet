#include "e_menu.h"
#include "renderer.h"

#include <stdio.h>

#include "raylib.h"

// =============================================================================
//  ITEM TABLE
// =============================================================================

// Single source of truth for the shop.
const MenuItem MENU_ITEMS[MENU_ITEM_COUNT] = {
    {.id = MENU_ITEM_BALL, .action = "BOWLING BALL", .price = 10 },
    {.id = MENU_ITEM_BAT,  .action = "BAT",          .price = 20 },
    {.id = MENU_ITEM_BOMB, .action = "BOMB",         .price = 40 },
};

static const Color MENU_BG_COLOR     = {  40,  40,  40, 230 };
static const Color MENU_HEADER_COLOR = {  25,  25,  25, 245 };
static const Color MENU_DISABLED     = { 130, 130, 130, 255 };

// =============================================================================
//  LAYOUT
// =============================================================================
//
// Items are laid out top-to-bottom, then in a new column.
// columns = ceil(count / MENU_MAX_ROWS)
// rows    = min(count, MENU_MAX_ROWS)

static int MenuColumns(void) { return (MENU_ITEM_COUNT + MENU_MAX_ROWS - 1) / MENU_MAX_ROWS; }
static int MenuRows   (void) { return (MENU_ITEM_COUNT < MENU_MAX_ROWS) ? MENU_ITEM_COUNT : MENU_MAX_ROWS; }
static int MenuItemCol(int i) { return i / MENU_MAX_ROWS; }
static int MenuItemRow(int i) { return i % MENU_MAX_ROWS; }

void MenuWindowSize(int *width, int *height)
{
    *width  = MENU_WIDTH * MenuColumns();
    *height = MENU_HEADER + MENU_ITEM_HEIGHT * MenuRows();
}

// Map a window-local cursor position to the menu item id under it, or -1.
int MenuPick(Vector2 mouseLocal)
{
    float rowY = mouseLocal.y - MENU_HEADER; // rows start below the header
    if (rowY < 0)
        return -1;

    int col = (int)(mouseLocal.x / MENU_WIDTH);
    int row = (int)(rowY / MENU_ITEM_HEIGHT);
    int i   = col * MENU_MAX_ROWS + row;

    if (col >= 0 && col < MenuColumns() &&
        row >= 0 && row < MENU_MAX_ROWS &&
        i   >= 0 && i   < MENU_ITEM_COUNT)
        return MENU_ITEMS[i].id;
    return -1;
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

    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        int  x          = MenuItemCol(i) * MENU_WIDTH;
        int  y          = MENU_HEADER + MenuItemRow(i) * MENU_ITEM_HEIGHT;
        int  textY      = y + (MENU_ITEM_HEIGHT - MENU_FONT_SIZE) / 2;
        bool affordable = coins >= MENU_ITEMS[i].price;

        if (MENU_ITEMS[i].id == hovered && affordable)
            DrawRectangle(x, y, MENU_WIDTH, MENU_ITEM_HEIGHT, (Color){ 255, 255, 255, 30 });

        DrawText(MENU_ITEMS[i].action, x + MENU_PADDING, textY, MENU_FONT_SIZE,
                 affordable ? RAYWHITE : MENU_DISABLED);

        // Price + coin glyph, right-aligned.
        char price[8];
        snprintf(price, sizeof price, "%d", MENU_ITEMS[i].price);
        int pw = MeasureText(price, MENU_FONT_SIZE);
        int px = x + MENU_WIDTH - MENU_PADDING - 14 - 4 - pw;
        DrawText(price, px, textY, MENU_FONT_SIZE, affordable ? GOLD : MENU_DISABLED);
        DrawCoinIcon(texCoin, px + pw + 4, y + (MENU_ITEM_HEIGHT - 14) / 2, 14);
    }

    // -- Gold frame around the whole shop --
    DrawRectangleLinesEx((Rectangle){ 0, 0, (float)width, (float)height }, 2.0f, GOLD);
}
