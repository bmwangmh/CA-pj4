# Project 4 — Snake (Longan Nano / GD32VF103)

PlatformIO project built on the Lab 11/12 `lab-longan-nano-starter` template.
C handles the game; **RISC-V assembly (`src/scoreboard.S`) handles the
persistent top-3 scoreboard**, as required by spec 1.1.

## Build & flash

```bash
cd snake
pio run                 # cross-compile
pio run -t upload       # DFU flash: hold BOOT0, tap RESET, then run this
```

> This dev container has no `pio` and no RISC-V embedded toolchain, so the
> firmware must be built/flashed on a machine with PlatformIO + the `gd32v`
> platform. The assembly was verified with `riscv64-linux-gnu-gcc -c`, and
> `game.c` passes `-Wall -Wextra` cleanly.

## Controls

| Input        | Action                                                       |
|--------------|--------------------------------------------------------------|
| Joystick ↑↓  | Menu: move selection / In-game: steer                        |
| Joystick ←→  | In-game: steer                                               |
| Joystick CTR | Menu: enter level / Death: open scoreboard                   |
| SW1          | In-game: force-quit to menu / Scoreboard & death: to menu    |
| SW2          | In-game: hold for 2× speed                                   |

## Layout

160×80 screen → top 16px status bar (`1-X  SCORE:NNN`), playfield is a
20×8 grid of 8×8 cells starting at y=16.

## Source map

| File            | Responsibility                                                  |
|-----------------|----------------------------------------------------------------|
| `main.c`        | IO init + state machine: menu → play → death                   |
| `game.h/.c`     | timing, RNG, input, rendering, snake/levels/AI, all 3 levels   |
| `scoreboard.S`  | **assembly**: record / sort / display persistent top-3         |
| `scoreboard.h`  | C-visible declarations of the assembly routines                |
| `lcd/`,`systick`,`utils`,`fatfs` | unchanged template drivers                    |

## Requirement → code mapping

**1.1 Scoreboard (RISC-V, 3 pts)** — `scoreboard.S`
- `scoreboard_update(level, score)`: insertion into a 3-slot array kept
  sorted descending by score, then by level — all in assembly. Storage in
  `.data` ⇒ persists across games, wiped by hard reset.
- `scoreboard_show()`: draws the ranking (calling only LCD/Get_Button leaf
  helpers) and blocks until SW1.
- `death_screen()` calls `scoreboard_update` on **every** death (even when
  the player goes straight back to the menu); CTR opens `scoreboard_show`.

**1.2 UI & controls (2 pts)** — `menu_loop`, `play_level` (SW1 abort), `main`.

**1.3 Snake logic (1 pt)** — steering, constant speed, SW2 2×, wall/self death.

**1.4 Items (1 pt)** — `spawn_item`/`update_items`: random empty cell, gold
lives 10 s then respawns.

**1.5 Score counter (1 pt)** — `draw_status`, redrawn on score change.

**2.1 Level 1-2 (3 pts)** — 4 static walls, a connected portal pair (exit at
the partner's facing direction), 50% gems (5 s, +2). Spawns avoid walls/portals.

**2.2 Level 1-3 (3 pts)** — enemy of fixed length 3 at the symmetric point;
chases the nearest gold, cruises otherwise, turns (right-first) before a wall;
player-head-into-enemy = death; enemy-head-into-player = enemy dies + drops 3
gold.

**3 Experience (4 pts)** — chose two:
- **3.1 flicker-free**: only changed cells are repainted (`snake_step`,
  per-item draw/erase, walls/portals drawn once); no full clears mid-game.
- **3.2 fixed FPS**: each frame is held to `FRAME_MS` via a busy-wait on
  `millis()`, so frame start intervals are constant regardless of logic cost.
