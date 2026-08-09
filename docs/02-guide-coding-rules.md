<h1 align="center">Coding Rules & System Architecture</h1>

This document outlines the coding rules, naming conventions, and architectural principles applied in **Lone-Blade**.

---

## I. Architecture Rules

1. **Non-Blocking Architecture:**
   - Functions executed inside signal handlers or periodic game loop updates must **never block** or invoke long sleep routines (`delay()`).
   - All timing must be driven by Active Kernel timers (`timer_set()`).

2. **Static Memory Discipline:**
   - Zero dynamic memory allocation (`malloc`, `free`, `new`, `delete`) inside the game loop to avoid heap fragmentation in the 16 KB RAM limit.
   - All entity pools (`MAX_MONSTERS = 5`, `MAX_BOSS_FIREBALLS = 3`) use fixed-size static arrays.

3. **Event-Driven Task Separation:**
   - Display task (`AC_TASK_DISPLAY_ID`) handles screen rendering, button events, and game state transitions.
   - Separate game modules (`game_player`, `game_monster`, `game_boss`, `game_item`, `game_combat`) encapsulate entity state data.

---

## II. Naming Conventions

| Entity Category | Convention | Example |
|---|---|---|
| Module Files | Lowercase with underscores (`.cpp` / `.h`) | `game_boss.cpp`, `scr_game_win.cpp` |
| Functions | `module_action()` | `player_attack()`, `game_boss_spawn()` |
| Signals | `AC_DISPLAY_*` or `GAME_*` | `AC_DISPLAY_GAME_TICK`, `AC_DISPLAY_GAME_WIN` |
| Constants / Macros | Uppercase with underscores | `PLAYER_MAX_HP`, `GAME_GROUND_Y` |
| Struct Types | PascalCase or `_t` suffix | `GamePlayer`, `SaveData` |

---

## III. Defensive Programming Rules

- **Division-by-Zero Protection:** All percentage or health bar calculations check for zero denominators (e.g. `boss.max_hp == 0`).
- **Array Bounds Clamping:** Drawing coordinates and HUD health hearts are clamped strictly within display bounds (`128x64`).
- **Flash Protection:** Flash write operations (`flash_write`) occur only when high score data changes, preventing unnecessary wear on chip sectors.
