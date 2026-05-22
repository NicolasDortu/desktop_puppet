#include "entities.h"

#include <string.h>

// =============================================================================
//  DISPATCHER
// =============================================================================
//
//  A single binary plays three different roles depending on its first
//  argument; children spawn each other by re-launching the same exe with the
//  appropriate subcommand:
//
//      main.exe                 -> puppet (the main window)
//      main.exe puppet          -> puppet
//      main.exe menu  <x> <y>   -> menu popup at screen (x, y)
//      main.exe item  <x> <y>   -> draggable ball at screen (x, y)
//
//  Sharing one binary lets every role pull from the same input / renderer /
//  IPC code and gives every window the same name on the taskbar.
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