cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME rp2040gnuboy)

get_filename_component(3RD_PARTY_DIR ./3rd-party ABSOLUTE)

include(sys/pico/sdk_import.cmake)

project(${TARGET_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

set(PLATFORM_IMPL sys/pico/video.cpp sys/pico/input.cpp sys/pico/rp2040.cpp sys/dummy/nosound.c sys/dummy/io.c)

# #If Linux / UNIX / BSD / APPLE / etc...
# if(UNIX OR APPLE) 
#         set(PLATFORM_IMPL sys/nix/io_network.c sys/nix/io_none.c sys/nix/io_pipe.c sys/nix/io.c sys/nix/nix.c)
# #If not unix probably windows?
# else() 
#         set(PLATFORM_IMPL sys/windows/windows.c sys/dummy/io.c)
#         add_definitions(-DALT_PATH_SEP)
# endif()

# set(CMAKE_C_STANDARD 99)

# set(CMAKE_C_FLAGS "-Wall")

# set(CMAKE_C_FLAGS_RELEASE "-O3")

# set(CMAKE_C_FLAGS_DEBUG "-g3")

include_directories(./include)
include_directories(./lib)
include_directories(./)
include_directories("/usr/local/include/")

# include_directories(${SDL2_INCLUDE_DIR})

# add_definitions(-DIS_LITTLE_ENDIAN -D_GNU_SOURCE=1 -D_REENTRANT)
add_definitions(-DGNUBOY_NO_MINIZIP -DGNUBOY_NO_SCREENSHOT -DIS_LITTLE_ENDIAN)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/sys/pico pico)

add_compile_definitions(${TARGET_NAME} PICO)

add_executable(${TARGET_NAME}
        ${PLATFORM_IMPL}
        ./assets/rom.c
        ./src/lcd.c ./src/refresh.c
        ./src/lcdc.c ./src/cpu.c ./src/mem.c ./src/rtc.c ./src/hw.c ./src/sound.c
        ./src/events.c ./src/keytable.c ./src/loader.c ./src/save.c ./src/io.c
        ./src/emu.c ./src/main.c ./src/rccmds.c ./src/rckeys.c ./src/rcvars.c ./src/exports.c
        ./src/split.c)
        

# target_link_libraries(${TARGET_NAME} pico_stdlib)

# target_link_libraries(${TARGET_NAME} ${SDL2_LIBRARIES})
target_link_libraries(${TARGET_NAME} PicoRail)

pico_enable_stdio_usb(${TARGET_NAME} 1)

pico_add_extra_outputs(${TARGET_NAME})
