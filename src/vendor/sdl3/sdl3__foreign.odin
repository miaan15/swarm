package sdl3

when ODIN_OS == .Windows {
	@(export) foreign import lib { "../../../vendor/build/SDL3/SDL3.lib" }
} else {
	@(export) foreign import lib { "../../../vendor/build/SDL3/libSDL3.a" }
}
