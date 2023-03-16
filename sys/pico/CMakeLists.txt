set(CMAKE_C_STANDARD 11)

get_filename_component(3RD_PARTY_DIR ../3rd-party ABSOLUTE)

# Initialise the Pico SDK and extras
include (pico_sdk_import.cmake)
include (pico_extras_import.cmake)
pico_sdk_init()

# pull in common dependencies

add_library(PicoRail INTERFACE)
target_sources(PicoRail INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/../../3rd-party/fatfs/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/../../3rd-party/fatfs/ffunicode.c

    # ${CMAKE_CURRENT_LIST_DIR}/audio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/video.cpp
    ${CMAKE_CURRENT_LIST_DIR}/file.cpp
    ${CMAKE_CURRENT_LIST_DIR}/input.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/storage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/st7789.cpp
    ${CMAKE_CURRENT_LIST_DIR}/usb.cpp
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
)

pico_generate_pio_header(PicoRail ${CMAKE_CURRENT_LIST_DIR}/st7789.pio)

# target_link_libraries(PicoRail INTERFACE hardware_dma hardware_pio hardware_pwm hardware_spi pico_stdlib)
target_link_libraries(PicoRail INTERFACE hardware_dma hardware_pio hardware_pwm hardware_spi pico_stdlib pico_unique_id tinyusb_device)
target_include_directories(PicoRail INTERFACE
    ${CMAKE_CURRENT_LIST_DIR} # for tusb_config
    ${CMAKE_CURRENT_LIST_DIR}/../../3rd-party/fatfs
)

# target_link_libraries(PicoRail INTERFACE pico_audio_i2s)
target_compile_definitions(PicoRail INTERFACE
    # AUDIO_I2S
    DISPLAY_ST7789
    INPUT_GPIO
)

# target_link_libraries(main PicoRail)

# create map/bin/hex file etc.
# pico_add_extra_outputs(main)

