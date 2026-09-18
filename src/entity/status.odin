package entity

status :: struct {
    pool_flag: u32,
    idx: u32,

    type: u32,

    ett_owner: u32,
    ett_links: [2]u32,

    //
    stack_cnt: u32,
    duration_ms, last_time_check_ms: u32,

    damage: f32,
}

status_sys : struct {
    pool: [^]status,
    cap, head, max_idx, len: u32,
} = {}
