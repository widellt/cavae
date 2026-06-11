#pragma once
/*
 * CAVAE — Royal Heart Kinetic Sculpture
 * boards/f28p55x.hpp — Board-specific constants for TI C2000 F28P55x LaunchPad
 *
 * LAUNCHXL-F28P55X
 *   CPU:    150 MHz single-core C28x + FPU32 + TMU
 *   Flash:  1088 KB
 *   UART:   LIN-A in SCI-compatibility mode (LinaRegs) — NOT SciaRegs
 *   QEP:    3× eQEP; eQEP1 routed via DIP switch SW3 on LaunchPad
 *           Default QEP pins: GPIO 20/21/23 (same physical mux as F28379D)
 *   PWM:    ePWM1 on GPIO 0/1 (same as F28379D)
 *   LED:    GPIO34 (LED4 on LaunchPad)
 *
 * Key differences from F28379D:
 *   1. SYSCLK 150 MHz (not 200 MHz) — all derived constants change
 *   2. SCI-A replaced by LIN-A in SCI-compat mode (LinaRegs struct)
 *   3. LIN baud rate uses LSPCLK prescaler, not direct SYSCLK
 *   4. Single core — no CPU2, no IPC
 *   5. PCLKCR register bit for LIN: CpuSysRegs.PCLKCR9.bit.LIN_A
 *
 * Do not include directly — include board_select.hpp instead.
 */

#include <cstdint>

namespace cavae::board {

/*─── Identity ─────────────────────────────────────────────────────*/
inline constexpr char kBoardName[]  = "LAUNCHXL-F28P55X";
inline constexpr char kDeviceName[] = "TMS320F28P550SJ";

/*─── Clock ────────────────────────────────────────────────────────*/
inline constexpr float    kSysClkMHz = 150.0f;
inline constexpr float    kSysClkHz  = kSysClkMHz * 1'000'000.0f;

// LIN-A on F28P55x is clocked from LSPCLK = SYSCLK / 4 = 37.5 MHz
inline constexpr float    kLspClkHz  = kSysClkHz / 4.0f;

/*─── PWM ──────────────────────────────────────────────────────────*/
inline constexpr float    kPwmFreqHz      = 20'000.0f;
inline constexpr uint16_t kPwmPeriod      =
    static_cast<uint16_t>(kSysClkHz / (2.0f * kPwmFreqHz));
// Dead-band: 100 ns @ 150 MHz = 15 SYSCLK counts
inline constexpr uint16_t kDeadbandCounts = 15;

/*─── UART (LIN-A in SCI-compatibility mode) ───────────────────────
 *
 * The F28P55x has no dedicated SCI peripheral. Instead, the LIN-A
 * module is placed in SCI-compatibility mode. The register struct is
 * LinaRegs (not SciaRegs), but the bit-field names are identical for
 * the SCI-compatible subset.
 *
 * Baud rate for LIN-SCI mode:
 *   BRR = (LSPCLK / (8 × BAUD)) − 1
 *   At 37.5 MHz LSPCLK, 115200 Bd → BRR ≈ 39
 */
inline constexpr uint32_t kUartBaud = 115'200;
inline constexpr uint32_t kUartBrr  =
    static_cast<uint32_t>(kLspClkHz / (8.0f * kUartBaud)) - 1;

// Clock-enable: LIN-A clock gate is in PCLKCR9 (not PCLKCR7)
#define CAVAE_UART_CLK_ENABLE()   ( CpuSysRegs.PCLKCR9.bit.LIN_A = 1 )

// Register accessors — LinaRegs mirrors SciaRegs field names for SCI-compat
#define CAVAE_UART_TXRDY          ( LinaRegs.SCICTL2.bit.TXRDY )
#define CAVAE_UART_TXBUF          ( LinaRegs.SCITXBUF.all )
#define CAVAE_UART_RXBUF          ( LinaRegs.SCIRXBUF.all )
#define CAVAE_UART_SWRESET_CLR()  ( LinaRegs.SCICTL1.bit.SWRESET = 0 )
#define CAVAE_UART_SWRESET_SET()  ( LinaRegs.SCICTL1.bit.SWRESET = 1 )

// PIE vector and group — LIN-A RX uses same PIE group 9 slot as SCI-A
#define CAVAE_UART_PIE_VECTOR     ( PieVectTable.LINA_RX_INT )
#define CAVAE_UART_PIE_IER_BIT    ( PieCtrlRegs.PIEIER9.bit.INTx1 )
#define CAVAE_UART_IER_GROUP      ( M_INT9 )
#define CAVAE_UART_PIE_ACK        ( PIEACK_GROUP9 )

// LIN-A init in SCI-compatibility mode
// Must set SCIMODE = 1 in SCIGCR0 to enable SCI compat before config
#define CAVAE_UART_INIT()                                          \
    do {                                                           \
        /* Release LIN from reset */                               \
        LinaRegs.SCIGCR0.bit.RESET     = 0;                        \
        LinaRegs.SCIGCR0.bit.RESET     = 1;                        \
        /* Enable SCI-compatibility mode */                        \
        LinaRegs.SCIGCR1.bit.SCIMODE   = 1;                        \
        LinaRegs.SCIGCR1.bit.TXENA     = 1;                        \
        LinaRegs.SCIGCR1.bit.RXENA     = 1;                        \
        LinaRegs.SCIGCR1.bit.STOPBITS  = 0;  /* 1 stop bit */      \
        LinaRegs.SCIGCR1.bit.PARITY    = 0;                        \
        LinaRegs.SCIGCR1.bit.PARITYENA = 0;                        \
        LinaRegs.SCIGCR1.bit.LOOPBACK  = 0;                        \
        /* 8 data bits */                                          \
        LinaRegs.SCIFORMAT.bit.CHAR    = 7;                        \
        /* Baud rate prescaler (BRR uses LSPCLK) */                \
        LinaRegs.BRSR.bit.SCI_LIN_PSL =                           \
            cavae::board::kUartBrr & 0x00FFFFFFu;                  \
        /* Enable RX interrupt */                                   \
        LinaRegs.SCISETINT.bit.SETRXINT = 1;                       \
        /* Bring out of software reset */                          \
        LinaRegs.SCIGCR1.bit.SWnRESET = 1;                        \
    } while(0)

/*─── GPIO pin assignments ─────────────────────────────────────────*/
namespace gpio {
    inline constexpr uint16_t kPwm1A    =  0;   // ePWM1A  (same as F28379D)
    inline constexpr uint16_t kPwm1B    =  1;   // ePWM1B
    // eQEP1 pins — verify DIP switch SW3 position on LAUNCHXL-F28P55X:
    //   SW3-1 OFF, SW3-2 OFF → routes eQEP1 to GPIO 20/21/23
    inline constexpr uint16_t kQep1A    = 20;
    inline constexpr uint16_t kQep1B    = 21;
    inline constexpr uint16_t kQep1I    = 23;
    // LIN-A TX/RX — GPIO 42/43 on F28P55X LaunchPad
    // (GPIO 28/29 used for XDS110 virtual COM — do not use for LIN)
    inline constexpr uint16_t kUartTx   = 42;
    inline constexpr uint16_t kUartRx   = 43;
    inline constexpr uint16_t kFaultLed = 34;   // LED4
} // namespace gpio

/*─── Single-core ──────────────────────────────────────────────────*/
inline constexpr bool kDualCore = false;  // F28P55x is single-core

} // namespace cavae::board
