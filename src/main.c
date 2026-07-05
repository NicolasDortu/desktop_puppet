#include "r_puppet.h"
#include "r_item.h"
#include "r_menu.h"
#include "r_coin.h"

#include <string.h>

// =============================================================================
//  ROLE ATTRIBUTION
// =============================================================================
//
//  The main binary plays several roles depending on its first argument.
//  Children spawn each other by re-launching the same exe with a subcommand:
//
//      main.exe                           -> puppet (the main window)
//      main.exe menu|item|coin  <x> <y>   -> popup at screen (x, y)
//
// =============================================================================

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "menu") == 0) return RunMenu(argc, argv);
        if (strcmp(argv[1], "item") == 0) return RunItem(argc, argv);
        if (strcmp(argv[1], "coin") == 0) return RunCoin(argc, argv);
    }
    return RunPuppet(argc, argv);
}