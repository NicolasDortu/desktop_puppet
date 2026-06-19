#include "r_puppet.h"
#include "r_item.h"
#include "r_menu.h"

#include <string.h>

// =============================================================================
//  ROLE ATTRIBUTION
// =============================================================================
//
//  The main binary plays three different roles depending on its first argument.
//  Children spawn each other by re-launching the same exe with a subcommand:
//
//      main.exe                      -> puppet (the main window)
//      main.exe menu|item  <x> <y>   -> menu|item popup at screen (x, y)
//
// =============================================================================

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "menu") == 0) return RunMenu(argc, argv);
        if (strcmp(argv[1], "item") == 0) return RunItem(argc, argv);
    }
    return RunPuppet(argc, argv);
}