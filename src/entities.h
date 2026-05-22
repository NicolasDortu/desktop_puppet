#ifndef ENTITIES_H
#define ENTITIES_H

// =============================================================================
//  ENTITY ROLES
// =============================================================================
//
//  The project ships a single executable; main() dispatches into one of these
//  role entry points based on argv[1]:
//      (no arg) / "puppet"   -> RunPuppet
//      "menu"   <x> <y>      -> RunMenu
//      "item"   <x> <y>      -> RunItem
//
//  Each role owns its own raylib window and its own main loop. Children are
//  spawned by re-launching the same binary with the appropriate subcommand
//  (see ipc.c).
// =============================================================================

int RunPuppet(int argc, char **argv);
int RunMenu  (int argc, char **argv);
int RunItem  (int argc, char **argv);

#endif
