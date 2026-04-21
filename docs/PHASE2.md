# Snake Game OS — Phase 2 Roadmap

This document tracks post–presentation fixes and planned Phase 2 work.

---

## Bug fix (presentation)

### Terminal resize (`SIGWINCH`)

**Problem:** Zooming the terminal (e.g. Cmd+ / Cmd−) changes the visible size after startup. The game only reads terminal dimensions once at launch, which leads to broken rendering: duplicated snake segments, elements drawn out of bounds, and inconsistent state.

**Goal:** Handle terminal resize events and keep the board consistent with the current size.

- Install a handler for **`SIGWINCH`** (window size change).
- On resize, re-read terminal dimensions (e.g. `ioctl` `TIOCGWINSZ` or equivalent already used at startup).
- **Re-layout / re-render** the game: clip or reposition the playfield, snake, food, and UI so nothing assumes the old row/column count.
- Optionally **pause** or show a short “resizing…” state if a full reconcile is expensive, then resume with a valid grid.

---

## Phase 2 — Features

| Feature | Summary |
|--------|---------|
| **AI ghost snake** | **Done:** Random-move second snake (`g`/`o`); **any overlap with the player ends the game.** |
| **Shrinking board** | **Walls advance inward** by one row/column on a timer or per level, reducing playable area and increasing difficulty. |
| **Multiple food** | **Done:** target `max(3, 1 + level)` (max 8) — **3+ foods from game start**; each eat triggers refill to target. |
| **Replay / ghost mode** | **Record player inputs** each run. On the **next** game, show a **ghost snake** replaying the **best recorded run** (or similar “best run” policy), alongside live play. |

---

## Suggested implementation order (non-binding)

1. **SIGWINCH** — unblocks correct behavior when the terminal changes size.
2. **Multiple food** — extends existing food/spawn logic; good foundation for higher difficulty.
3. **Shrinking board** — changes bounds and collision; pairs well with multi-food and level scaling.
4. **AI ghost snake** — second mover + collision rules.
5. **Replay / ghost** — input recording, storage, and synchronized playback (largest cross-cutting effort).

---

## Notes

- Phase 2 items should respect existing project constraints (custom libs, no banned standard APIs in game logic unless the codebase policy is explicitly updated).
- Replay storage format and “best run” definition (score vs. length vs. level) should be decided before heavy implementation.
