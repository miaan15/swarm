package entity

action :: struct {
    from, to: u32,
    type: u32,
    data: [3]u32,
}

action_sys : struct {
    buffer: [^]action,
    cap, len: u32,
} = {}
