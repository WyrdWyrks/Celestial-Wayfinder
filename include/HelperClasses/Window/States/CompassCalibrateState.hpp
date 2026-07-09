#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "TextDrawCommand.hpp"
#include "NavigationUtils.h"
#include "FilesystemUtils.h"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // CompassCalibrateState
    // -------------------------------------------------------------------------
    // Runs a compass calibration routine until the user exits manually.
    //
    // Behaviour:
    //   - onEnter: starts NavigationUtils calibration.
    //   - refreshIntervalMs: returns REFRESH_RATE_MS so the display ticks.
    //   - Each tick (via ContentLayer redraw): iterates calibration and
    //     redraws the live X/Y/Z min/max calibration ranges so the user can
    //     watch the values converge.
    //   - onExit: ends calibration, saves data to SPIFFS via FilesystemModule.
    //
    // The owning Window must wire BUTTON_3 (Cancel) to pop the state — there
    // is no automatic timeout; the user decides when calibration is complete.
    //
    // Draw update strategy:
    //   Because refreshIntervalMs() returns a non-zero value, Utilities will
    //   call Utilities::render() on a timer rather than only on input.  Each
    //   tick iterates the calibration and rebuilds the draw commands with the
    //   latest calibration data (mirrors CompassDebugState's readout).
    //
    //   The owning Window (or Manager) should call onTick() each time the
    //   display refresh fires.  Alternatively, override this class and update
    //   within a custom draw command.
    //
    // Note: NavigationUtils and FilesystemModule are both part of this
    // library.  LED feedback is intentionally omitted here — subclasses or
    // the owning application can add patterns via onEnter/onExit hooks.

    class CompassCalibrateState : public WindowState
    {
    public:
        static constexpr uint32_t REFRESH_RATE_MS   = 30;

        CompassCalibrateState()
        {
            bindInput(InputID::BUTTON_3, "Cancel");
            refreshIntervalMs = REFRESH_RATE_MS;
        }

        // ------------------------------------------------------------------
        // Lifecycle
        // ------------------------------------------------------------------

        void onEnter(const StateTransferData &) override
        {
            NavigationUtils::BeginCalibration();
            _rebuildDrawCommands();
        }

        void onExit() override
        {
            NavigationUtils::EndCalibration();

            // Persist calibration
            ArduinoJson::JsonDocument calibDoc;
            NavigationUtils::GetCalibrationData(calibDoc);
            FilesystemModule::Utilities::WriteFile(
                NavigationUtils::GetCalibrationFilename(), calibDoc);

            WindowState::onExit(); // clears draw commands
        }
        
        // ------------------------------------------------------------------
        // Per-tick update — called automatically by Window::onTick()
        // ------------------------------------------------------------------

        void onTick() override
        {
            NavigationUtils::IterateCalibration();
            _rebuildDrawCommands();
        }

    private:
        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            auto displayLine  = 1;
            auto displayLines = Utilities::selectBottomTextLine();

            // "Calibrating..." label
            addDrawCommand(std::make_shared<TextDrawCommand>(
                "Calibrating...",
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(displayLine) }
            ));

            if (displayLines < 4)
            {
                return; // not enough lines for the header + X/Y/Z rows
            }

            JsonDocument calibrationData;
            NavigationUtils::GetCalibrationData(calibrationData);

            std::string xCalStr = formatCalRow("X", calibrationData["xMin"].as<float>(), calibrationData["xMax"].as<float>());
            displayLine++;
            addDrawCommand(std::make_shared<TextDrawCommand>(
                xCalStr,
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(displayLine) }
            ));

            std::string yCalStr = formatCalRow("Y", calibrationData["yMin"].as<float>(), calibrationData["yMax"].as<float>());
            displayLine++;
            addDrawCommand(std::make_shared<TextDrawCommand>(
                yCalStr,
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(displayLine) }
            ));

            std::string zCalStr = formatCalRow("Z", calibrationData["zMin"].as<float>(), calibrationData["zMax"].as<float>());
            displayLine++;
            addDrawCommand(std::make_shared<TextDrawCommand>(
                zCalStr,
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(displayLine) }
            ));
        }

        static std::string formatCalRow(const char* axis, float minVal, float maxVal) {
            char buf[32];
            for (int prec = 2; prec >= 0; --prec) {
                snprintf(buf, sizeof(buf), "%s: [%.*f, %.*f]", axis, prec, minVal, prec, maxVal);
                if (strlen(buf) <= 21) break;
            }
            return std::string(buf);
        }
    };

} // namespace DisplayModule
