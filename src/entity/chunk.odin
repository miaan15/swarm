package entity

chunk_data :: struct {
    alive: bool,
    pos: [2]f32,
}

chunk_sys : struct {
    hmap: [^]chunk_data,
    cap, len: u32
} = {}
