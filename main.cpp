/*
 * CAVAE — Royal Heart Kinetic Sculpture
 * main.cpp (FreeRTOS) — Kernel startup, task creation, ISR registration
 *
 * Board-agnostic: compile with either
 *   -DCAVAE_BOARD_F28379D    (LAUNCHXL-F28379D, 200 MHz, SCI-A)
 *   -DCAVAE_BOARD_F28P55X    (LAUNCHXL-F28P55X, 150 MHz, LIN-A)
 *
 * All board differences are isolated to board_select.hpp and the
 * CAVAE_UART_* / CAVAE_UART_CLK_ENABLE macros it provides.
 *
 * Task map:
 *  ┌──────────────────┬──────────┬──────────┬────────────────────────┐
 *  │ Task             │ Priority │ Period   │ Trigger                │
 *  ├──────────────────┼──────────┼──────────┼────────────────────────┤
 *  │ FaultTask        │    7     │ event    │ gFaultQueue            │
 *  │ ControlTask      │    6     │ 50 µs    │ gControlSemaphore      │
 *  │ CommandTask      │    5     │ event    │ gCommandQueue          │
 *  │ TelemetryTask    │    4     │ 10 ms    │ vTaskDelayUntil        │
 *  └──────────────────┴──────────┴──────────┴────────────────────────┘
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "F28x_Project.h"

#include "../cpp/board_select.hpp"
#include "../cpp/config.hpp"
#include "../cpp/heart_controller.hpp"
#include "../cpp/telemetry.hpp"

#include "src/control_task.hpp"
#include "src/telemetry_task.hpp"
#include "src/command_task.hpp"
#include "src/fault_task.hpp"
#include "include/rtos_config.hpp"

// ─── Shared RTOS handles ─────────────────────────────────────────
namespace cavae::rtos {
    SemaphoreHandle_t gControlSemaphore = nullptr;
    QueueHandle_t     gCommandQueue     = nullptr;
    QueueHandle_t     gFaultQueue       = nullptr;
}

// ─── Application singletons ──────────────────────────────────────
static cavae::HeartController gHeart;

static cavae::tasks::ControlTask*   gpControlTask   = nullptr;
static cavae::tasks::TelemetryTask* gpTelemetryTask = nullptr;
static cavae::tasks::CommandTask*   gpCommandTask   = nullptr;
static cavae::tasks::FaultTask*     gpFaultTask     = nullptr;

static TaskHandle_t hControl   = nullptr;
static TaskHandle_t hTelemetry = nullptr;
static TaskHandle_t hCommand   = nullptr;
static TaskHandle_t hFault     = nullptr;

// Static task/queue/semaphore storage (no heap)
static StaticTask_t      xControlTCB,    xTelemetryTCB,    xCommandTCB,    xFaultTCB;
static StackType_t       xControlStack  [cavae::rtos::kStackControl];
static StackType_t       xTelemetryStack[cavae::rtos::kStackTelemetry];
static StackType_t       xCommandStack  [cavae::rtos::kStackCommand];
static StackType_t       xFaultStack    [cavae::rtos::kStackFault];
static StaticSemaphore_t xControlSemStorage;
static StaticQueue_t     xCmdQStorage, xFaultQStorage;
static uint8_t           xCmdQBuf  [cavae::rtos::kCmdQueueDepth   * sizeof(uint8_t)];
static uint8_t           xFaultQBuf[cavae::rtos::kFaultQueueDepth * sizeof(cavae::rtos::FaultCode)];

// ─── Hardware init (board-agnostic via macros) ───────────────────
namespace {

void initClocks() noexcept {
    using namespace cavae::board::gpio;

    // ePWM1
    CpuSysRegs.PCLKCR2.bit.EPWM1  = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM2  = 1;
    // eQEP1
    CpuSysRegs.PCLKCR19.bit.EQEP1 = 1;
    // UART peripheral — different register on each board
    CAVAE_UART_CLK_ENABLE();

    // GPIO mux — same peripheral numbers, board header provides pin numbers
    GPIO_SetupPinMux(kPwm1A,    GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kPwm1B,    GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kQep1A,    GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kQep1B,    GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kQep1I,    GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kUartTx,   GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kUartRx,   GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(kFaultLed, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(kFaultLed, GPIO_OUTPUT, GPIO_PUSHPULL);
}

void initPwm() noexcept {
    using namespace cavae::board;
    EPwm1Regs.TBPRD               = kPwmPeriod;
    EPwm1Regs.TBCTL.bit.CTRMODE   = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.PHSEN     = TB_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV    = TB_DIV1;
    EPwm1Regs.CMPA.bit.CMPA       = kPwmPeriod / 2;
    EPwm1Regs.CMPB.bit.CMPB       = kPwmPeriod / 2;
    EPwm1Regs.AQCTLA.bit.CAU      = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAD      = AQ_CLEAR;
    EPwm1Regs.AQCTLB.bit.CBU      = AQ_CLEAR;
    EPwm1Regs.AQCTLB.bit.CBD      = AQ_SET;
    EPwm1Regs.DBCTL.bit.OUT_MODE  = DB_FULL_ENABLE;
    EPwm1Regs.DBCTL.bit.POLSEL    = DB_ACTV_HIC;
    EPwm1Regs.DBFED.bit.DBFED     = kDeadbandCounts;  // board-specific count
    EPwm1Regs.DBRED.bit.DBRED     = kDeadbandCounts;
    EPwm1Regs.ETSEL.bit.INTEN     = 1;
    EPwm1Regs.ETSEL.bit.INTSEL    = ET_CTR_ZERO;
    EPwm1Regs.ETPS.bit.INTPRD     = ET_1ST;
}

void initQep() noexcept {
    using namespace cavae::config;
    EQep1Regs.QPOSMAX              = kEncoderCPR - 1;
    EQep1Regs.QPOSINIT             = 0;
    EQep1Regs.QEPCTL.bit.FREE_SOFT = 2;
    EQep1Regs.QEPCTL.bit.PCRM     = QEP_MAX_RESET;
    EQep1Regs.QEPCTL.bit.QPEN     = 1;
    EQep1Regs.QDECCTL.bit.QSRC    = QDEC_QEP;
}

void initUart() noexcept {
    // CAVAE_UART_INIT() expands to the correct SCI-A or LIN-A sequence
    CAVAE_UART_INIT();
    CAVAE_UART_PIE_IER_BIT = 1;
    IER |= CAVAE_UART_IER_GROUP;
}

} // anonymous namespace

// ─── ISRs (extern "C" for PIE vector table) ──────────────────────
extern "C" {

interrupt void epwm1Isr() {
    BaseType_t xWoken = pdFALSE;
    xSemaphoreGiveFromISR(cavae::rtos::gControlSemaphore, &xWoken);
    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all  = PIEACK_GROUP3;
    portYIELD_FROM_ISR(xWoken);
}

interrupt void uartRxIsr() {
    BaseType_t xWoken = pdFALSE;
    const uint8_t byte = static_cast<uint8_t>(CAVAE_UART_RXBUF & 0xFFu);
    xQueueSendToBackFromISR(cavae::rtos::gCommandQueue, &byte, &xWoken);
    CAVAE_UART_SWRESET_CLR();
    CAVAE_UART_SWRESET_SET();
    PieCtrlRegs.PIEACK.all = CAVAE_UART_PIE_ACK;
    portYIELD_FROM_ISR(xWoken);
}

} // extern "C"

// ─── FreeRTOS hooks ──────────────────────────────────────────────
extern "C" {

void vApplicationMallocFailedHook() {
    taskDISABLE_INTERRUPTS();
    GpioDataRegs.GPASET.bit.GPIO34 = 1;
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t, char*) {
    taskDISABLE_INTERRUPTS();
    GpioDataRegs.GPASET.bit.GPIO34 = 1;
    for (;;) {}
}

void vApplicationGetIdleTaskMemory(StaticTask_t** ppxTCB,
                                   StackType_t**  ppxStack,
                                   uint32_t*      pulSize) {
    static StaticTask_t xIdleTCB;
    static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];
    *ppxTCB   = &xIdleTCB;
    *ppxStack = xIdleStack;
    *pulSize  = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTCB,
                                    StackType_t**  ppxStack,
                                    uint32_t*      pulSize) {
    static StaticTask_t xTimerTCB;
    static StackType_t  xTimerStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTCB   = &xTimerTCB;
    *ppxStack = xTimerStack;
    *pulSize  = configTIMER_TASK_STACK_DEPTH;
}

#ifdef DEBUG
void vAssertCalled(const char* file, int line) {
    taskDISABLE_INTERRUPTS();
    char buf[80];
    snprintf(buf, sizeof(buf), "[ASSERT] %s:%d\r\n", file, line);
    cavae::Telemetry::log(buf);
    for (;;) {}
}
#endif

} // extern "C"

// ─── Main ────────────────────────────────────────────────────────
int main() {
    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    EALLOW;
    PieVectTable.EPWM1_INT       = &epwm1Isr;
    CAVAE_UART_PIE_VECTOR        = &uartRxIsr;  // SCI-A or LIN-A, per board
    EDIS;

    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    IER |= M_INT3;

    initClocks();
    initPwm();
    initQep();
    initUart();

    EINT;
    ERTM;

    // ── RTOS primitives (all static) ─────────────────────────────
    cavae::rtos::gControlSemaphore =
        xSemaphoreCreateBinaryStatic(&xControlSemStorage);
    cavae::rtos::gCommandQueue =
        xQueueCreateStatic(cavae::rtos::kCmdQueueDepth,  sizeof(uint8_t),
                           xCmdQBuf, &xCmdQStorage);
    cavae::rtos::gFaultQueue =
        xQueueCreateStatic(cavae::rtos::kFaultQueueDepth,
                           sizeof(cavae::rtos::FaultCode),
                           xFaultQBuf, &xFaultQStorage);

    configASSERT(cavae::rtos::gControlSemaphore);
    configASSERT(cavae::rtos::gCommandQueue);
    configASSERT(cavae::rtos::gFaultQueue);

    // ── Task objects ─────────────────────────────────────────────
    static cavae::tasks::ControlTask   controlTask(gHeart);
    static cavae::tasks::TelemetryTask telemetryTask(gHeart);
    static cavae::tasks::CommandTask   commandTask(gHeart);
    gpControlTask   = &controlTask;
    gpTelemetryTask = &telemetryTask;
    gpCommandTask   = &commandTask;

    hControl = xTaskCreateStatic(
        cavae::tasks::ControlTask::launch,   "Control",
        cavae::rtos::kStackControl,  gpControlTask,
        cavae::rtos::kPrioControl,   xControlStack,  &xControlTCB);

    hTelemetry = xTaskCreateStatic(
        cavae::tasks::TelemetryTask::launch, "Telemetry",
        cavae::rtos::kStackTelemetry, gpTelemetryTask,
        cavae::rtos::kPrioTelemetry,  xTelemetryStack, &xTelemetryTCB);

    hCommand = xTaskCreateStatic(
        cavae::tasks::CommandTask::launch,   "Command",
        cavae::rtos::kStackCommand,  gpCommandTask,
        cavae::rtos::kPrioCommand,   xCommandStack,  &xCommandTCB);

    static cavae::tasks::FaultTask faultTask(gHeart, hControl, hTelemetry, hCommand);
    gpFaultTask = &faultTask;

    hFault = xTaskCreateStatic(
        cavae::tasks::FaultTask::launch,     "Fault",
        cavae::rtos::kStackFault,    gpFaultTask,
        cavae::rtos::kPrioFault,     xFaultStack,    &xFaultTCB);

    configASSERT(hControl);
    configASSERT(hTelemetry);
    configASSERT(hCommand);
    configASSERT(hFault);

    // ── Boot log includes board name from constexpr ───────────────
    cavae::Telemetry::log("[BOOT] CAVAE Royal Heart — FreeRTOS\r\n[BOOT] Board: ");
    cavae::Telemetry::log(cavae::board::kBoardName);
    cavae::Telemetry::log("\r\n");

    vTaskStartScheduler();
    for (;;) {}
}
