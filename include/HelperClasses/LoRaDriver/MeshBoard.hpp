#pragma once

#include <Arduino.h>
#include <MeshCore.h>   // mesh::MainBoard, BD_STARTUP_NORMAL

// Minimal mesh::MainBoard for the Wayfinder hardware. MeshCore's own
// helpers/ESP32Board.cpp is excluded from the build (it pulls a per-variant
// target.h we don't ship — see meshcore_lib_filter.py), and we only need the
// handful of hooks the routing engine actually calls:
//
//   * getBattMilliVolts()   - only used for advert/telemetry app-data, which
//                             this broadcast-only setup never sends. Returns 0.
//   * getManufacturerName() - debug string only.
//   * reboot()              - used by MeshCore's watchdog / OTA paths.
//   * getStartupReason()    - always NORMAL here; we don't wake-on-packet.
//
// onBeforeTransmit / onAfterTransmit stay no-ops: TX power and any RF switch on
// V3 are handled inside RadioLib, not by a board GPIO.
class MeshBoard : public mesh::MainBoard
{
public:
    uint16_t getBattMilliVolts() override { return 0; }

    const char* getManufacturerName() const override { return "Celestial Wayfinder"; }

    void reboot() override { esp_restart(); }

    uint8_t getStartupReason() const override { return BD_STARTUP_NORMAL; }
};
