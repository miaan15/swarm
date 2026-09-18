package entity

sprite_profile :: struct {
    tex: u32,
    rect: [4]f32,
}

sprite :: struct {
    idx: u32,

    tex: u32,
    src, dest: [4]f32,

    last_tick_pos: [2]f32,
    interpolate_pos:[2]f32,

    ett_owner: u32,
    ett_links: [2]u32,

    ett_offset: [2]f32,
    ett_z: i8,
}

sprite_sys : struct {
    profile_list: [^]sprite_profile,
    profile_cap, profile_len: u32,

    slot_pool: [^]u32,
    list: [^]sprite,
    cap, head, max_idx, len: u32,
} = {}
