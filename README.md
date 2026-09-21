the below are all AI-generated

---

# Swarm

A high-performance, data-oriented 2D entity simulation and rendering engine written in **Odin** from scratch. Swarm is designed to simulate and render massive crowds of autonomous entities (demonstrating **10,000+ interactive NPCs** with layered sprites and collisions) at silky-smooth frame rates with **zero runtime heap allocations**.

---

## Table of Contents

- [Overview & Vision](#overview--vision)
- [How to Build and Run](#how-to-build-and-run)
  - [Prerequisites](#prerequisites)
  - [First-Time Run (Compiling Vendors)](#first-time-run-compiling-vendors)
  - [Subsequent Runs](#subsequent-runs)
  - [Building an Optimized Executable](#building-an-optimized-executable)
- [What Makes Swarm Special?](#what-makes-swarm-special)
- [Project Architecture & Directory Structure](#project-architecture--directory-structure)
- [Memory Management Architecture](#memory-management-architecture)
  - [The Arena Hierarchy](#the-arena-hierarchy)
  - [Double-Buffered Scratch Arenas](#double-buffered-scratch-arenas)
- [Simulation Loop & Frame Pipeline](#simulation-loop--frame-pipeline)
  - [Fixed-Timestep Simulation with Sub-Tick Interpolation](#fixed-timestep-simulation-with-sub-tick-interpolation)
  - [System Execution Order](#system-execution-order)
- [Deep Dive: The `src/entity/` Subsystem](#deep-dive-the-srcentity-subsystem)
  - [1. Generic Packed Pool (`pool.odin`)](#1-generic-packed-pool-poolodin)
  - [2. Entity Architecture & Component Linking (`entity.odin`)](#2-entity-architecture--component-linking-entityodin)
  - [3. Spatial Hash Grid Manager (`chunk.odin`)](#3-spatial-hash-grid-manager-chunkodin)
  - [4. Dynamic BVH & Fat AABB Tree (`collider.odin`)](#4-dynamic-bvh--fat-aabb-tree-colliderodin)
  - [5. Sprite Layering & Radix Sort Integration (`sprite.odin`)](#5-sprite-layering--radix-sort-integration-spriteodin)
  - [6. Action Queue & Status System (`action.odin`, `status.odin`)](#6-action-queue--status-system-actionodin-statusodin)
- [Rendering Pipeline & Radix Sort (`src/engine/draw.odin`)](#rendering-pipeline--radix-sort-srcenginedrawodin)
- [The Demo Scene: 10,000 NPCs (`src/game/game.odin`)](#the-demo-scene-10000-npcs-srcgamegameodin)
- [Configuration Reference (`#config`)](#configuration-reference-config)

---

## Overview & Vision

Swarm is built around mechanical sympathy and data-oriented principles. Rather than relying on traditional heavy Object-Oriented hierarchies or generic, bloated relational Entity-Component-System (ECS) frameworks, Swarm uses a **Hybrid Sparse-Set Pool + Intrusive Linked List** architecture paired with **Arena-based memory pools**, **Spatial Hash Grids**, **Dynamic BVH Trees**, and a **64-bit LSD Radix-Sorted 2D rendering pipeline**.

### Core Highlights
- **10,000+ Active Entities**: Simultaneous physics, multi-layer animated sprites, colliders, and spatial partitioning.
- **Zero Runtime Allocations**: No garbage collection pauses, no OS heap fragmentation; all memory is managed through pre-allocated bump arenas.
- **Cache-Optimized Iteration**: Contiguous memory arrays for dense entity updates.
- **Deterministic Simulation**: 20 TPS fixed-rate simulation loop with sub-tick visual Hermite/linear interpolation for 60/144/240Hz monitors.
- **High-Throughput Rendering**: 8-pass 64-bit Radix Sort grouping draws by layer, screen-Y depth, and component sub-order in $O(N)$ time.

---

## How to Build and Run

### Prerequisites

1. **[Odin Compiler](https://odin-lang.org/)**.
2. **CMake** (v3.20 or newer).
3. **C/C++ Build Toolchain**:
   - **Windows**: Visual Studio (MSVC) with C++ Desktop tools.
   - **Linux**: GCC / Clang and standard build essentials.

The project bundles the C source code for **SDL3** and **SDL3_image** in the `vendor/` directory.

### First-Time Run (Compiling Vendors)

From the project root, simply run:

```bash
odin run .
```

### Subsequent Runs

Once the vendor libraries in `vendor/build/` have been compiled, you can bypass CMake for near-instant compilation and launch:

```bash
odin run . -define:BUILD_VENDORS=false
```

Or you can invoke the game package directly from `src/`:

```bash
odin run src
```

---

## What Makes Swarm Special?

1. **No-Heap Runtime Philosophy**
   Every single subsystem (entities, sprites, colliders, chunks, draw calls, query results) draws its memory from a single monolithic Arena (`omni_arena`) or transient double-buffered scratch arenas (`tick_arena`, `frame_arena`). Not a single `malloc`, `free`, or runtime reallocation occurs inside the main loop.
2. **Hybrid Sparse-Set + In-Pool Intrusive Lists**
   Components are stored in contiguous generic packed pools (`pool($T)`). Entities can attach an arbitrary number of components via embedded doubly-linked lists inside the component pools. This provides $O(1)$ component addition, $O(1)$ cascade destruction, and direct dense array iteration without pointer indirection.
3. **Fail-Safe "Index 0" Stub Pattern**
   Every pool, chunk map, texture cache, and node tree reserves slot `0` as an immutable "stub/null object". Querying or updating an expired or invalid ID returns a valid pointer to this dummy object with a warning log rather than causing a segmentation fault or memory corruption.

---

## Project Architecture & Directory Structure

```
swarm/
├── main.odin                 # Root bootstrapper; builds SDL3/SDL3_image via CMake & launches src
├── asset/                    # Game assets
│   └── img/
│       ├── char_base.png     # Character body sprite atlas (races & genders)
│       └── char_hair.png     # Character hair sprite atlas
├── vendor/                   # Submodule / source trees for external C libraries
│   ├── SDL3/
│   └── SDL3_image/
└── src/
    ├── main.odin             # Main loop, window creation, tick accumulator, frame orchestration
    ├── global/
    │   └── global.odin       # Global state: arenas, window, paths, timing, config
    ├── engine/
    │   ├── core/
    │   │   ├── mem.odin      # Bump Arena allocator implementation
    │   │   └── log.odin      # Colorized, level-filtered console logger
    │   └── draw.odin         # Texture cache, draw queue, 8-pass Radix sort, SDL3 renderer
    ├── entity/               # The core data-oriented entity engine
    │   ├── pool.odin         # Generic packed sparse-set pool with free list
    │   ├── entity.odin       # Entity definition, lifecycle, transform sync, cascade cleanup
    │   ├── chunk.odin        # Open-addressing spatial hash grid manager
    │   ├── collider.odin     # Dynamic BVH AABB Tree with Fat AABBs & AVL balancing
    │   ├── sprite.odin       # Sprite profiles, multi-layer sprites, visual tick interpolation
    │   ├── action.odin       # Inter-entity action queue ring buffer
    │   └── status.odin       # Status effects & buff/debuff pool
    └── game/
        └── game.odin         # Demo scenario: spawns and updates 10,000 animated NPCs
```

---

## Memory Management Architecture

Memory in Swarm is strictly hierarchical and predictable.

```
+-------------------------------------------------------------------------------+
|                                  omni_arena                                   |
|                              (Default: 500 MB)                                |
|  +---------------------------+-----------------------+---------------------+  |
|  | Persistent System Pools   | tick_arena_raw[2]     | frame_arena_raw[2]  |  |
|  | (Entities, Sprites,       | (Double-buffered,     | (Double-buffered,   |  |
|  |  Colliders, Chunks, etc.) |  2 x 50 MB)           |  2 x 50 MB)         |  |
|  +---------------------------+-----------------------+---------------------+  |
+-------------------------------------------------------------------------------+
```

### The Arena Hierarchy

Defined in [`src/engine/core/mem.odin`] and initialized in [`src/global/global.odin`]:

1. **`omni_arena`** (`500 MB`):
   - Allocated once at program startup via `mem.alloc()`.
   - Lives for the entire duration of the process.
   - Holds all long-lived arrays: entity pools, sprite pools, collider trees, chunk hash entries, texture handles, and draw call buffers.
2. **Sub-Arenas**:
   - `core.arena_init_in_arena()` carves sub-arenas out of `omni_arena` without touching the OS allocator.

### Double-Buffered Scratch Arenas

To avoid memory fragmentation and allow safe multi-buffering:
- **`tick_arena_raw[2]`** (`50 MB` each):
  - Swapped and reset at the beginning of each simulation tick:
    ```odin
    global.tick_arena_cur_idx = 1 - global.tick_arena_cur_idx
    global.tick_arena = &global.tick_arena_raw[global.tick_arena_cur_idx]
    core.arena_reset(global.tick_arena)
    ```
  - Used for transient calculations, such as the output slices of spatial chunk queries (`chunk_mng_query`).
- **`frame_arena_raw[2]`** (`50 MB` each):
  - Swapped and reset at the beginning of every render frame.
  - Used for per-frame temporary string formatting, UI allocations, or transient visual sorting structures.

Those double-buffered scratch arenas allow reading the last tick/frame data.

---

## Simulation Loop & Frame Pipeline

Defined in [`src/main.odin`].

### Fixed-Timestep Simulation with Sub-Tick Interpolation

```
       Frame Start (Wall Clock Time)
                    │
                    ▼
         Poll SDL Window Events
                    │
                    ▼
         Swap & Reset Frame Arena
                    │
                    ▼
           Compute Delta Time
     (Clamped to max 0.2s / min 5 FPS)
                    │
                    ▼
     Accumulate Time: accum += dt
                    │
       ┌────────────┴────────────┐
       ▼                         ▼
 [accum > tick_delta]       [accum <= tick_delta]
       │                         │
       ├─ Swap Tick Arena        │
       ├─ game_update()          │
       ├─ sprite_update_early()  │
       ├─ action_update()        │
       ├─ entity_update()        │
       ├─ collider_update()      │
       ├─ game_update_late()     │
       └─ accum -= tick_delta    │
       (Loop until drained)      │
                    │            │
                    └────────────┘
                                 │
                                 ▼
              Compute Alpha: accum / tick_delta
                                 │
                                 ▼
                     Interpolate & Draw Visuals
                                 │
                                 ▼
                     Radix Sort Draw Commands
                                 │
                                 ▼
                       SDL_RenderPresent
```

### System Execution Order

Inside each tick (running at 20 TPS):
1. **`game.game_update()`**: User game logic, AI state transitions, steering behaviors.
2. **`entity.sprite_sys_update_early()`**: Records `last_tick_pos` for every active sprite before positions change, enabling interpolation during rendering.
3. **`entity.action_sys_update()`**: Processes the inter-entity action queue.
4. **`entity.entity_sys_update()`**: Integrates velocities into positions (`pos += vel`). If an entity moved, synchronizes its chunk position and updates the destinations and sorting keys of all child sprites and colliders.
5. **`entity.collider_sys_update()`**: Synchronizes collider chunks, checks Fat AABB containment, updates dynamic BVH nodes, and tests broadphase pairs.
6. **`game.game_update_late()`**: Post-physics resolution and cleanup.

During rendering:
1. **`game.game_visual()`**: Per-frame visual effects and camera setup.
2. **`game.game_draw()`**: Debug overlays (e.g., BVH bounding boxes).
3. **`entity.sprite_sys_draw()`**: Computes interpolated screen positions `interpolate_pos = lerp(last_tick_pos, dest, alpha)`, updates sprite chunk coordinates, and pushes draw calls into the draw buffer.
4. **`engine.draw_present()`**: Radix-sorts all queued draws and issues SDL3 render batches.

---

## Deep Dive: The `src/entity/` Subsystem

The `src/entity/` directory contains the architecture of the engine. Here is an in-depth breakdown of how each module works and collaborates.

### 1. Generic Packed Pool (`pool.odin`)

[`src/entity/pool.odin`] implements a generic **Packed Sparse-Set Pool with an embedded Free List Stack**:

```odin
pool :: struct($T: typeid) {
    slot_pool: [^]i32,
    data_list: [^]T,
    cap, head, max_key, len: u32,
    data_field_key_offset: uintptr,
}
```

#### Key Architecture & Invariants:
- **Compile-Time/Init Reflection**: `pool_init` verifies that type `$T` contains a `key: u32` field using Odin's `reflect.struct_field_by_name(T, "key")` and records its exact byte offset.
- **Dense Data List (`data_list`)**:
  - Active elements are packed contiguously from index `1` to `len - 1`.
  - Iterating over all active entities or components is a contiguous, cache-friendly array traversal without holes.
- **Sparse Mapping & Free List (`slot_pool`)**:
  - `slot_pool[key]` serves a dual purpose:
    - **When Alive**: `slot_pool[key] = -i32(dense_index)`. The negative sign indicates the slot is alive, and its absolute value is the index in `data_list`.
    - **When Dead / Free**: `slot_pool[key] = +i32(next_free_key)`. Dead keys form a singly-linked free list stack starting at `pool.head`.
- **$O(1)$ Allocation (`pool_create`)**:
  - If `head == max_key`, allocates the next unused sequential key (`max_key += 1`).
  - Otherwise, pops the top free key from `slot_pool[head]`.
  - Appends the new item to `data_list[len]`, writes `key` into `ptr.key` via offset, sets `slot_pool[key] = -i32(len)`, and increments `len`.
- **$O(1)$ Swap-and-Pop Deletion (`pool_destroy`)**:
  - When key `K` (at dense index `idx`) is destroyed, the last element in `data_list` (`len - 1`) is copied over into `idx`.
  - The moved item's `slot_pool` entry is updated to point to `idx`.
  - The freed key `K` is prepended to the free list (`slot_pool[K] = head; head = K`).
  - `len` is decremented.
- **The Index-0 Stub**:
  - Slot `0` is initialized as a dummy object.
  - `pool_get()` on an invalid or dead key logs a trace and returns `&pool.data_list[0]`, preventing crashes.
- **Validation Engine (`_pool_validate`)**:
  - Verifies bidirectional consistency (`data_list[idx].key == key`), checks for cycles in the free list, and validates that dense array bounds match active counts.

---

### 2. Entity Architecture & Component Linking (`entity.odin`)

[`src/entity/entity.odin`] defines the entity structure and coordinates components.

```odin
entity :: struct {
    key: u32,
    logic_flag: u8,
    pos: [2]f32,
    scale: [2]f32,
    z: i8,
    vel: [2]f32,
    last_pos: [2]f32,

    chunk_key: u32,

    // Head keys for intrusive linked lists in component pools
    spr_begin, spr_len: u32,
    col_begin, col_len: u32,
    status_begin, status_len: u32,
}
```

#### Intrusive Doubly-Linked Component Architecture:
Instead of allocating dynamic arrays (`[dynamic]^Component`) for each entity—which causes severe heap fragmentation—each component struct (`sprite`, `collider`, `status`) contains:
- `ett_owner: u32`: Key of the owning entity.
- `ett_links: [2]u32`: Index `0` is the `prev` key; index `1` is the `next` key in that component's pool.

```
Entity (key: 42)
  ├── spr_begin: 105 ───► Sprite 105 (Base Body)
  │                         └── ett_links[1]: 208 ───► Sprite 208 (Hair)
  │                                                      └── ett_links[1]: 0 (End)
  └── col_begin: 301 ───► Collider 301 (Hitbox)
                            └── ett_links[1]: 0 (End)
```

- **Attaching Components**: Functions like `entity_new_sprite()` create a component in the `sprite_pool` and prepend it to `ett.spr_begin` in $O(1)$ time.
- **Cascade Deletion**: When `entity_destroy(key)` is called, it iterates through `spr_begin`, `col_begin`, and `status_begin`, destroying each attached component in its respective pool, unlinks the entity from the spatial chunk manager, and frees the entity itself.

#### Dirty-Checking Transform Propagation:
In `entity_sys_update()`:
- `ptr.pos += ptr.vel`
- Positions are compared against `ptr.last_pos`.
- **Only if an entity has moved**:
  1. Updates the spatial partition chunk (`chunk_mng_update`).
  2. Traverses all attached sprites, updates their `dest` rectangle based on `pos + offset * scale`, and recomputes their 64-bit Radix sorting key.
  3. Traverses all attached colliders and updates their world bounding rectangles.
  4. Updates `ptr.last_pos = ptr.pos`.
- On creation, `last_pos` is initialized to `NaN`, guaranteeing that initial transforms are computed on the first frame.

---

### 3. Spatial Hash Grid Manager (`chunk.odin`)

[`src/entity/chunk.odin`] implements a 2D spatial hashing grid for broadphase queries and culling.

#### Structure & Data Representation:
- **`chunk_size`**: Default 1024.0 units per grid cell.
- **Coordinate Hashing**: Translates world positions `(x, y)` to cell `(cx, cy) = (floor(x / chunk_size), floor(y / chunk_size))`.
- **Hash Function**: Combines coordinates into a `u64` and applies a high-entropy 64-bit SplitMix-style hash:
  ```odin
  hash := (u64(pos[0]) << 32) | u64(pos[1])
  hash ~= hash >> 30; hash *= 0xbf58476d1ce4e5b9
  hash ~= hash >> 27; hash *= 0x94d049bb133111eb
  hash ~= hash >> 31
  ```
- **Open-Addressing Hash Table (`chunk_map`)**: Fixed-capacity array with linear probing.
- **Intrusive Item Chains**: Each hash bucket contains `ins_begin: u32` (head of a doubly-linked list of `chunk_instance` items located in `ins_pool`).

#### Boundary-Crossing Optimization:
When `chunk_mng_update(mng, key, new_center)` is called:
1. Calculates `new_chunk_pos`.
2. If `new_chunk_pos == old_chunk_pos`, **returns immediately** (zero work for entities moving within the same chunk!).
3. If crossed, unlinks the instance from the old chunk bucket and prepends it to the new chunk bucket in $O(1)$.

#### Broadphase Range Query (`chunk_mng_query`):
- Given a query rectangle, expands by `ENTITY_MAX_BOUNDS_SIZE / 2` (512px).
- Determines `min_chunk` and `max_chunk` grid coordinates.
- Collects all candidate item keys into an array dynamically allocated from `global.tick_arena`.
- Returns the results with **zero heap allocation overhead**!

---

### 4. Dynamic BVH & Fat AABB Tree (`collider.odin`)

[`src/entity/collider.odin`] provides a dynamic Bounding Volume Hierarchy (AABB Tree) for collision detection.

```odin
collider_node :: struct {
    pool_flag: u32,
    collider_key: u32,
    parent: u32,
    child: [2]u32,
    height: i32,
    rect: [4]f32,
    tag: u32,
}
```

#### 1. Fat AABB Caching
To prevent re-inserting moving colliders into the BVH tree every tick, each node stores a **Fat AABB**:
$$\text{Fat Rect} = \text{Collider Rect} \pm \text{fat\_aabb\_offset} \quad (\text{default: } 16\text{px})$$
In `collider_sys_update()`, if `node.rect` still completely contains `collider.rect`, no tree update occurs! Only when the collider moves outside its fat boundary is it removed and re-inserted.

#### 2. Surface Area Heuristic (SAH) Greedy Insertion
When inserting a leaf:
- Calculates the cost of placing the new leaf under candidate nodes based on **bounding box perimeter (surface area in 2D)**.
- Traverses down the tree choosing the branch that minimizes perimeter expansion and inheritance costs:
  $$\text{Cost} = 2 \times \text{Perimeter}(\text{Union}(\text{Node}, \text{Leaf})) + \text{InheritCost}$$

#### 3. AVL-Style Tree Balancing
As leaves are inserted or removed, `_collider_tree_balance()` evaluates the height difference between child subtrees (`balance = height(C) - height(B)`). If $|balance| > 1$, it executes tree rotations (left/right rotations) to maintain balanced $O(\log N)$ tree height.

#### 4. Hierarchical Broadphase Pair Processing (`_collider_tree_process_pairs`)
- Recursively traverses pairs of nodes $(A, B)$.
- Immediately prunes subtrees if their bounding boxes do not overlap.
- When both are leaves ($A \neq B$), checks exact collider overlap and triggers the collision callback.
- Supports both internal self-collisions ($A = B$) and cross-tree queries.

---

### 5. Sprite Layering & Radix Sort Integration (`sprite.odin`)

[`src/entity/sprite.odin`] manages visual presentation and texture mappings.

#### Sprite Profiles (Flyweight Pattern):
- `sprite_profile` maps a sub-rectangle of a loaded texture:
  ```odin
  sprite_profile :: struct {
      tex: u32,
      rect: [4]u32, // x, y, w, h in pixel coordinates
  }
  ```
- Created once at startup (e.g., specific hair styles, race/gender base bodies) and shared across all sprite instances.

#### Sub-Tick Interpolation:
- In `sprite_sys_update_early()`:
  `last_tick_pos = dest.xy`
- In `sprite_sys_draw()`:
  $$\text{render\_pos} = \text{last\_tick\_pos} + (\text{dest} - \text{last\_tick\_pos}) \times \text{tick\_frame\_alpha}$$
- Smooths out motion on high-refresh-rate displays regardless of the simulation tick rate.

#### Bit-Packed 64-Bit Sorting Key:
Each sprite computes a 64-bit unsigned integer `sorting` that packs all layering and depth information:

```
 63       56 55                                24 23       16 15        0
+-----------+------------------------------------+-----------+-----------+
| Entity Z  |          Screen Y-Depth            | Sub-Z     | Reserved  |
|  (8 bits) |             (32 bits)              | (8 bits)  | (16 bits) |
+-----------+------------------------------------+-----------+-----------+
```

1. **Entity Layer Z (Bits 56..63)**:
   ```odin
   spr.sorting |= u64(transmute(u8)ptr.z ~ 0x80) << 56
   ```
   Biased with `~ 0x80` so signed `i8` integers sort correctly as unsigned.
2. **Screen Y Position (Bits 24..55)**:
   ```odin
   _y: u32 = transmute(u32)ptr.pos[1]
   _y ~= (u32(-i32(_y >> 31)) | 0x80000000)
   spr.sorting |= u64(_y) << 24
   ```
   Converts IEEE-754 floating-point $Y$-coordinates into order-preserving unsigned integers. Characters with greater $Y$ values (lower on the screen) are drawn in front of characters further up the screen, creating 2.5D depth sorting!
3. **Component Sub-Z (Bits 16..23)**:
   ```odin
   spr.sorting |= u64(transmute(u8)spr.ett_z ~ 0x80) << 16
   ```
   Ensures that an entity's hair (`ett_z = 1`) always renders on top of its body (`ett_z = 0`).

---

### 6. Action Queue & Status System (`action.odin`, `status.odin`)

- **`action_sys`** ([`action.odin`]): A pre-allocated FIFO ring buffer of `action` structs (`from`, `to`, `type`, `data: [3]u32`). Decouples inter-entity messaging (attacks, spell casts, events) so systems can emit actions during updates without immediate mutation hazards.
- **`status_sys`** ([`status.odin`]): A packed pool tracking active buffs, debuffs, stack counts, and tick damage timers attached to entities.

---

## Rendering Pipeline & Radix Sort (`src/engine/draw.odin`)

[`src/engine/draw.odin`] implements the batching and rendering backend.

### The Draw Union
Every draw call is represented by a compact 40-byte union:
```odin
draw :: struct {
    sorting: u64,
    type: enum u8 { TEXTURE, BOX, RECTANGLE },
    using _: struct #raw_union {
        texture:   struct { idx: u32, src, dest: [4]f32 },
        box:       struct { rect: [4]f32, color: [4]u8 },
        rectangle: struct { rect: [4]f32, thickness: f32, color: [4]u8 },
    },
}
```

### 8-Pass LSD Radix Sort (`draw_present`)
Instead of an expensive comparison sort, Swarm uses a **Least Significant Digit (LSD) Radix Sort**:
1. **Pass 1**: 8-bit bucket sort by `.type` (`TEXTURE`, `BOX`, `RECTANGLE`).
2. **Passes 2..9**: 8 passes of 8 bits each across the entire 64-bit `.sorting` key (shifts of $0, 8, 16, \dots, 56$).
3. Two draw buffers (`draw_sys.buffer[0]` and `draw_sys.buffer[1]`) are ping-ponged during passes.
4. Completes in **$O(N)$ linear time** with zero heap allocations.
5. The resulting buffer is iterated and rendered through SDL3 (`RenderTexture`, `RenderFillRect`, `RenderFillRects`).

### Procedural Missing Texture Stub
At startup, `texture_sys_init()` procedurally generates a 32x32 magenta/black checkerboard texture at slot `0`. Any failed texture load or invalid index automatically falls back to this stub rather than crashing.

---