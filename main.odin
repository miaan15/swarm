package main

import "core:c/libc"
import "core:fmt"
import "core:strings"
import "src"

BUILD_VENDORS :: #config(BUILD_VENDORS, true)

run_command :: proc(cmd: string, desc: string) {
    fmt.printf("%s...\n", desc)

    c_cmd := strings.clone_to_cstring(cmd, context.temp_allocator)
    exit_code := libc.system(c_cmd)

    if exit_code != 0 {
        fmt.eprintf("[ERROR] Step '%s' failed with exit code: %d\n", desc, exit_code)
        libc.exit(exit_code)
    }

    fmt.printf("[OK] %s finished successfully.\n\n", desc)
}

build_vendors :: proc() {
    // SDL3
    run_command(
        "cmake -S vendor/SDL3/ -B vendor/build/SDL3/ -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DSDL_TESTS=OFF",
        "Configuring SDL3",
    )
    run_command(
        "cmake --build vendor/build/SDL3/ --config Release",
        "Compiling SDL3",
    )

    // SDL3_image
    run_command(
        "cmake -S vendor/SDL3_image/ -B vendor/build/SDL3_image/ -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DSDLIMAGE_TESTS=OFF -DSDLIMAGE_SAMPLES=OFF",
        "Configuring SDL3_image",
    )
    run_command(
        "cmake --build vendor/build/SDL3_image/ --config Release",
        "Compiling SDL3_image",
    )
}

main :: proc() {
    when BUILD_VENDORS {
        build_vendors()
    }

    src.main_run()
}
