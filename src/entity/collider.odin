package entity

collider :: struct {
    idx: u32,

    rect: [4]f32,

    ett_owner: u32,
    ett_links: [2]u32,

    ett_offset: [2]f32,
    ett_size: [2]f32,
}

collider_sys : struct {
    slot_pool: [^]u32,
    list: [^]collider,
    cap, head, max_idx, len: u32,
} = {}
