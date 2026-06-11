#pragma once
/*
 * CAVAE — Royal Heart Kinetic Sculpture
 * boards/f28379d.hpp — Board-specific constants for TI C2000 F28379D LaunchPad
 *
 * LAUNCHXL-F28379D
 *   CPU:    200 MHz dual-core C28x (Delfino)
 *   FPU:    FPU32
 *   Flash:  1 MB
 *   UART:   Dedicated SCI-A peripheral (SciaRegs)
 *   QEP:    eQEP1 on GPIO 20/21/23
 *   PWM:    ePWM1 on GPIO 0/1
 *   LED:    GPIO34 (blue LED D10)
 *
 * Do not include directly — include board_select.hpp instead.
 */

#include <cstdint>

namespace cavae::board {

/*─── Identity ─────────────────────────────────────────────────────*/
inline constexpr char kBoardName[]   = "LAUNCHXL-F28379D";
inline constexpr char kDeviceName[]  = "TMS320F28379D";

/*─── Clock ────────────────────────────────────────────────────────*/
inline constexpr float    kSysClkMHz = 200.0f;
inline constexpr float    kSysClkHz  = kSysClkMHz * 1'000'000.0f;

/*─── PWM ──────────────────────────────────────────────────────────*/
inline constexpr float    kPwmFreqHz      = 20'000.0f;
inline constexpr uint16_t kPwmPeriod      =
    static_cast<uint16_t>(kSysClkHz / (2.0f * kPwmFreqHz));
// Dead-band: 100 ns @ 200 MHz = 20 SYSCLK counts
inline constexpr uint16_t kDeadbandCounts = 20;

/*─── UART ─────────────────────────────────────────────────────────*/
// F28379D uses dedicated SCI-A (SciaRegs)
// BRR = SYSCLK / (8 × BAUD) − 1
inline constexpr uint32_t kUartBaud = 115'200;
inline constexpr uint32_t kUartBrr  =
    static_cast<uint32_t>(kSysClkHz / (8.0f * kUartBaud)) - 1;

// Clock-enable register for the UART peripheral
#define CAVAE_UART_CLK_ENABLE()   ( CpuSysRegs.PCLKCR7.bit.SCI_A = 1 )

// Register accessors used by UartHal
#define CAVAE_UART_TXRDY          ( SciaRegs.SCICTL2.bit.TXRDY )
#define CAVAE_UART_TXBUF          ( SciaRegs.SCITXBUF.all )
#define CAVAE_UART_RXBUF          ( SciaRegs.SCIRXBUF.all )
#define CAVAE_UART_SWRESET_CLR()  ( SciaRegs.SCICTL1.bit.SWRESET = 0 )
#define CAVAE_UART_SWRESET_SET()  ( SciaRegs.SCICTL1.bit.SWRESET = 1 )

// PIE vector and group for RX interrupt
#define CAVAE_UART_PIE_VECTOR     ( PieVectTable.SCIA_RX_INT )
#define CAVAE_UART_PIE_IER_BIT    ( PieCtrlRegs.PIEIER9.bit.INTx1 )
#define CAVAE_UART_IER_GROUP      ( M_INT9 )
#define CAVAE_UART_PIE_ACK        ( PIEACK_GROUP9 )

// SCI-A init sequence (F28379D native SCI)
#define CAVAE_UART_INIT()                                    \
    do {                                                     \
        SciaRegs.SCICCR.bit.SCICHAR   = 7;  /* 8-bit */     \
        SciaRegs.SCICCR.bit.STOPBITS  = 0;  /* 1 stop */    \
        SciaRegs.SCICCR.bit.PARITYENA = 0;  /* no parity */ \
        SciaRegs.SCICTL1.bit.TXENA    = 1;                   \
        SciaRegs.SCICTL1.bit.RXENA    = 1;                   \
        SciaRegs.SCIHBAUD.all = (cavae::board::kUartBrr >> 8) & 0xFFu; \
        SciaRegs.SCILBAUD.all =  cavae::board::kUartBrr       & 0xFFu; \
        SciaRegs.SCICTL1.bit.SWRESET  = 1;                   \
        SciaRegs.SCICTL2.bit.RXBKINTENA = 1;                 \
    } while(0)

/*─── GPIO pin assignments ─────────────────────────────────────────*/
namespace gpio {
    inline constexpr uint16_t kPwm1A    =  0;   // ePWM1A
    inline constexpr uint16_t kPwm1B    =  1;   // ePWM1B
    inline constexpr uint16_t kQep1A    = 20;   // eQEP1A
    inline constexpr uint16_t kQep1B    = 21;   // eQEP1B
    inline constexpr uint16_t kQep1I    = 23;   // eQEP1 index
    inline constexpr uint16_t kUartTx   = 28;   // SCI-A TX
    inline constexpr uint16_t kUartRx   = 29;   // SCI-A RX
    inline constexpr uint16_t kFaultLed = 34;   // Blue LED D10
} // namespace gpio

/*─── Dual-core ────────────────────────────────────────────────────*/
inline constexpr bool kDualCore = true;   // F28379D has CPU1 + CPU2
// This firmware targets CPU1 only; CPU2 is unused for CAVAE

} // namespace cavae::board
