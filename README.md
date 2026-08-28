# Desktop Buddy/Puppet

A desktop toy in the spirit of *Interactive Buddy*: a ragdoll puppet lives on
your desktop in a transparent, always-on-top window. Drag it around, throw it
into walls, and spend the coins it earns (by getting hurt) on items to torment
it with — a bowling ball, a baseball bat, a bomb, a guided missile.

Windows-only, written in C with [raylib](https://www.raylib.com/).

## Building & running

Requires MinGW-w64 `gcc`, `make`, and `windres` on PATH. raylib headers live in
`include/`, the static library in `lib/`.

```
make
bin\main.exe
```

## Distributing (itch.io)

```
make dist
```

Builds a release binary (optimized, `-mwindows` so no console window pops up,
symbols stripped) and zips it to `dist/DesktopPuppet.zip`: `Desktop Buddy.exe`
and `assets/` sitting flat next to each other, ready to upload as a Windows
download. No installer, no DLLs to bundle — raylib is statically linked, and
the only DLL dependencies are ones every Windows install already has
(GDI32, KERNEL32, msvcrt, SHELL32, USER32, WINMM).

`LoadAssetTexture`/`LoadAssetSound` (`renderer.c`) resolve `assets/` next to
the *running exe itself* (`GetApplicationDirectory()`), not the working
directory — so the exe can be renamed or moved anywhere as long as `assets/`
stays beside it. The dev build keeps that true too: every `make` mirrors the
tracked `assets/` into `bin/assets/` (`sync-assets`), so `bin\main.exe` finds
its assets the same way a shipped build does.

`itch.toml` at the repo root just names the play executable for itch's
desktop app; a manual browser download/unzip doesn't need it at all.

## How to play

| Action | Effect |
|---|---|
| Left-drag the puppet (or an item) | Grab it; release while moving to throw |
| Right-click the puppet | Open/close the shop |
| Hard wall crash or solid item hit | Puppet is "hurt": X eyes, +1 coin |
| Shop footer ("SOUND: ON/OFF") | Mute/unmute all sound effects |

Items despawn after 60 s. The bomb detonates after a 3 s fuse. The missile
chases your **cursor** and detonates on the first thing it touches — screen
border or puppet — so where you point is where it strikes.

## Architecture: one exe, many processes

Everything is a single binary that plays different roles depending on its
first argument (dispatch in `main.c`):

```
main.exe                                  -> puppet (parent, owns everything)
main.exe menu <x> <y> <pid>               -> shop popup
main.exe item <type> <x> <y> <pid> <slot> -> one item (ball/bat/bomb/missile)
main.exe coin <x> <y> <pid>               -> floating coin popup (~1 s)
```

Each role gets its **own transparent, undecorated, topmost window** that is
resized/moved every frame to hug its content (`UpdateWindow`). That is the
trick that lets the puppet and items roam the whole desktop without one giant
invisible window blocking clicks everywhere.

### Shared memory instead of messages

The puppet process creates a named shared-memory region
(`Local\desktop_puppet_<pid>`); children derive the same name from the parent
PID passed via argv and map it. There is **no message protocol** — the
`SharedState` struct in `r_sync.h` *is* the synced state:

- the puppet publishes its 6 limbs every frame,
- each item child publishes its particles into its slot,
- the menu child publishes the clicked row and the mute flag,
- an exploding bomb/missile publishes a blast (fields first, `seq` bump last).

Torn reads are harmless because every field is rewritten the next frame.
Collisions across processes work by each side resolving against a **local
copy** of the other side's snapshot and keeping only its own share of the
correction; the other side applies its share in its own process.

Children poll a process handle on the parent and exit when it dies. The parent
reaps children that closed their windows.

### File map

| File | Role |
|---|---|
| `main.c` | argv dispatch into the four roles |
| `physics.c/.h` | Verlet particles, bone constraints, collision resolvers, blast, bounds |
| `e_puppet.c/.h` | puppet entity: limbs, bones, pose guard, drawing (eyes incl.) |
| `e_item.c/.h` | item entity: spec table, construction, drawing (bomb, missile, …) |
| `e_menu.c/.h` | shop entity: item table, layout, picking, drawing |
| `r_puppet.c` | parent main loop: input, physics, coins, sounds |
| `r_item.c` | item parent API (spawn/collide/reap) + item child main loop |
| `r_menu.c` | menu parent API (open/close/actions) + menu child main loop |
| `r_coin.c` | coin popup spawn (ring of fire-and-forget children) + child loop |
| `r_sync.c/.h` | `SharedState` layout + region create/attach helpers |
| `ipc.h`, `ipc_win32.c` | Win32 isolation: shared memory, process spawn/liveness, cursor (`<windows.h>` never leaks into other files — it clashes with raylib names) |
| `input.c/.h` | generic grab/drag/throw for any body |
| `renderer.c/.h` | overlay window setup, work-area query, asset loading |
| `config.h` | physics tuning, hurt/coin thresholds, FPS |

## Physics: Verlet particles + distance constraints

Every physical thing is a `Body`: particles integrated with **Verlet** (velocity
is implicit — `v = pos - oldPos`), wired together by bones (distance
constraints). Per frame: integrate with friction + gravity, bounce off the
work-area edges (taskbar excluded), then relax the bones 3 iterations.

- The puppet is 6 particles (body, head, 2 hands, 2 feet). **Hard** bones fix
  the silhouette; **soft** head-to-limb diagonals are the springs that make
  limbs swing and snap back.
- Distance constraints can't tell left from right, so `EnforcePuppetPose`
  mirrors a limb back to its own side of the body→head axis — only when the
  limb is calm relative to the body, and it keeps the limb moving *with* the
  body afterward (mirroring its absolute velocity used to kill throws mid-air).
- Items are tiny bodies: ball/bomb/missile are 1 particle, the bat is 2
  particles + 1 rigid bone (a capsule).
- **Impact**: positional overlap resolution alone barely moves the puppet, so
  a *freely moving* item also injects its velocity into the limb it hits,
  scaled by the item's `punch`. Dragged items transfer nothing — rolling the
  ball at the puppet is the rewarded move, shoving it by hand is not.
- **Blast**: one uniform kick along blast-center → body-center, same vector for
  every particle (per-particle radial kicks cancelled through the bones and the
  puppet barely moved). Fades linearly to the blast radius.
- The missile skips all of this: powered flight steering toward the cursor,
  capped at cruise speed, until it detonates.

## Sounds

Loaded from `assets/` (mp3). Each effect plays in the process that owns the
triggering event; the shop's mute flag lives in shared memory and every
audio process applies it via `SetMasterVolume` each frame.

| Sound | When | Process |
|---|---|---|
| `s_bonk` | bat hits a limb at speed | puppet |
| `s_bowling` | ball hits at speed, only when not dragged | puppet |
| `s_cash` | coin earned | puppet |
| `s_ouch` | puppet hurt (~1 in 3, it was grating every time) | puppet |
| `s_fuze` | bomb spawns | bomb child |
| `s_missile` | loops while the missile flies | missile child |
| `s_explosion` | bomb/missile detonates (child lingers invisibly until it finishes) | item child |

Missing assets are safe: shapes fall back to flat circles/lines, sounds to
silence.

## Tuning knobs

| What | Where |
|---|---|
| Gravity, friction, bounce, bone stiffness | `config.h` (`PHYS_*`) |
| Hurt thresholds, coin cooldown | `config.h` (`HURT_*`) |
| Item sizes, hit strength (`punch`), colors | `ITEM_SPECS` in `e_item.c` |
| Shop labels and prices | `MENU_ITEMS` in `e_menu.c` |
| Bomb fuse/blast, missile speed/agility/fuel | `e_item.h` |
| Puppet size | `radius` in `r_puppet.c` |
