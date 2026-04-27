# Snake Game OS - Viva Preparation Guide

## Table of Contents
1. [Project Elevator Pitch](#1-project-elevator-pitch)
2. [High-Level Explanation](#2-high-level-explanation)
3. [Low-Level Walkthrough](#3-low-level-walkthrough)
4. [Features Implemented](#4-features-implemented)
5. [OS Concepts Mapping](#5-os-concepts-mapping)
6. [Design Justifications (Why Questions)](#6-design-justifications)
7. [Deep Dives (How Questions)](#7-deep-dives)
8. [Limitations and Improvements](#8-limitations-and-improvements)
9. [Rapid-Fire Q&A](#9-rapid-fire-qa)
10. [Phase 2 Features (Deep Dive + Snippets)](#10-phase-2-features-deep-dive--snippets)

---

## 1. Project Elevator Pitch

> We built a fully functional Snake game in C that demonstrates core Operating System concepts. Instead of relying on standard libraries, we implemented our own **memory allocator** (64KB heap with first-fit allocation), our own **math library** (using hardware-level algorithms like Russian Peasant multiplication), our own **string library**, and our own **terminal I/O system** using raw mode and ANSI escape sequences. The game runs entirely in the terminal with 256-color rendering, multiple game modes, obstacles, special food types, color themes, and persistent statistics — all managed through our custom OS-level primitives.

---

## 2. High-Level Explanation

### How to explain the project in 2 minutes (demo opening):

**"What did we build?"**

A terminal-based Snake game written in pure C with zero external dependencies. The key constraint we imposed on ourselves: **don't use standard library functions for core operations**. We wrote our own malloc/free, our own strlen/strcpy, our own multiply/divide. This forced us to understand how these OS and system-level primitives actually work under the hood.

**"How is it structured?"**

The project has three clean layers:

```
                    +---------------------------+
                    |        main.c             |
                    |     (Game Loop /          |
                    |      Orchestrator)        |
                    +------+------+------+------+
                           |      |      |
                +----------+  +---+---+  +----------+
                |             |       |              |
          +-----v-----+ +----v----+ +v-----------+  |
          |   game/    | |   ui/   | |   lib/     |  |
          |            | |         | |            |  |
          | snake.c    | |renderer | |memory.c    |  |
          | food.c     | |screens  | |math.c      |  |
          | board.c    | |themes   | |string.c    |  |
          | score.c    | |         | |screen.c    |  |
          | obstacles.c| |         | |keyboard.c  |  |
          | stats.c    | |         | |            |  |
          +------------+ +---------+ +------------+  |
                                                      |
          game/ depends on lib/ (uses alloc, math)    |
          ui/ depends on lib/ (uses screen) and game/ |
          main.c orchestrates everything              |
```

- **lib/** — OS-level primitives. Knows nothing about the game. Could be reused in any project.
- **game/** — Pure game logic. No rendering, no terminal calls. Just data structures and rules.
- **ui/** — Rendering and display. Translates game state into terminal output.
- **main.c** — The game loop that ties everything together.

**"What OS concepts does it demonstrate?"**

Memory management (custom heap allocator), terminal device control (raw mode via termios), signal handling (SIGINT), non-blocking I/O, system calls (read, write, ioctl), file persistence, and low-level arithmetic (bit-shift algorithms used in real ALUs).

---

### How to explain the project in 5 minutes (detailed walkthrough):

Start with the 2-minute version above, then add:

**"Let me walk you through what happens when you run the game."**

1. **Initialization** — `memory_init()` grabs a single 64KB block from the OS via malloc. This is our "virtual RAM." From this point on, all game allocations go through our custom `alloc()/dealloc()`. Then `keyboard_init()` puts the terminal into raw mode — disabling echo, disabling line buffering, making reads non-blocking.

2. **Title Screen** — The player picks Classic or Wrap-Around mode. While they browse, we're seeding our PRNG — the longer they wait, the more different the random seed, so every game plays differently.

3. **Theme Selection** — Player picks from 4 color themes (Classic, Ice, Lava, Rainbow). Each theme is a `const struct` with color codes for head, body gradient, food, obstacles, and borders.

4. **Game Object Creation** — Board dimensions are computed from the terminal size (via `ioctl(TIOCGWINSZ)` system call). The snake is created as a **singly-linked list** of 3 segments, allocated from our custom heap. Food is spawned at a random position that avoids the snake and obstacles.

5. **The Game Loop** (runs ~5-12 times per second depending on level):
   - Read keyboard input (non-blocking — returns immediately if no key pressed)
   - Compute where the head would move next
   - Check for death: wall collision, self-collision, obstacle collision
   - Check if food is eaten
   - Execute the move: alloc a new head segment, dealloc the tail (unless growing)
   - If leveled up: spawn 1-2 obstacles at safe positions
   - Render only what changed (delta rendering)
   - Sleep for the frame delay

6. **Cleanup** — On game over or quit: free all game objects through our custom dealloc, restore the terminal to its original state, save stats to disk.

**"The key insight is that every operation you take for granted — allocating memory, reading a keypress, printing a colored character, multiplying two numbers — we implemented from scratch using OS-level primitives."**

---

## 3. Low-Level Walkthrough

### Memory Lifecycle (trace a single snake segment through the allocator):

```
1. snake_move() calls alloc(sizeof(SnakeSegment))     [snake.c:53]
                              |
                              v
2. alloc(12) enters our custom allocator               [memory.c:24]
   - size=12 >= MIN_ALLOC(8), so no bump
   - Start scanning from virtual_ram[0]
   - Find first free block with size >= 12
                              |
                              v
3. Block found: [HDR: size=65528, in_use=0]
   - Leftover = 65528 - 12 = 65516 >= MIN_SPLIT(16)
   - SPLIT: create new header at offset 20 (8+12)
     [HDR: size=12, in_use=1] [HDR: size=65508, in_use=0]
   - Return pointer to byte 8 (past header)
                              |
                              v
4. Snake segment lives at this address, stores {x, y, *next}
                              |
                              v
5. When tail is removed, dealloc(tail_ptr) is called   [snake.c:67]
   - Compute header: ptr - 8
   - Set in_use = 0
   - Forward coalesce: if next block is also free, merge them
     [HDR: size=12, in_use=0] + [HDR: size=N, in_use=0]
     becomes [HDR: size=12+8+N, in_use=0]
```

### Input Handling (trace a keypress from hardware to game logic):

```
1. User presses UP arrow key
                              |
                              v
2. Terminal sends 3 bytes: \033 (ESC), [, A
                              |
                              v
3. read_key() in keyboard.c:
   - read(STDIN, &c, 1) gets \033
   - Recognizes escape sequence prefix
   - read() two more bytes: '[' and 'A'
   - seq[0]=='[' and seq[1]=='A' => return KEY_UP (1000)
                              |
                              v
4. main.c:132-133:
   - key == KEY_UP => snake_set_direction(snake, DIR_UP)
                              |
                              v
5. snake_set_direction() in snake.c:88-92:
   - Current direction: DIR_RIGHT (1)
   - New direction: DIR_UP (0)
   - |0 - 1| = 1, which != 2 (not opposite)
   - Direction change ACCEPTED
   (If snake was going DOWN(2), |0-2|=2, REJECTED — can't reverse)
                              |
                              v
6. Next tick: snake_compute_next_head() uses dy[DIR_UP] = -1
   - new_y = head->y + (-1) = head->y - 1
   - Snake moves up
```

### Rendering (trace how a colored character appears on screen):

```
1. render_snake() iterates the linked list               [renderer.c:40]
                              |
                              v
2. For the head (dist==0):
   - screen_set_color(theme->head_color)                  [screen.c:26]
   - Outputs: \033[38;5;46m  (ANSI 256-color: foreground green)
   - screen_put_char(OFFSET_X + x, OFFSET_Y + y, '@')    [screen.c:13]
   - Outputs: \033[row;colH  (cursor positioning) then '@'
                              |
                              v
3. For body segments (dist > 0):
   - Rainbow theme: color = rainbow_colors[dist % 6]
   - Other themes: color = body_colors[min(dist-1, 4)]
   - This creates a gradient effect — body fades from head color
                              |
                              v
4. screen_flush() calls fflush(stdout)                    [screen.c:42]
   - All buffered escape sequences + characters sent to terminal at once
   - Terminal interprets ANSI codes and renders colored characters
```

---

## 4. Features Implemented

### Core Game Features

| Feature | How It Works | Key Code |
|---------|-------------|----------|
| **Snake Movement** | Singly-linked list. Each tick: alloc new head, dealloc tail. Direction stored as enum (UP=0, RIGHT=1, DOWN=2, LEFT=3) with delta arrays dx[], dy[] | `snake.c:5-6, 50-73` |
| **Collision Detection** | Wall: bounds check. Self: iterate all segments except tail. Obstacles: linear scan of array | `board.c:28-33`, `snake.c:75-86`, `obstacles.c:73-80` |
| **Food System** | Random spawn avoiding snake + obstacles. 3 types: Normal(65%), Bonus(20%), Slow(15%). Timed food expires after 30 ticks | `food.c:7-69` |
| **Scoring & Leveling** | Level = food_eaten / 5. Speed = max(80, 200 - level*15) ms. Bonus food gives 3x points but doesn't accelerate leveling | `score.c:16-33` |
| **Two Game Modes** | Classic: die on wall. Wrap-Around: modulo arithmetic wraps position to opposite edge | `main.c:145-154`, `board.c:35-38` |

### Advanced Features

| Feature | How It Works | Key Code |
|---------|-------------|----------|
| **Obstacles** | 1-2 spawned per level-up. Max 10. Safe placement: avoids snake, food, and Manhattan distance <= 2 from head. 100-retry limit prevents hangs | `obstacles.c:43-71` |
| **4 Color Themes** | Static const theme structs with 256-color ANSI codes. Classic (green), Ice (blue), Lava (orange), Rainbow (cycling 6 colors) | `themes.c:5-14` |
| **Body Gradient** | Each theme has 5 body colors. Segment index maps to color: `body_colors[min(dist-1, 4)]`. Rainbow theme cycles through 6 colors using modulo | `renderer.c:50-54` |
| **Slow Food Effect** | Eating '~' food adds 50ms to frame delay for 30 ticks. Separate from base speed — stacks on top of level speed | `main.c:184-186, 238-241` |
| **Persistent High Score** | Written to `data/highscore.dat` via fprintf. Loaded on game start via fscanf. Updated if current score exceeds saved high score | `score.c:35-53` |
| **Lifetime Statistics** | Games played, total food eaten, longest snake, best level. Saved to `data/stats.dat`. Survives across game sessions | `stats.c:1-48` |
| **Pause/Resume** | Toggle with P key. Paused state skips game logic, shows overlay. On unpause, full screen redraw (clear + re-render everything) | `main.c:110-125` |
| **Terminal Size Detection** | `ioctl(TIOCGWINSZ)` system call. Board adapts to terminal size. Minimum 22x14 enforced with error message if too small | `board.c:7-17`, `main.c:66-73` |
| **Victory Condition** | If snake fills the entire board (`length >= area - 1`), `food_spawn()` returns NULL, triggering victory screen | `food.c:17-19`, `main.c:202-209` |
| **Delta Rendering** | Only changed elements redrawn per tick. Border drawn once. Obstacles drawn once on spawn. HUD updated only on score change. Tail erased by overwriting with space | `main.c:228-233`, `renderer.c:117-120` |

---

## 5. OS Concepts Mapping

### Primary OS Concepts

**1. Heap Memory Management (memory.c)**
- **What we built:** A 64KB virtual RAM pool with first-fit block allocation
- **OS equivalent:** This is exactly what `malloc()/free()` do internally. The OS gives each process a heap segment, and the C runtime manages allocations within it
- **Key details:**
  - Block header = 8 bytes (size + in_use flag + padding)
  - First-fit scan: walk blocks linearly, take the first one that fits
  - Block splitting: if leftover >= 16 bytes, create a new free block
  - Forward coalescing on free: merge consecutive free blocks to reduce fragmentation
- **Textbook topics:** Heap management, first-fit/best-fit/worst-fit, internal and external fragmentation, coalescing, memory allocation algorithms

**2. Terminal Device Control (keyboard.c, screen.c)**
- **What we built:** Raw terminal mode for direct character-by-character input, ANSI escape sequences for cursor control and color
- **OS equivalent:** This is device driver interaction. The terminal is a character device; `termios` is the POSIX interface for configuring its behavior
- **Key details:**
  - `termios` struct controls: echo, canonical mode, signal generation, read behavior
  - ANSI escape sequences are a device control protocol — we send `\033[row;colH` to position the cursor, `\033[38;5;Nm` for 256-color
- **Textbook topics:** Character devices, device drivers, device control, I/O modes (canonical vs raw)

**3. Signal Handling (keyboard.c:12-19)**
- **What we built:** Custom SIGINT handler that restores the terminal before exiting
- **OS equivalent:** The kernel delivers signals to processes. Without our handler, Ctrl+C would kill the process and leave the terminal in raw mode (broken)
- **Key details:**
  - `signal(SIGINT, sigint_handler)` registers our handler
  - Handler calls `keyboard_restore()` (reset termios), `screen_show_cursor()`, then `_exit(0)`
  - Uses `_exit()` not `exit()` because `_exit()` is async-signal-safe
- **Textbook topics:** Signals, interrupt handling, signal handlers, async-signal-safety

**4. Non-Blocking I/O (keyboard.c:28-29)**
- **What we built:** `VMIN=0, VTIME=0` makes `read()` return immediately with 0 bytes if no input
- **OS equivalent:** This is polling I/O — the game loop checks for input each tick without blocking. Alternative approaches would be select()/poll() or separate input thread
- **Textbook topics:** Blocking vs non-blocking I/O, polling, I/O multiplexing

**5. System Calls**
- **What we use:**
  - `read()` — read from stdin (keyboard input)
  - `write()` (via printf/putchar) — write to stdout (screen output)
  - `ioctl(TIOCGWINSZ)` — query terminal dimensions from the kernel
  - `usleep()` — suspend process for microseconds (frame timing)
  - `tcgetattr()/tcsetattr()` — get/set terminal attributes
  - `fopen()/fprintf()/fscanf()` — file I/O for persistence
- **Textbook topics:** System call interface, user mode vs kernel mode transitions

**6. Pseudo-Random Number Generation (math.c:75-86)**
- **What we built:** MINSTD Linear Congruential Generator with Schrage's method
- **OS equivalent:** The kernel provides `/dev/urandom` for randomness. Our PRNG is deterministic — given the same seed, it produces the same sequence. Real OS PRNGs mix in hardware entropy
- **Textbook topics:** Deterministic randomness, entropy sources, PRNG algorithms

**7. Low-Level Arithmetic (math.c:3-55)**
- **What we built:** Multiplication via Russian Peasant (shift-and-add), division via binary long division (shift-and-subtract)
- **OS equivalent:** These are the exact algorithms implemented in hardware ALUs. The CPU's multiplier circuit does shift-and-add in parallel; we do it sequentially in software
- **Textbook topics:** ALU operations, binary arithmetic, bit manipulation

**8. File System I/O (score.c, stats.c)**
- **What we built:** Read/write game data to plain text files using fopen/fprintf/fscanf
- **OS equivalent:** These C library calls map to open()/read()/write() system calls. The kernel handles the actual disk I/O through the file system driver
- **Textbook topics:** File descriptors, file system interface, persistent storage

---

## 6. Design Justifications

### Q: Why a custom memory allocator instead of just using malloc/free?
**A:** The whole point is to demonstrate how an OS manages heap memory. Our `memory.c` simulates what malloc does internally — we grab one big 64KB chunk at startup (like the OS giving a process its heap), then manage allocations within that using a first-fit algorithm with block headers. Every `alloc()` and `dealloc()` in the game goes through our allocator.

### Q: Why first-fit? Why not best-fit or worst-fit?
**A:** First-fit is O(n) in the number of blocks and has good average-case behavior — it finds the first block large enough and uses it. Best-fit would also be O(n) but causes more external fragmentation (tiny unusable leftover blocks). Worst-fit wastes the most memory. First-fit is a practical balance of simplicity and performance, and many real allocators use it as a starting point.

### Q: Why a linked list for the snake? Why not an array?
**A:** The snake grows and shrinks from both ends every tick. With a linked list, prepending a new head is O(1). An array would need element shifting or a circular buffer. The linked list also demonstrates dynamic memory — every segment is individually allocated and freed through our custom allocator, showing heap management in action.

### Q: Why implement your own string and math functions?
**A:** To demonstrate that standard library functions are algorithms, not magic. `my_multiply` uses the same shift-and-add approach as hardware ALUs. `my_divide` uses binary long division. The string functions show manual pointer manipulation and null-terminated string conventions. This demonstrates understanding of what the OS and hardware do below the abstraction layer.

### Q: Why separate lib/, game/, and ui/ layers?
**A:** Separation of concerns — a fundamental OS and software design principle. `lib/` provides system-level utilities with no game knowledge. `game/` contains pure logic with no rendering. `ui/` handles display. You could swap the renderer without touching game logic. This also enables unit testing — we test `lib/` functions without needing a terminal.

### Q: Why raw mode instead of ncurses?
**A:** Using ncurses would hide the OS-level concepts we want to demonstrate. By configuring termios directly, we show how terminal device control works at the POSIX level — disabling echo, canonical mode, and signal generation, and handling escape sequences ourselves. This is the actual interface between user space and the terminal device driver.

### Q: Why MINSTD for random numbers?
**A:** MINSTD is one of the simplest well-tested PRNGs. It has good statistical properties for a game (no visible patterns in food placement). We use Schrage's method to prevent 32-bit overflow without needing 64-bit types, which is an elegant mathematical trick worth demonstrating.

### Q: Why base leveling on food_eaten instead of score?
**A:** If leveling was based on score, bonus food (3 points) would make the game speed up 3x faster — punishing the player for getting bonus food. By counting food_eaten, every food item advances difficulty equally regardless of point value. This is a fairness design decision.

---

## 7. Deep Dives

### Deep Dive: Memory Allocator

**Q: Walk me through exactly what happens when alloc(size) is called.**

(`memory.c:24-47`)
1. If `size < MIN_ALLOC (8)`, bump it up to 8 to prevent tiny fragments
2. Start pointer `ptr` at beginning of virtual RAM
3. Walk blocks linearly. Each block has an 8-byte header: `[size:4B | in_use:1B | padding:3B]`
4. Find first free block where `block->size >= size`:
   - **Splitting:** If leftover space >= 16 bytes (MIN_SPLIT), split the block. Create a new free block header after our allocation
   - Mark `in_use = 1`
   - Return `ptr + HEADER_SIZE` (user gets pointer past the header)
5. If end reached without finding a block: return NULL (out of memory)

**Q: How does dealloc and coalescing work?**

(`memory.c:49-67`)
1. Compute block header: `(char*)user_ptr - HEADER_SIZE`
2. Set `in_use = 0`
3. **Forward coalescing:** Check the next block in memory. If it's free, absorb it: `block->size += HEADER_SIZE + next->size`. Repeat for all consecutive free blocks.

**Q: What is external fragmentation and how does your allocator handle it?**

External fragmentation occurs when free memory exists but is scattered in small non-contiguous blocks. Our allocator handles it with:
- **Coalescing:** Merging adjacent free blocks reduces fragmentation
- **Minimum allocation size (8 bytes):** Prevents extremely small unusable blocks
- **Minimum split threshold (16 bytes):** Only split a block if the remainder is actually useful

### Deep Dive: Terminal I/O

**Q: Explain exactly how raw mode works.**

(`keyboard.c:21-35`)
```
Normal terminal (canonical mode):
  User types: H-e-l-l-o-Enter -> program receives "Hello\n" all at once
  Characters are echoed on screen as you type

Raw mode (our configuration):
  User types: H -> program immediately receives 'H'
  Nothing appears on screen (echo disabled)
  Ctrl+C doesn't kill the program (ISIG disabled)
  read() returns immediately even if nothing was pressed (VMIN=0, VTIME=0)
```

The `termios` struct has flags we disable:
- `ECHO` — don't display typed characters (we render our own game)
- `ICANON` — don't wait for Enter (character-at-a-time delivery)
- `ISIG` — don't generate SIGINT on Ctrl+C (we handle it ourselves via signal handler)

And control characters we set:
- `VMIN = 0` — minimum bytes for read() to return: 0 (don't block)
- `VTIME = 0` — timeout for read(): 0 (return immediately)

**Q: How do arrow keys work? They're not single characters.**

(`keyboard.c:51-73`)
Arrow keys send 3-byte escape sequences:
- UP:    `\033[A` (bytes: 27, 91, 65)
- DOWN:  `\033[B` (bytes: 27, 91, 66)
- RIGHT: `\033[C` (bytes: 27, 91, 67)
- LEFT:  `\033[D` (bytes: 27, 91, 68)

In `read_key()`: read one byte. If it's `\033` (ESC), read two more bytes. If byte 2 is `[`, byte 3 tells us which arrow. Return our own constant (KEY_UP=1000, etc.)

**Q: How do ANSI escape sequences work?**

They're a protocol between the program and the terminal emulator:
- `\033[2J` — clear entire screen (ESC [ 2 J)
- `\033[row;colH` — move cursor to position (ESC [ row ; col H)
- `\033[38;5;Nm` — set foreground to 256-color index N
- `\033[0m` — reset all formatting
- `\033[?25l` / `\033[?25h` — hide/show cursor

These are just bytes written to stdout. The terminal emulator interprets them and performs the actions.

### Deep Dive: PRNG and Math

**Q: Explain the Russian Peasant multiplication algorithm.**

(`math.c:3-23`)
To compute `a * b` using only shifts and additions:
```
Example: 13 * 6

  a=13, b=6:  b is even, skip.    a=26, b=3
  a=26, b=3:  b is odd, result += 26 = 26.  a=52, b=1
  a=52, b=1:  b is odd, result += 52 = 78.  a=104, b=0
  Done. 13 * 6 = 78
```
Algorithm: while b > 0, if b is odd add a to result. Double a, halve b. O(log b) iterations. This is exactly shift-and-add — the same principle hardware multipliers use.

**Q: Explain binary long division.**

(`math.c:25-50`)
To compute `a / b`:
```
Process a one bit at a time, from MSB to LSB:
  - Shift remainder left by 1
  - Bring down the next bit of a into remainder
  - If remainder >= b: subtract b from remainder, set quotient bit to 1
```
O(32) iterations for 32-bit numbers. Same algorithm as pencil-and-paper long division but in binary.

**Q: What is Schrage's method and why do we need it?**

(`math.c:75-86`)
MINSTD computes: `seed = (16807 * seed) mod (2^31 - 1)`

The problem: `16807 * seed` can overflow a 32-bit integer. Schrage's method splits this into two smaller multiplications:
- `m = 2^31 - 1 = 2147483647`
- `q = m / a = 127773` and `r = m % a = 2836`
- Then: `a*seed mod m = a*(seed mod q) - r*(seed / q)`
- If result < 0, add m

Each intermediate multiplication stays within 32-bit range. We avoid overflow without needing 64-bit types.

### Deep Dive: Game Loop

**Q: Walk through one complete tick of the game loop.**

(`main.c:101-243`)

```
Step 1: INPUT          read_key() — non-blocking, returns KEY_NONE if nothing pressed
        |
Step 2: DIRECTION      snake_set_direction() — reject opposite directions (|diff| == 2)
        |
Step 3: COMPUTE        snake_compute_next_head() — nx = head.x + dx[dir], ny = head.y + dy[dir]
        |
Step 4: WALL CHECK     Classic: die if nx/ny out of bounds
                        Wrap: nx = (nx + width) % width, same for ny
        |
Step 5: SELF CHECK     Iterate all segments except tail. Die if (nx,ny) matches any.
                        (Skip tail because it will vacate this tick)
        |
Step 6: OBSTACLE CHECK Linear scan of obstacles array. Die if match.
        |
Step 7: FOOD CHECK     grew = (food != NULL && nx == food.x && ny == food.y)
        |
Step 8: EXECUTE MOVE   alloc new head segment, prepend to linked list.
                        If !grew: walk to second-to-last, dealloc tail.
        |
Step 9: POST-MOVE      If ate food: increment score, update stats, check level-up,
                        spawn obstacles, free old food, spawn new food.
                        If food is timed: decrement ticks_remaining, respawn on expiry.
        |
Step 10: RENDER        Draw snake with gradient colors.
                        Update HUD only if score changed (delta rendering).
                        screen_flush() — push all buffered output at once.
        |
Step 11: DELAY         usleep(speed * 1000) where speed = max(80, 200 - level*15) ms
                        If slow effect active: speed += 50, decrement slow_ticks
```

---

## 8. Limitations and Improvements

### Honest Limitations (shows critical thinking):

**1. Only Forward Coalescing**
Our `dealloc()` merges with the next free block but not the previous one. If block A is freed, then block B (before A) is freed, B won't merge with A. A real allocator uses **boundary tags** (footers) to coalesce in both directions.

**2. O(n) Tail Removal**
`snake_move()` walks the entire linked list to find the second-to-last segment for tail removal. A **doubly-linked list** would make this O(1) at the cost of 4 extra bytes per segment.

**3. No Thread Safety**
Our allocator uses a static global `virtual_ram` with no synchronization. In a multi-threaded environment, concurrent alloc/dealloc calls would corrupt memory. Real allocators use mutexes or per-thread arenas.

**4. No Backward Free Block Search**
`alloc()` always starts scanning from the beginning of virtual RAM. A real allocator might maintain a **free list** (explicit linked list of free blocks) or use **next-fit** (start scanning from where the last allocation ended) for better performance.

**5. Food Spawn Can Loop**
`food_spawn()` retries randomly until it finds a valid position. On a nearly-full board this could take many iterations. The obstacle spawner has a 100-retry limit, but food doesn't (though we do detect victory when the board is full).

**6. Single-Process Model**
Everything runs in one process, one thread. A real system would separate input handling, game logic, and rendering into threads or processes with IPC (inter-process communication).

### What We Would Improve:

| Improvement | Concept It Would Demonstrate |
|---|---|
| Backward coalescing with boundary tags | Complete heap management |
| Doubly-linked list for snake | Trade-off analysis: memory vs performance |
| Free list for faster allocation | Data structure optimization in OS allocators |
| Buddy system allocator | Alternative allocation strategy |
| Input thread + game thread with mutex | Concurrency, synchronization |
| Memory-mapped file for stats | mmap(), virtual memory |
| Double buffering for rendering | Framebuffer management in display drivers |

---

## 9. Rapid-Fire Q&A

### Memory & Allocation

**Q: What's the time complexity of your allocator?**
A: `alloc()` is O(n) in number of blocks (linear scan). `dealloc()` is O(k) where k is consecutive free blocks after the target (for coalescing).

**Q: What does HEADER_SIZE=8 represent?**
A: 4 bytes for `size` (int), 1 byte for `in_use` (char), 3 bytes padding for 4-byte alignment. Total 8 bytes metadata per block.

**Q: What happens if alloc() runs out of memory?**
A: Returns NULL. Callers like `snake_move()` check for NULL and gracefully handle it (no crash, just no movement that tick).

**Q: Why MIN_ALLOC = 8?**
A: Prevents creating blocks so tiny they're useless. 8 bytes is enough to store a pointer (for a free list, if we added one) and avoids severe internal fragmentation.

**Q: Why MIN_SPLIT = 16?**
A: That's HEADER_SIZE(8) + MIN_ALLOC(8). If leftover space after allocation is less than 16 bytes, we can't fit a header + useful data, so don't bother splitting.

**Q: What is internal vs external fragmentation?**
A: Internal: wasted space inside an allocated block (e.g., asked for 5 bytes, got 8 due to MIN_ALLOC). External: free memory exists but is in scattered small blocks that can't satisfy a large request.

### Terminal & I/O

**Q: Why `_exit(0)` and not `exit(0)` in the signal handler?**
A: `_exit()` is async-signal-safe — it terminates immediately without calling atexit handlers or flushing stdio buffers, which could deadlock in a signal handler. `exit()` is NOT safe to call from signal handlers.

**Q: What's the difference between VMIN/VTIME settings?**
A: `VMIN=0, VTIME=0`: `read()` returns immediately with 0 bytes if no input (non-blocking). `VMIN=1, VTIME=0`: blocks until at least 1 byte arrives. `VMIN=0, VTIME=5`: waits up to 0.5 seconds, then returns.

**Q: Why TCSAFLUSH instead of TCSANOW?**
A: `TCSAFLUSH` discards any unread input before applying new settings, preventing leftover buffered characters from causing glitches. `TCSANOW` changes immediately but doesn't flush.

**Q: What is the ioctl() system call used for here?**
A: `ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)` queries the terminal driver for the window size. Returns a `winsize` struct with `ws_col` (columns) and `ws_row` (rows). We use it to make the game board fit the terminal.

### Game Logic

**Q: How do you prevent the snake from reversing into itself?**
A: `snake_set_direction()` calculates `|new_dir - current_dir|`. Our enum is UP=0, RIGHT=1, DOWN=2, LEFT=3. Opposite directions always differ by exactly 2 (UP/DOWN, LEFT/RIGHT). If diff == 2, the change is rejected.

**Q: Why skip the tail in self-collision check?**
A: The tail will be removed during this same tick (the move hasn't happened yet when we check). If we blocked it, the snake couldn't chase its own tail — a valid and expected gameplay pattern.

**Q: How does wrap-around mode work mathematically?**
A: `board_wrap_position()` uses modulo: `x = (x + width) % width`. Adding width before mod handles negative values (e.g., x=-1 becomes width-1, wrapping to the right edge). Same for y.

**Q: How are obstacles safely placed?**
A: `is_safe()` checks: not on another obstacle, not on food, not on any snake segment, and Manhattan distance > 2 from snake head (so the player has time to react). 100 random attempts; if all fail, skip that obstacle.

**Q: How does the speed formula work?**
A: `speed = max(80, 200 - level * 15)` milliseconds per frame. Level 0 = 200ms (5 fps). Level 8+ = 80ms (12.5 fps). This creates smooth progressive difficulty with a floor so the game is always playable.

### Architecture

**Q: How do the layers communicate?**
A: One-directional dependency: `main.c` creates game objects (from `game/`), passes them to renderers (in `ui/`), which use primitives (from `lib/`). No circular dependencies. Game logic never calls rendering functions. Rendering reads game state but never modifies it.

**Q: How do you test without a terminal?**
A: The `lib/` modules (math, string, memory) have zero terminal dependencies. We wrote 62 unit tests using a simple macro-based framework (ASSERT/TEST_SUMMARY). `make test` builds and runs them independently. Game logic in `game/` also has no terminal dependency and could be unit tested similarly.

**Q: Why no external libraries?**
A: The project constraint is to demonstrate OS-level concepts. Using ncurses, SDL, or standard library math/string functions would hide the very things we're trying to show. Everything is built from POSIX system calls and basic C I/O upward.

---

## 10. Phase 2 Features (Deep Dive + Snippets)

These were added in Phase 2 in response to the Phase 1 viva: a terminal-resize bug fix, AI ghost snake, multiple simultaneous food, speed powerup zones, a shrinking board, and a replay ghost of your best run.

Source files added: `src/game/ghost.{c,h}`, `src/game/powerups.{c,h}`, `src/game/replay.{c,h}`.

### 10.1 SIGWINCH-Driven Terminal Resize

**Problem (Phase 1 bug):** the game read terminal size only at startup. Cmd-+ / Cmd-- mid-game changed the actual columns/rows but the board still used the old size — the snake duplicated, food vanished, drawing went out of bounds.

**Fix:** install a `SIGWINCH` (window-change) signal handler. The handler does the bare minimum allowed inside a signal: set a `volatile sig_atomic_t` flag. The game loop polls the flag each tick; when set, it re-reads `TIOCGWINSZ`, resizes the board, refits the snake into the new bounds, and redraws.

```c
/* src/game/board.c */
static volatile sig_atomic_t g_winch_pending;

static void winch_handler(int sig) {
    (void)sig;
    g_winch_pending = 1;            /* only async-signal-safe work */
}

void terminal_install_winch_handler(void) {
    struct sigaction sa;
    g_winch_pending = 0;
    sa.sa_handler = winch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL); /* register kernel-delivered signal */
}

int terminal_consume_winch(void) {
    if (g_winch_pending) { g_winch_pending = 0; return 1; }
    return 0;
}
```

Game loop side (`src/main.c`):

```c
if (terminal_consume_winch() || !board_matches_terminal(board)) {
    int ar = apply_terminal_resize(...);   /* re-reads TIOCGWINSZ, refits */
    ...
}
```

`apply_terminal_resize` recomputes `board_w = tw - 4`, `board_h = th - 6`, calls `board_set_size`, then `snakes_refit_inset` to translate the snake (and ghost) so they remain inside the new bounds. Food that ended up off-board is cleared and respawned.

**OS concept:** signal handling. Why a flag instead of doing the work inside the handler? Because most functions (printf, malloc, even our `alloc`) are not async-signal-safe — calling them from a handler can deadlock or corrupt state. The "signal sets a flag, main loop services it" pattern is the textbook way to handle this safely.

---

### 10.2 AI Ghost Snake

A second, computer-controlled snake roams the board. It picks a random direction each tick, refuses moves that would hit a wall or itself, and kills the player on contact.

**Spawn (`src/game/ghost.c`)** — places a 3-segment snake in the playable area (respecting the inset from the shrinking board), avoiding the player, food, and obstacles. Up to 400 retries.

```c
Snake *ghost_create(Snake *player, Foods *foods, Obstacles *obs,
                    int bw, int bh, int inset, int *seed) {
    int play_w = bw - 2*inset, play_h = bh - 2*inset;
    if (play_w < 4 || play_h < 1) return NULL;

    for (int attempts = 0; attempts < 400; attempts++) {
        int sx = inset + 2 + my_mod(my_abs(pseudo_random(seed)), play_w - 2);
        int sy = inset + my_mod(my_abs(pseudo_random(seed)), play_h);
        Snake *g = snake_create(sx, sy);
        if (!g) return NULL;
        /* reject if any segment overlaps player/food/obstacle */
        ...
        if (ok) return g;
        snake_free(g);
    }
    return NULL;
}
```

**Step (random walk with wall avoidance):**

```c
int ghost_step(Snake *ghost, Board *board, int *seed) {
    for (int attempt = 0; attempt < 16; attempt++) {
        int d = my_mod(my_abs(pseudo_random(seed)), 4);
        snake_set_direction(ghost, (Direction)d);
        int gx, gy;
        snake_compute_next_head(ghost, &gx, &gy);

        if (board->mode == 0) {
            if (board_check_wall_collision(board, gx, gy)) continue;
        } else {
            board_wrap_position(board, &gx, &gy);
        }
        if (snake_check_self_collision(ghost, gx, gy)) continue;

        snake_move(ghost, gx, gy, 0);
        return 1;
    }
    return 0;   /* trapped this tick */
}
```

**Player collision** (`src/main.c`): after the player's move and after the ghost's move, `snake_snakes_overlap()` does an O(P*G) pairwise check; any overlap kills the player.

```c
if (ghost && snake_snakes_overlap(snake, ghost)) { snake->alive = 0; break; }
```

**Why it works:** the ghost reuses the existing `Snake` data structure and movement primitives — nothing new at the linked-list level. Only the *policy* (random direction with retry) is different.

---

### 10.3 Multiple Simultaneous Food

Phase 1 had a single food pointer. Phase 2 stores up to 8 food pieces in a fixed-size array, with a target count that grows with level.

```c
/* src/game/food.h */
#define MAX_FOOD_PIECES 8
#define MIN_FOOD_PIECES 3

typedef struct {
    Food pieces[MAX_FOOD_PIECES];
    int count;
} Foods;

int foods_target_count(int level) {           /* clamp(1+level, 3, 8) */
    int t = 1 + level;
    if (t < MIN_FOOD_PIECES) t = MIN_FOOD_PIECES;
    if (t > MAX_FOOD_PIECES) t = MAX_FOOD_PIECES;
    return t;
}
```

`foods_fill_to_target` keeps spawning until the array reaches the target or it can't find a free cell:

```c
void foods_fill_to_target(Foods *fs, Snake *s, Snake *ghost, Obstacles *obs,
                          int board_w, int board_h, int inset, int *seed, int target) {
    int guard = 0;
    while (fs->count < target && guard < 500) {
        if (foods_try_add_one(fs, s, ghost, obs, board_w, board_h, inset, seed) != 0)
            break;
        guard++;
    }
}
```

**Bug we hit and fixed:** when timed food (`FOOD_BONUS`/`FOOD_SLOW`) expired, the loop erased the cell and called `foods_fill_to_target` to add a replacement, **but didn't call `render_foods` afterwards.** The new food existed in the array but was never drawn. After several expiries the board appeared to run out of food. Fix:

```c
/* src/main.c — food expiry pass */
if (expired_any) {
    foods_fill_to_target(&foods, snake, ghost, &obstacles, board_w, board_h,
                         board->inset, &seed, target_food);
    render_foods(&foods, board, theme);   /* <-- the missing call */
}
```

**Removal trick:** `foods_remove_at` does an O(1) "swap with last" instead of shifting:

```c
void foods_remove_at(Foods *fs, int index) {
    fs->pieces[index] = fs->pieces[fs->count - 1];
    fs->count--;
}
```

---

### 10.4 Speed Powerup Zones

Glowing tiles spawn on the board. Walking through one gives the snake a temporary speed change: `+/>` (boost) or `%/<` (slow). The zone has its own lifetime independent of being hit.

**Data (`src/game/powerups.h`):**

```c
typedef enum { POW_BOOST, POW_SLOW } PowerupType;

typedef struct {
    int x, y;
    PowerupType type;
    int ticks_remaining;   /* zone lifetime on the board */
    int glow_phase;        /* animates the displayed character */
} Powerup;

#define MAX_POWERUPS 3
#define POWERUP_LIFETIME 120
#define POWERUP_EFFECT_TICKS 25
```

**Per-tick maintenance** — animate, expire, allow new spawns on a cooldown:

```c
int powerups_tick(Powerups *p) {
    int expired = 0;
    if (p->spawn_cooldown > 0) p->spawn_cooldown--;

    int i = 0;
    while (i < p->count) {
        p->zones[i].glow_phase = (p->zones[i].glow_phase + 1) % 4;
        if (p->zones[i].ticks_remaining > 0) {
            p->zones[i].ticks_remaining--;
            if (p->zones[i].ticks_remaining == 0) {
                powerups_remove_at(p, i);
                expired = 1;
                continue;
            }
        }
        i++;
    }
    return expired;
}
```

**Hit detection in the game loop (`src/main.c`):**

```c
pup_idx = powerups_find_at(&powerups, nx, ny);
if (pup_idx >= 0) {
    if (powerups.zones[pup_idx].type == POW_BOOST) {
        boost_ticks = POWERUP_EFFECT_TICKS;
        slow_ticks = 0;
    } else {
        slow_ticks = POWERUP_EFFECT_TICKS;
        boost_ticks = 0;
    }
    render_powerup_clear(...);
    powerups_remove_at(&powerups, pup_idx);
}
```

**How the effect actually changes speed** — modifies the per-tick `usleep` delay:

```c
speed = score_get_speed(score);
if (boost_ticks > 0) { speed = my_max(40, speed - 70); boost_ticks--; }
if (slow_ticks  > 0) { speed += 50;                    slow_ticks--;  }
usleep(my_multiply(speed, 1000));
```

**Glow animation** is just the renderer alternating two characters using `glow_phase`:

```c
ch = (p->zones[i].glow_phase % 2) ? '>' : '+';   /* boost */
ch = (p->zones[i].glow_phase % 2) ? '<' : '%';   /* slow */
```

---

### 10.5 Shrinking Board

Every 3 levels the playable area shrinks by 1 cell on every side (an "inset"). The terminal-sized outer border stays fixed; an inner border is drawn at the new playable boundary.

**Data + helpers (`src/game/board.h`, `board.c`):**

```c
typedef struct {
    int width, height;
    int mode;
    int inset;   /* playable area: [inset, w-1-inset] x [inset, h-1-inset] */
} Board;

int board_inset_for_level(int board_w, int board_h, int level) {
    int target = my_divide(level, 3);   /* shrink every 3 levels */
    int max_inset = my_divide(board_w - 6, 2);
    if (my_divide(board_h - 4, 2) < max_inset) max_inset = my_divide(board_h - 4, 2);
    if (target > max_inset) target = max_inset;   /* keep play area >= 6x4 */
    return target;
}
```

**Wall collision now respects inset:**

```c
int board_check_wall_collision(Board *b, int x, int y) {
    int lo_x = b->inset,           lo_y = b->inset;
    int hi_x = b->width - b->inset - 1;
    int hi_y = b->height - b->inset - 1;
    if (x < lo_x || x > hi_x || y < lo_y || y > hi_y) return 1;
    return 0;
}
```

**Wrap mode also respects inset** so wrap-around bounces off the *inner* edges, not the terminal edges:

```c
void board_wrap_position(Board *b, int *x, int *y) {
    int play_w = b->width  - 2*b->inset;
    int play_h = b->height - 2*b->inset;
    *x = b->inset + my_mod(*x - b->inset + play_w, play_w);
    *y = b->inset + my_mod(*y - b->inset + play_h, play_h);
}
```

**Refitting the snake when the wall closes in** — translate the bounding box of player+ghost so it's centered inside the new shrunken region:

```c
/* src/game/snake.c */
int snakes_refit_inset(Snake *player, Snake *ghost, int new_w, int new_h, int inset) {
    int min_x, max_x, min_y, max_y;
    int play_w = new_w - 2*inset, play_h = new_h - 2*inset;
    /* compute bbox over both snakes ... */
    int bw = max_x - min_x + 1, bh = max_y - min_y + 1;
    if (bw > play_w || bh > play_h) return 0;   /* can't fit, snake dies */
    int ox = -min_x + inset + my_divide(play_w - bw, 2);
    int oy = -min_y + inset + my_divide(play_h - bh, 2);
    /* translate every segment by (ox, oy) */
}
```

**Triggered on level-up (`src/main.c`):**

```c
if (score->level > prev_level) {
    if (apply_level_shrink(...)) {        /* changes inset, refits snake */
        if (!snake->alive) break;
        redraw_full_game(...);            /* full clear + redraw with inner border */
    }
}
```

**Inner border drawn by the renderer** when `b->inset > 0`:

```c
if (b->inset > 0) {
    int x0 = OFFSET_X + b->inset - 1;
    int y0 = OFFSET_Y + b->inset - 1;
    int x1 = OFFSET_X + b->width  - b->inset;
    int y1 = OFFSET_Y + b->height - b->inset;
    /* corners + edges (-, |) */
}
```

---

### 10.6 Replay Ghost Mode (Best-Run Recording)

After every game, if the score beats the previously saved best, the head trail is written to `data/replay.dat`. The next game loads that trail and replays it as a dim `.` ghost stepping in lockstep with the live tick.

**Recording format (`src/game/replay.h`):**

```c
#define REPLAY_MAX_FRAMES 6000

typedef struct { short x; short y; } ReplayFrame;

typedef struct {
    ReplayFrame frames[REPLAY_MAX_FRAMES];
    int count;
    int score;
    int board_w;
    int board_h;
} Replay;
```

`short` halves the storage vs `int`; 6000 frames × 4 bytes = 24 KB per replay — easily fits in our 64 KB heap *plus* on-disk file.

**Recording each tick (`src/main.c`):**

```c
snake_move(snake, nx, ny, grew);
replay_record_frame(&record, nx, ny);   /* head position after the move */
```

**Saving only if best, with magic number for sanity (`src/game/replay.c`):**

```c
#define REPLAY_MAGIC 0x52504C59   /* "RPLY" */

void replay_save_if_best(const Replay *rec, int final_score) {
    int prev_best = 0;
    load_score_only(&prev_best);
    if (final_score <= prev_best) return;

    FILE *f = fopen(REPLAY_PATH, "wb");
    int magic = REPLAY_MAGIC;
    fwrite(&magic, sizeof(int), 1, f);
    fwrite(&final_score, sizeof(int), 1, f);
    fwrite(&rec->board_w, sizeof(int), 1, f);
    fwrite(&rec->board_h, sizeof(int), 1, f);
    fwrite(&rec->count, sizeof(int), 1, f);
    fwrite(rec->frames, sizeof(ReplayFrame), rec->count, f);
    fclose(f);
}
```

**Loading + magic check** — refuses files that don't start with `RPLY`, so a corrupt or unrelated file can't crash us:

```c
if (fread(&magic, sizeof(int), 1, f) != 1 || magic != REPLAY_MAGIC) {
    fclose(f); return 0;
}
```

**Replay during the next game** — we keep an index `replay_step` and a "previous cell" so we can erase the old `.` before drawing the new one. We *also* check that the previous cell isn't currently occupied by the snake/ghost/food/powerup before erasing — that prevents the replay's eraser from punching holes in real game elements:

```c
if (has_best && replay_step < best.count) {
    if (replay_prev_x >= 0 &&
        board_in_play_area(board, replay_prev_x, replay_prev_y) &&
        !snake_occupies_cell(snake, replay_prev_x, replay_prev_y) &&
        (!ghost || !snake_occupies_cell(ghost, replay_prev_x, replay_prev_y)) &&
        foods_find_at(&foods, replay_prev_x, replay_prev_y) < 0 &&
        powerups_find_at(&powerups, replay_prev_x, replay_prev_y) < 0) {
        render_replay_ghost_clear(replay_prev_x, replay_prev_y);
    }
    replay_prev_x = best.frames[replay_step].x;
    replay_prev_y = best.frames[replay_step].y;
    if (board_in_play_area(board, replay_prev_x, replay_prev_y))
        render_replay_ghost(replay_prev_x, replay_prev_y, theme);
    replay_step++;
}
```

**Why no collision with the replay ghost?** It's a *visual* aid showing your previous best run, not another opponent. If we made it lethal you couldn't use it as a guide.

---

### 10.7 Likely Phase 2 Viva Questions

**Q: Why do you set a flag in the SIGWINCH handler instead of resizing immediately?**
A: Signal handlers can interrupt the program at any point, including the middle of a `printf` or our `alloc()`. Calling non-async-signal-safe functions risks deadlock or memory corruption. The flag pattern keeps the handler trivial — only `volatile sig_atomic_t = 1` — and defers all real work to the main loop where it's safe.

**Q: Why `volatile sig_atomic_t` for the flag?**
A: `volatile` stops the compiler from caching the variable in a register (it might be set asynchronously), and `sig_atomic_t` is the only integer type C guarantees can be read/written atomically with respect to a signal interrupt.

**Q: Why does the ghost snake re-pick a direction up to 16 times per step?**
A: A purely random direction would often be invalid (would hit a wall or the ghost's own body). 16 retries lets it "wiggle" out of corners. If all 16 fail (it's truly trapped), `ghost_step` returns 0 and the ghost stands still that tick — better than crashing or moving illegally.

**Q: Why store food in a fixed-size array instead of a linked list?**
A: Food count is small (≤ 8) and fixed at compile time. An array gives O(1) random access, no per-item allocation, and the swap-with-last removal trick is O(1). A linked list would add allocation overhead for no gain.

**Q: Walk through what happens when the inset increases by 1 at a level-up.**
A: `apply_level_shrink` calls `board_set_inset`, which clamps and stores the new inset on the Board. Then `snakes_refit_inset` computes the bounding box of player+ghost, checks it still fits the smaller play area (else snake dies), and translates every segment by an offset that recenters the snake inside the new region. Food and powerups outside the new playable area are dropped and respawned inside it. Finally the screen is fully redrawn so the new inner border appears.

**Q: How big can the replay file get?**
A: Header is 5 ints = 20 bytes. Each frame is 2 shorts = 4 bytes. Max 6000 frames = 24,000 bytes payload. So <24 KB total. We chose `short` (16-bit) over `int` (32-bit) for x/y because the board dimensions are well under 32,767.

**Q: What's the magic number for, in the replay file?**
A: It's a 4-byte signature (`"RPLY"` = 0x52504C59) at the start of the file. Before reading anything else we check `magic == REPLAY_MAGIC`. If a user accidentally puts a different file at that path, or the file is corrupted, we refuse to load it instead of crashing.

**Q: Why doesn't the replay ghost cause death on collision?**
A: Design choice. It's a *learning aid* — the dim trail shows where you were doing well in your previous best run, so you can see how to beat it. If touching it killed you, you couldn't follow it.

**Q: How do powerups interact with the slow-food effect?**
A: Slow-food and slow-powerup share the `slow_ticks` counter. Boost-powerup uses a separate `boost_ticks`. Picking up a boost zeroes `slow_ticks` (and vice versa) so opposite effects don't cancel ambiguously — the latest one wins.
