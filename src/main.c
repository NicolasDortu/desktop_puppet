#include "e_puppet.h"
#include "e_menu.h"
#include "e_item.h"

#include <string.h>

// =============================================================================
//  ROLE ATTRIBUTION
// =============================================================================
//
//  The main binary plays three different roles depending on its first
//  argument; children spawn each other by re-launching the same exe with the
//  appropriate subcommand:
//
//      main.exe                 -> puppet (the main window)
//      main.exe menu  <x> <y>   -> menu popup at screen (x, y)
//      main.exe item  <x> <y>   -> draggable ball at screen (x, y)
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