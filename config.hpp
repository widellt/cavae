#pragma once
/*
 * CAVAE — Royal Heart Kinetic Sculpture
 * config.hpp — Application-level constants (board-independent)
 *
 * Hardware constants (clocks, GPIO, UART) now live in board_select.hpp.
 * This file contains only values that are the same on every board.
 *
 * Include board_select.hpp for hardware specifics.
 * Include this file for application tuning parameters.
 */

#include <cstdint>
#include "board_select.hpp"   // pulls in cavae::board + cavae::config aliases

namespace cavae::config {

/*─── Control loop ─────────────────────────────────────────────────*/
inline constexpr float kControlLoopHz  = 10'000.0f;
inline constexpr float kControlLoopDt  = 1.0f / kControlLoopHz;
inline constexpr float kTelemetryHz    = 100.0f;
inline constexpr float kTelemetryUs    = 1'000'000.0f / kTelemetryHz;

/*─── Encoder ──────────────────────────────────────────────────────*/
inline constexpr uint32_t kEncoderCPR  = 4'000;   // counts per revolution
inline constexpr float    kTwoPi       = 6.28318530718f;

/*─── Cam geometry ─────────────────────────────────────────────────*/
inline constexpr float kCamADefault    = 1.00f;   // long  radius (opens doors)
inline constexpr float kCamBDefault    = 0.55f;   // short radius (closes doors)

/*─── Door thresholds ──────────────────────────────────────────────*/
inline constexpr float kDoorClosedThr  = 0.05f;
inline constexpr float kDoorOpenThr    = 0.95f;

/*─── Motor limits ─────────────────────────────────────────────────*/
inline constexpr float kMaxDuty        = 0.92f;
inline constexpr float kMinDuty        = 0.08f;
inline constexpr float kMaxRpm         = 75.0f;

/*─── PI controller gains ──────────────────────────────────────────*/
inline constexpr float kKp             = 0.008f;
inline constexpr float kKi             = 0.002f;
inline constexpr float kIntegralClamp  = 50.0f;

/*─── RPM filter ───────────────────────────────────────────────────*/
inline constexpr float kRpmFilterAlpha = 0.98f;

/*─── Torque estimate ──────────────────────────────────────────────*/
inline constexpr float kTorquePerDuty  = 0.45f;   // Nm / duty (empirical)

} // namespace cavae::config
