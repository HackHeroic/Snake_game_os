# Phase 2 — TODO

## Bug Fixes
- [x] **Terminal resize bug** — handled via `SIGWINCH` (`src/game/board.c`); main loop polls and re-fits the board.
- [x] **Food despawn render bug** — when timed food expired, refilled food was added to data but not redrawn; now `render_foods` runs after the expiry refill (`src/main.c`).

## Features
- [x] **AI ghost snake** — `src/game/ghost.c`, second snake moves randomly; collision kills the player.
- [x] **Speed powerup zones** — `src/game/powerups.c`, glowing `+/>` (boost) and `%/<` (slow) zones spawn periodically; effect lasts `POWERUP_EFFECT_TICKS`.
- [x] **Shrinking board** — `Board.inset` grows every 3 levels (`board_inset_for_level`); inner border drawn, snake/food refit on shrink.
- [x] **Multiple food spawning** — `foods_target_count(level)` keeps `max(3, 1+level)` foods up to 8.
- [x] **Replay/ghost mode** — `src/game/replay.c` records head trail per game, saves best run to `data/replay.dat`, replays as a dim `.` ghost on the next game.
