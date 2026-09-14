# swarm

A high-performance NPC swarm simulation engine built with **C23** and **Raylib**. Designed to simulate thousands of entities with stateless, hot-reloadable logic and optimized spatial data structures.

---

## Core Architecture

### 1. Memory: The Arena Pattern
The engine avoids `malloc`/`free` during the simulation to prevent fragmentation and latency.
- **`omni_arena`**: Long-lived storage for entity pools, textures, and system-wide state.
- **`tick_arena`**: Double-buffered transient storage. It is reset every simulation tick. Perfect for returning temporary lists (e.g., query results) without cleanup overhead.

### 2. High-Performance ECS
- **SoA (Structure of Arrays)**: `entity_pos_soa` stores `x`, `y`, `vx`, `vy` in blocks of 8. This ensures contiguous memory access for the physics update loop, maximizing cache hits and allowing for future SIMD optimization.
- **Chunk Grid**: A `64x64` spatial grid used for broad-phase culling and neighborhood queries.
- **Dynamic AABB Tree**: Colliders are stored in a self-balancing AABB tree. It uses "Fat AABBs" (inflated by `fat_aabb_offset`) to avoid expensive tree re-insertions when entities move small distances.

### 3. Rendering Pipeline
- **Radix Sorting**: All draw calls are sorted by a 64-bit `meta` key using a multi-pass radix sort.
    - **Bits [63-56]**: Z-index.
    - **Bits [55-24]**: Y-coordinate (mapped from float to sortable uint).
    - This provides automated 2.5D depth sorting (objects lower on screen are drawn in front).
- **Frustum Culling**: The engine automatically culls entities outside the camera view using the spatial chunk grid before issuing draw calls.

### 4. Stateless Hot-Reloading
Simulation logic in `src/handle/` is compiled into `libhandle.so`. The main binary loads these via `dlopen`.
- **Statelessness**: All state (health, position, etc.) is stored in pools in the main binary. The handles only perform calculations on that state.
- **Workflow**: Modify `src/handle/*.c` -> Press **Ctrl+R** in-game -> Logic reloads instantly while the simulation continues.

---

## Engine API

### Entity Management
```c
// Create an entity with a specific position
u32 entity_idx = entity_create(x, y, &entity_ptr);

// Query entities in a rectangular area (results allocated in tick_arena)
u32 *results; usize count;
entity_query(x, y, w, h, &results, &count);
```

### Component Management
Entities are linked to components via index-based linked lists.
```c
entity_add_portrait(entity_idx, profile_idx, &portrait_ptr);
entity_add_collider(entity_idx, &collider_ptr);
entity_add_status(entity_idx, type, &status_ptr);
```

### Rendering
```c
drawer *dr = draw_make();
dr->tex = texture_idx;
dr->dx = x; dr->dy = y;
draw_meta_set_z(&dr->meta, z_layer);
draw_meta_set_y(&dr->meta, y_coord);
```

### Actions (Messaging)
Actions allow entities to interact without direct coupling.
```c
u32 data[3] = { damage_value, 0, 0 };
action_make(from_idx, to_idx, ACTION_DAMAGE, data);
```

---

## Simulation Flow

1. **`game_update()`**: Fixed-tick simulation loop. All physics, AI, and spatial updates happen here. Frame-independent.
2. **`game_visual()`**: Variable-rate rendering loop. Use `tick_frame_alpha` to interpolate between the last two simulation ticks for smooth motion regardless of frame rate.
3. **`draw_present()`**: Finalizes the frame by sorting and executing batch draw calls.

---

## Developer Workflow

### Build Commands
- **Debug Build**: `./do.sh` (Includes AddressSanitizer and UndefinedBehaviorSanitizer).
- **Release Build**: `./do.sh --release` (-O3 optimization).
- **Hot-Reload Lib**: `./do.sh --reload` (Compiles only the shared library).

### Running
- Execute `./build/a`.
- Press **Ctrl + R** to hot-reload logic.
