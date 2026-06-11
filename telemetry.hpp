#pragma once
/*
 * CAVAE — Royal Heart Kinetic Sculpture
 * telemetry.hpp — Board-agnostic JSON serialiser over UART
 *
 * Uses CAVAE_UART_* macros from board_select.hpp so the same code
 * compiles for both F28379D (SciaRegs) and F28P55x (LinaRegs).
 *
 * Output format (100 Hz, ~80 bytes/frame):
 *   {"s":0,"a":1.234,"d":0.45,"r":72.1,"t":0.031,"h":5,"c":2,"ca":1.00,"cb":0.55}
 */

#include <cstdio>
#include <cstdint>
#include "board_select.hpp"
#include "F28x_Project.h"

namespace cavae {

struct TelemetryFrame {
    uint8_t  state;
    float    camAngle;
    float    doorPos;
    float    motorRpm;
    float    torqueEst;
    uint32_t heartbeats;
    uint32_t openCycles;
    float    camA;
    float    camB;
};

class Telemetry {
public:
    // ── Transmit one JSON telemetry frame ────────────────────────
    static void send(const TelemetryFrame& f) noexcept {
        char buf[160];
        const int len = snprintf(buf, sizeof(buf),
            "{\"s\":%u,"
            "\"a\":%.3f,"
            "\"d\":%.3f,"
            "\"r\":%.1f,"
            "\"t\":%.4f,"
            "\"h\":%lu,"
            "\"c\":%lu,"
            "\"ca\":%.2f,"
            "\"cb\":%.2f,"
            "\"board\":\"%s\"}\r\n",
            static_cast<unsigned>(f.state),
            f.camAngle,
            f.doorPos,
            f.motorRpm,
            f.torqueEst,
            static_cast<unsigned long>(f.heartbeats),
            static_cast<unsigned long>(f.openCycles),
            f.camA,
            f.camB,
            cavae::board::kBoardName          // identifies which board is running
        );
        transmit(buf, len);
    }

    // ── Transmit a raw log string ─────────────────────────────────
    static void log(const char* msg) noexcept {
        while (*msg) {
            while (!CAVAE_UART_TXRDY) { /* spin */ }
            CAVAE_UART_TXBUF = static_cast<uint16_t>(*msg++);
        }
    }

private:
    static void transmit(const char* buf, int len) noexcept {
        for (int i = 0; i < len; ++i) {
            while (!CAVAE_UART_TXRDY) { /* spin */ }
            CAVAE_UART_TXBUF = static_cast<uint16_t>(buf[i]);
        }
    }
};

} // namespace cavae
