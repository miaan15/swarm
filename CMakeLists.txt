cmake_minimum_required(VERSION 3.25)
project(a C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang"))
    add_compile_options(-fsanitize=address,undefined -g -O0)
    add_link_options(-fsanitize=address,undefined)
endif()

# vendors
function(build_vendor_lib lib_name)
    set(VENDOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${lib_name}")
    set(VENDOR_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/build/${lib_name}")

    if(NOT EXISTS "${VENDOR_BUILD_DIR}")
        message(STATUS "Building vendor: ${lib_name}...")

        file(TO_NATIVE_PATH "${VENDOR_SOURCE_DIR}" NATIVE_SOURCE_DIR)
        file(TO_NATIVE_PATH "${VENDOR_BUILD_DIR}" NATIVE_BUILD_DIR)

        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -S "${NATIVE_SOURCE_DIR}"
                -B "${NATIVE_BUILD_DIR}"
                -DCMAKE_BUILD_TYPE=Release
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE CONFIG_RESULT
        )

        if(NOT CONFIG_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to configure vendor target: ${lib_name}")
        endif()

        execute_process(
            COMMAND ${CMAKE_COMMAND} --build "${NATIVE_BUILD_DIR}" --config Release
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE BUILD_RESULT
        )

        if(NOT BUILD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to build vendor target: ${lib_name}")
        endif()
    else()
        message(STATUS "Vendor ${lib_name} build exists at ${VENDOR_BUILD_DIR}. Skipping.")
    endif()
endfunction()
#
build_vendor_lib(raylib)

# exe
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# var
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    _EXE_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

# more flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# include
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/raylib/src"
)

# raylid
set(RAYLIB_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/build/raylib/raylib")

find_library(RAYLIB_LIB
    NAMES raylib libraylib.a raylib.lib
    PATHS "${RAYLIB_LIB_DIR}"
    NO_DEFAULT_PATH
)

if(NOT RAYLIB_LIB)
    message(FATAL_ERROR "raylib library not found in ${RAYLIB_LIB_DIR}")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIB})

# more linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32
        gdi32
        winmm
        shell32
        user32
    )
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${OPENGL_LIBRARIES}
        pthread
        m
        "-framework IOKit"
        "-framework Cocoa"
        "-framework CoreVideo"
    )
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        GL
        m
        pthread
        dl
        rt
        X11
    )
endif()