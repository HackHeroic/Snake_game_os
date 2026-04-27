# CHEAT SHEET — Snake Game OS

## 30-Second Pitch
Snake game in C demonstrating OS concepts. Custom memory allocator (64KB heap, first-fit), custom math (Russian Peasant multiply, binary long division), custom string ops, raw terminal I/O via termios, signal handling, ANSI 256-color rendering. No external libraries. 1600 lines, 62 unit tests, 14 modules in 3 layers.

---

## Architecture (draw this on board if asked)

```
main.c  ──>  game/   ──>  lib/
             ui/    ──>  lib/
```
- **lib/** = OS primitives (memory, math, string, keyboard, screen)
- **game/** = pure logic (snake, food, board, score, obstacles, stats)
- **ui/** = rendering (renderer, screens, themes)
- Dependencies flow downward only. No circular deps.

---

## 6 OS Concepts — One Line Each

| Concept | Code | Key Detail |
|---------|------|------------|
| **Heap Allocator** | memory.c | 64KB pool, 8B headers, first-fit, forward coalescing |
| **Terminal Control** | keyboard.c | termios raw mode: ~ECHO ~ICANON ~ISIG, VMIN=0 VTIME=0 |
| **Signal Handling** | keyboard.c:12 | SIGINT handler restores terminal, uses _exit() (async-safe) |
| **Non-blocking I/O** | keyboard.c:29 | VMIN=0 VTIME=0 = read() returns immediately |
| **System Calls** | board.c:9 | ioctl(TIOCGWINSZ) for terminal size, read() for input |
| **Low-level Math** | math.c | Russian Peasant = shift+add, Division = shift+subtract, PRNG = Schrage's method |

---

## Memory Allocator — Key Numbers

- Pool: **64KB** (single malloc at startup)
- Header: **8 bytes** (4 size + 1 in_use + 3 padding)
- Min alloc: **8 bytes** (prevents tiny fragments)
- Min split: **16 bytes** (header + min alloc)
- Strategy: **first-fit** (O(n) scan)
- Coalescing: **forward only** (limitation: no backward)

---

## Key Algorithms

**Multiply** (Russian Peasant): while b>0: if b is odd, result+=a. Double a, halve b. O(log n).

**Divide** (Binary Long Division): scan bits MSB to LSB, shift remainder left, bring down bit, subtract divisor if fits. O(32).

**PRNG** (MINSTD + Schrage): seed = 16807*seed mod (2^31-1). Schrage splits into hi/lo to avoid 32-bit overflow.

---

## Game Loop — 11 Steps

```
Input -> Direction -> Compute next pos -> Wall check -> Self check -> Obstacle check
-> Food check -> Execute move (alloc head, dealloc tail) -> Post-move (score, level, spawn)
-> Food despawn countdown -> Render (delta) + Delay (usleep)
```

---

## Snake Data Structure

- **Singly-linked list**: head -> seg -> seg -> ... -> tail
- Each segment: {x, y, *next} — allocated via our custom alloc()
- Move: alloc new head (O(1)), dealloc tail (O(n) — walk to 2nd-to-last)
- Self-collision skips tail (it vacates same tick)
- Direction rejection: |new - current| == 2 means opposite, reject it

---

## Features Quick List

- 2 modes: Classic (wall=death) / Wrap (modulo edges)
- 3 food types: Normal * (65%, 1pt), Bonus $ (20%, 3pt, 30-tick timer), Slow ~ (15%, +50ms delay)
- Obstacles: 1-2 per level-up, max 10, safe placement (Manhattan dist > 2 from head)
- 4 themes: Classic/Ice/Lava/Rainbow (256-color ANSI, body gradient)
- Leveling: level = food_eaten / 5, speed = max(80, 200 - level*15) ms
- Persistent high score + lifetime stats (file I/O)
- Delta rendering: only changed elements redrawn per tick

---

## Limitations (have these ready — shows depth)

1. **Forward coalescing only** — real allocators use boundary tags for both directions
2. **O(n) tail removal** — doubly-linked list would fix it
3. **No thread safety** — no mutex on allocator
4. **No free list** — always scans from start, could maintain explicit free block list
5. **Food spawn can loop** — no retry limit (obstacles have 100-retry cap)
6. **Single process** — could split into input/logic/render threads

---

## If They Ask "Why Not Just Use...?"

| "Why not malloc?" | We're demonstrating HOW malloc works internally |
|---|---|
| "Why not ncurses?" | We're demonstrating HOW terminal control works at POSIX level |
| "Why not math.h?" | We're showing the ALU-level algorithms (shift+add, shift+subtract) |
| "Why not arrays?" | Linked list demonstrates dynamic allocation from our heap |
| "Why not best-fit?" | More fragmentation, same O(n). First-fit is the practical choice |

---

## Terminal Mode — The 3 Flags

```
~ECHO   = don't print what user types (we render the game ourselves)
~ICANON = don't buffer until Enter (character-at-a-time for instant response)
~ISIG   = don't let Ctrl+C kill us (we handle cleanup in our signal handler)
```

## Arrow Keys = 3-Byte Escape Sequences

```
UP: \033[A    DOWN: \033[B    RIGHT: \033[C    LEFT: \033[D
```
read_key() reads byte 1 (\033), then 2 more. Byte 3 identifies the arrow.

---

## Numbers to Remember

- **64KB** virtual RAM pool
- **8 bytes** per block header
- **62** unit tests across 3 test files
- **14** source modules in **3** layers
- **~1600** lines of code
- **256-color** ANSI rendering
- **4** themes, **3** food types, **2** game modes
- **80ms** minimum frame delay (fastest speed)
- **200ms** starting frame delay (slowest speed)

---

## PHASE 2 — Cheat Sheet

### What's New
| Feature | File | One-liner |
|---|---|---|
| Resize fix | `board.c` | SIGWINCH handler sets a flag; main loop refits + redraws |
| AI ghost snake | `ghost.c` | Random direction (16 retries), kills on overlap |
| Multi-food | `food.c` | Array of up to 8, target = clamp(1+level, 3, 8) |
| Powerup zones | `powerups.c` | Boost/slow zones, lifetime 120 ticks, effect 25 ticks |
| Shrinking board | `board.c` | `inset` field grows every 3 levels, inner border drawn |
| Replay ghost | `replay.c` | Saves best run head trail to `data/replay.dat`, redraws as dim `.` |

### SIGWINCH Pattern (Why a flag, not direct work)
```c
static volatile sig_atomic_t g_winch_pending;
static void winch_handler(int sig) { g_winch_pending = 1; }   /* only safe op */
/* main loop: if (terminal_consume_winch()) apply_terminal_resize(...); */
```
- `volatile` — value can change asynchronously, don't cache in register
- `sig_atomic_t` — atomic read/write w.r.t. signal interrupts
- Real work (alloc, printf, ioctl) happens in main loop, not handler

### Ghost Snake — 16-Retry Random Walk
```c
for (int attempt = 0; attempt < 16; attempt++) {
    pick random direction
    if (would hit wall or self) continue;
    snake_move(...); return 1;
}
return 0;   /* trapped this tick — stays still */
```

### Food — Fixed-Size Array Tricks
```c
#define MAX_FOOD_PIECES 8
target = max(3, 1+level), capped at 8
foods_remove_at: pieces[i] = pieces[count-1]; count--;   /* O(1) swap-with-last */
```
**Bug we caught:** timed-food expiry refilled data but didn't `render_foods()`. Fix = call render once after the expiry pass.

### Powerup Speed Effect
```c
speed = score_get_speed(score);                          /* base */
if (boost_ticks > 0) speed = max(40, speed - 70);        /* faster */
if (slow_ticks  > 0) speed += 50;                        /* slower */
usleep(speed * 1000);
```
Boost & slow share *no* counter — picking one zeros the other so latest wins.

### Shrinking Board — Inset Math
```c
playable area: [inset, w-1-inset] x [inset, h-1-inset]
inset = level / 3, capped so play area >= 6 wide x 4 tall
```
Wall check, wrap, food spawn, ghost spawn, render border — *all* respect inset.

On level-up: `snakes_refit_inset` translates snake's bbox to recenter inside new area; food outside new area is cleared + respawned.

### Replay File Layout
```
[ magic 'RPLY' (4B) | score (4B) | board_w (4B) | board_h (4B) | count (4B) | frames... ]
each frame = 2 shorts (x, y) = 4 bytes
max 6000 frames -> ~24 KB file
```
- Magic number rejects corrupt/wrong files
- `short` not `int` — board never exceeds 32,767
- Save *only if* current score > saved score
- Replay ghost is **cosmetic only** (no death) — it's a learning aid

### Phase 2 Likely-To-Be-Asked Q's

| Question | Short answer |
|---|---|
| Why a flag in SIGWINCH? | Signal handlers can't safely call non-async-signal-safe code (printf, alloc). |
| Why `volatile sig_atomic_t`? | Asynchronous changes + atomic w.r.t. signals. |
| Why 16 retries for ghost? | Lets it wiggle out of corners; if all fail, stand still. |
| Why array for food, list for snake? | Food count is small + bounded; snake grows unboundedly and needs O(1) head insert. |
| Why magic number in replay file? | Reject corrupt/foreign files instead of crashing on garbage. |
| Why shrink every 3 levels? | Coarser steps: shrinking every level felt too aggressive. |
| Why doesn't replay kill you? | It's a learning aid showing your previous best — must be followable. |
