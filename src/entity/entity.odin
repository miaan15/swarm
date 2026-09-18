package entity

entity :: struct {
    idx: u32,

    logic_flag: u8,

    z: i8,
    scale: [2]f32,

    pos: [2]f32,
    vel: [2]f32,

    spr_begin, spr_len: u32,
    col_begin, col_len: u32,

    status_begin, status_len: u32,
}

entity_sys : struct {
    slot_pool: [^]u32,
    list: [^]entity,
    cap, head, max_idx, len: u32,
} = {}
