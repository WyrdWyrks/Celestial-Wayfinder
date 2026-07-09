#pragma once

#include "Window.hpp"
#include "States/CompassDebugState.hpp"
#include "States/CompassCalibrateState.hpp"
#include "States/TextDisplayState.hpp"
#include "DisplayUtilities.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // CompassWindow
    // -------------------------------------------------------------------------
    // Three-state compass flow:
    //
    //   CompassDebugState (live azimuth, 100 ms refresh)
    //     ↓ BUTTON_4 ("Calibrate?")
    //   TextDisplayState (prompt — "Start calibration?")
    //     ↓ BUTTON_4 ("Yes")     → CompassCalibrateState (30 ms timed calibration)
    //     ↓ BUTTON_3 ("No/Back") → back to CompassDebugState
    //
    // BUTTON_3 in CompassDebugState pops the window entirely.
    //
    // Usage:
    //   Utilities::pushWindow(std::make_shared<CompassWindow>());

    class CompassWindow : public Window
    {
    public:
        CompassWindow()
        {
            // Allocate states
            _debugState     = std::make_shared<CompassDebugState>();
            _calibrateState = std::make_shared<CompassCalibrateState>();

            // Calibration-prompt text state
            _promptState = std::make_shared<TextDisplayState>();
            {
                std::vector<TextDrawData> promptText;
                promptText.emplace_back("Calibration:", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 1 });
                promptText.emplace_back("Rotate the device", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 2 });
                promptText.emplace_back("slowly in all axes", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 });
                promptText.emplace_back("until numbers", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 4 });
                promptText.emplace_back("stabilize", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 5 });
                _promptState->setLines(promptText);
            }

            // Wire adjacent state references
            _debugState->setAdjacentState(InputID::BUTTON_4, _promptState);
            _promptState->setAdjacentState(InputID::BUTTON_4, _calibrateState);

            _debugState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx)
            {
                Utilities::popWindow();
            });
            _debugState->bindInput(InputID::BUTTON_4, "Calibrate", [this](const InputContext &ctx)
            {
                pushState(_promptState);
            });

            _promptState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx)
            {
                popState();
            });
            _promptState->bindInput(InputID::BUTTON_4, "Start", [this](const InputContext &ctx)
            {
                pushState(_calibrateState);
            });

            _calibrateState->bindInput(InputID::BUTTON_3, "Done", [this](const InputContext &ctx)
            {
                popState();
            });

            setInitialState(_debugState);
        }

    private:
        std::shared_ptr<CompassDebugState>     _debugState;
        std::shared_ptr<TextDisplayState>      _promptState;
        std::shared_ptr<CompassCalibrateState> _calibrateState;
    };

} // namespace DisplayModule
