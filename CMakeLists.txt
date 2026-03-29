cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME STM32H743)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C, and CXX languages
enable_language(C ASM CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LVGL_DIR ${CMAKE_CURRENT_LIST_DIR}/Drivers/LVGL)
file(GLOB_RECURSE LVGL_SOURCES
    "${LVGL_DIR}/src/*.c"               # 核心源码
    "${LVGL_DIR}/porting/*.c" # 你的移植接口 (disp/indev)
    "${LVGL_DIR}/demos/*.c"
    # 如果你有 SquareLine 导出的 UI 文件，也可以把路径加在这里，例如:
    # "${LVGL_DIR}/ui/*.c"
)
file(GLOB_RECURSE UI_SOURCES
    "Drivers/USER/ui/*.c"
    "Drivers/USER/ui/*.cpp"
)
# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Drivers/USER/Src/mysys.c
    Drivers/USER/Src/lcd_fonts.c
    Drivers/USER/Src/lcd_image.c
    Drivers/USER/Src/lcd_rgb.c
    Drivers/USER/Src/lcd_test.c
    Drivers/USER/Src/led.c
    Drivers/USER/Src/openmv_spi.c
    Drivers/USER/Src/openmv_uart.c
    Drivers/USER/Src/sdram.c
    Drivers/USER/Src/usart.c
    Drivers/USER/Src/touch_800x480.c
    Drivers/USER/Src/touch_iic.c
    Core/Src/syscalls_user.c
    Drivers/USER/Src/retarget.c
    ${LVGL_SOURCES}
    ${UI_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Drivers/USER/Inc
    Drivers/USER/ui
    ${LVGL_DIR}                  # 包含 lv_conf.h
    ${LVGL_DIR}/src              # 包含 lvgl.h 及核心头文件
    ${LVGL_DIR}/porting # 包含 lv_port_disp.h 等
    ${LVGL_DIR}/demos
    ${LVGL_DIR}/demos/widgets
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    LV_CONF_INCLUDE_SIMPLE       # 让 LVGL 直接 include "lv_conf.h" 而不是 "../../"
    LV_LVGL_H_INCLUDE_SIMPLE     # 某些组件需要这个
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)


# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
