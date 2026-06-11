# CAVAE — Royal Heart Kinetic Sculpture
# CMakeLists.txt — Dual-board build configuration
#
# Build for F28379D LaunchPad:
#   cmake -DCAVAE_BOARD=F28379D -DCMAKE_TOOLCHAIN_FILE=../toolchain-c2000.cmake ..
#
# Build for F28P55x LaunchPad:
#   cmake -DCAVAE_BOARD=F28P55X -DCMAKE_TOOLCHAIN_FILE=../toolchain-c2000.cmake ..

cmake_minimum_required(VERSION 3.20)
project(cavae_royal_heart CXX C ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Board selection ─────────────────────────────────────────────
set(CAVAE_BOARD "F28379D" CACHE STRING "Target board: F28379D or F28P55X")
set_property(CACHE CAVAE_BOARD PROPERTY STRINGS "F28379D" "F28P55X")

if(CAVAE_BOARD STREQUAL "F28379D")
    message(STATUS "CAVAE target: LAUNCHXL-F28379D (200 MHz, SCI-A)")
    set(BOARD_DEFINE   CAVAE_BOARD_F28379D)
    set(DEVICE_DEFINE  _LAUNCHXL_F28379D)
    set(C2000_DEVICE   F28379D)

elseif(CAVAE_BOARD STREQUAL "F28P55X")
    message(STATUS "CAVAE target: LAUNCHXL-F28P55X (150 MHz, LIN-A)")
    set(BOARD_DEFINE   CAVAE_BOARD_F28P55X)
    set(DEVICE_DEFINE  _LAUNCHXL_F28P55X)
    set(C2000_DEVICE   F28P55X)

else()
    message(FATAL_ERROR "Unknown CAVAE_BOARD '${CAVAE_BOARD}'. Use F28379D or F28P55X.")
endif()

# ─── Toolchain flags ─────────────────────────────────────────────
add_compile_options(
    --silicon_version=28
    --large_memory_model
    --unified_memory
    --float_support=fpu32
    --include_path=${CMAKE_SOURCE_DIR}/FreeRTOS/include
    --include_path=${CMAKE_SOURCE_DIR}/FreeRTOS/portable/CCS/C2000_C28x
    --include_path=${CMAKE_SOURCE_DIR}/include
    --include_path=${CMAKE_SOURCE_DIR}/../cpp
    --define=${BOARD_DEFINE}
    --define=${DEVICE_DEFINE}
    --define=CPU1
    --diag_warning=225
    --display_error_number
    --abi=eabi
    -O2
)

# ─── FreeRTOS kernel sources ─────────────────────────────────────
set(FREERTOS_SOURCES
    FreeRTOS/tasks.c
    FreeRTOS/queue.c
    FreeRTOS/list.c
    FreeRTOS/timers.c
    FreeRTOS/event_groups.c
    FreeRTOS/stream_buffer.c
    FreeRTOS/portable/MemMang/heap_4.c
    FreeRTOS/portable/CCS/C2000_C28x/port.c
    FreeRTOS/portable/CCS/C2000_C28x/portasm.asm
)

# ─── Application sources ─────────────────────────────────────────
set(APP_SOURCES
    main.cpp
)

set(EXE_NAME "cavae_${C2000_DEVICE}")
add_executable(${EXE_NAME} ${FREERTOS_SOURCES} ${APP_SOURCES})

target_include_directories(${EXE_NAME} PRIVATE
    FreeRTOS/include
    FreeRTOS/portable/CCS/C2000_C28x
    include
    ../cpp
)

# ─── Linker ──────────────────────────────────────────────────────
set_target_properties(${EXE_NAME} PROPERTIES
    LINK_FLAGS "--map_file=${EXE_NAME}.map \
                --stack_size=0x400 \
                --heap_size=0x800 \
                -l rts2800_fpu32_fast_supplement.lib"
)
