const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // exe
    const exe = b.addExecutable(.{
        .name = "swarm",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{ },
        }),
    });

    // SDL3
    const SDL3_src_dir = "vendor/SDL3";
    const SDL3_build_dir = "vendor/build/SDL3";

    const SDL3_step = b.step("vendor_SDL3", "Build vendor: SDL3");

    const SDL3_cmake_config = b.addSystemCommand(&.{
        "cmake",
        "-S", b.path(SDL3_src_dir).getPath(b),
        "-B", b.path(SDL3_build_dir).getPath(b),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DSDL_TESTS=OFF", "-DSDL_STATIC=ON", "-DSDL_SHARED=ON",
        "-DSDL_WAYLAND=OFF" // FIXME
    });
    const SDL3_cmake_build = b.addSystemCommand(&.{
        "cmake",
        "--build", b.path(SDL3_build_dir).getPath(b),
        "--config", "Release"
    });
    if (b.build_root.handle.access(b.graph.io, SDL3_build_dir, .{})) |_| {} else |_| {
        SDL3_cmake_build.step.dependOn(&SDL3_cmake_config.step);
    }
    SDL3_step.dependOn(&SDL3_cmake_build.step);
    exe.step.dependOn(SDL3_step);

    exe.root_module.link_libc = true;
    exe.root_module.linkSystemLibrary("SDL3", .{});
    exe.root_module.addLibraryPath(b.path(SDL3_build_dir));

    const SDL3_incl_dir = "include";
    exe.root_module.addIncludePath(b.path(b.fmt("{s}/{s}", .{SDL3_src_dir, SDL3_incl_dir})));

    // final
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
