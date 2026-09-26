cmake_minimum_required(VERSION 3.28)

if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
    message(FATAL_ERROR
        "C++ modules require the 'Ninja' or 'Visual Studio 17.4+' generator.\n"
        "Please run: cmake -B build -GNinja"
    )
endif()

project(swarm LANGUAGES CXX)

#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# vendor
function(build_vendor_lib lib_name)
    get_filename_component(VENDOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${lib_name}" ABSOLUTE)
    get_filename_component(VENDOR_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/build/${lib_name}" ABSOLUTE)

    if(NOT EXISTS "${VENDOR_BUILD_DIR}/CMakeCache.txt")
        message(STATUS "Building vendor: ${lib_name}...")

        file(TO_NATIVE_PATH "${VENDOR_SOURCE_DIR}" NATIVE_SOURCE_DIR)
        file(TO_NATIVE_PATH "${VENDOR_BUILD_DIR}" NATIVE_BUILD_DIR)

        # config
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -S "${NATIVE_SOURCE_DIR}"
                -B "${NATIVE_BUILD_DIR}"
                -DCMAKE_BUILD_TYPE=Release
                ${ARGN}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE CONFIG_RESULT
        )

        if(NOT CONFIG_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to configure vendor target: ${lib_name}")
        endif()

        # build
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build "${NATIVE_BUILD_DIR}" --config Release
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE BUILD_RESULT
        )

        if(NOT BUILD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to build vendor target: ${lib_name}")
        endif()
    else()
        message(STATUS "OK, ${lib_name}")
    endif()
endfunction()

set(SDL3_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/build/SDL3")
set(SDL3_IMAGE_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/build/SDL3_image")

build_vendor_lib(SDL3
    -DBUILD_SHARED_LIBS=OFF
    -DSDL_TESTS=OFF
)
find_package(SDL3 REQUIRED CONFIG
    PATHS "${SDL3_BUILD_DIR}"
    NO_DEFAULT_PATH
)

build_vendor_lib(SDL3_image
    -DBUILD_SHARED_LIBS=OFF
    -DSDLIMAGE_TESTS=OFF
    -DSDLIMAGE_SAMPLES=OFF
    -DCMAKE_PREFIX_PATH=${SDL3_BUILD_DIR}
)
find_package(SDL3_image REQUIRED CONFIG
    PATHS "${SDL3_IMAGE_BUILD_DIR}"
    NO_DEFAULT_PATH
)

# var
file(GLOB_RECURSE ALL_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

get_filename_component(MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" ABSOLUTE)
list(REMOVE_ITEM ALL_SOURCES "${MAIN_SOURCE}")

# target
add_executable(${PROJECT_NAME} "${MAIN_SOURCE}")

target_sources(${PROJECT_NAME} PRIVATE
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    FILES ${ALL_SOURCES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# include
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# link
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
)
