#pragma once
/*
 * CAVAE — Royal Heart Kinetic Sculpture
 * board_select.hpp — Compile-time board selection
 *
 * Include THIS header everywhere (config, telemetry, main).
 * Never include board/f28379d.hpp or boards/f28p55x.hpp directly.
 *
 * ─── How to select a board ────────────────────────────────────────
 *
 * Pass a preprocessor define to the compiler.  In CMakeLists.txt:
 *
 *   # For F28379D LaunchPad:
 *   target_compile_definitions(${PROJECT_NAME} PRIVATE CAVAE_BOARD_F28379D)
 *
 *   # For F28P55x LaunchPad:
 *   target_compile_definitions(${PROJECT_NAME} PRIVATE CAVAE_BOARD_F28P55X)
 *
 * In TI CCS: Project Properties → C/C++ Build → Settings →
 *   C2000 Compiler → Predefined Symbols → add CAVAE_BOARD_F28379D
 *   (or CAVAE_BOARD_F28P55X)
 *
 * ─── What this exposes ────────────────────────────────────────────
 *
 * After including this header, use the cavae::board:: namespace:
 *
 *   cavae::board::kSysClkMHz        — CPU frequency (float)
 *   cavae::board::kPwmPeriod        — ePWM1 period register value
 *   cavae::board::kDeadbandCounts   — dead-band counts for gate driver
 *   cavae::board::kUartBrr          — UART baud-rate register value
 *   cavae::board::kBoardName        — human-readable string
 *   cavae::board::gpio::kPwm1A      — GPIO number for ePWM1A
 *   cavae::board::gpio::kUartTx     — GPIO number for UART TX
 *   ... etc.
 *
 * Hardware macros (defined in each board header):
 *   CAVAE_UART_CLK_ENABLE()  — enable clock gate for UART peripheral
 *   CAVAE_UART_INIT()        — configure UART/LIN registers
 *   CAVAE_UART_TXRDY         — TX ready flag (lvalue)
 *   CAVAE_UART_TXBUF         — TX data register (lvalue)
 *   CAVAE_UART_RXBUF         — RX data register (rvalue)
 *   CAVAE_UART_SWRESET_CLR() — assert  peripheral software reset
 *   CAVAE_UART_SWRESET_SET() — release peripheral software reset
 *   CAVAE_UART_PIE_VECTOR    — PIE vector table entry (lvalue)
 *   CAVAE_UART_PIE_IER_BIT   — PIE IER enable bit (lvalue)
 *   CAVAE_UART_IER_GROUP     — IER group mask
 *   CAVAE_UART_PIE_ACK       — PIE ACK group mask
 */

#if defined(CAVAE_BOARD_F28379D)
    #include "boards/f28379d.hpp"

#elif defined(CAVAE_BOARD_F28P55X)
    #include "boards/f28p55x.hpp"

#else
    #error "No board selected. Define CAVAE_BOARD_F28379D or CAVAE_BOARD_F28P55X \
in your compiler flags.  See firmware/cpp/board_select.hpp for instructions."
#endif

/*
 * Re-export cavae::board into cavae::config for backward compatibility
 * with any code that still uses cavae::config::kSysClkMHz etc.
 */
namespace cavae::config {
    using namespace cavae::board;
} // namespace cavae::config
