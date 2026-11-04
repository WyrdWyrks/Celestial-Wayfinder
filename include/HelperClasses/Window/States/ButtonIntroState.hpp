#pragma once

#include <functional>
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "TextDrawCommand.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // ButtonIntroState
    // -------------------------------------------------------------------------
    // Shows "Press" in all four corners. Each corner label clears when its
    // button is pressed; once all four have been pressed onComplete fires.

    class ButtonIntroState : public WindowState
    {
    public:
        ButtonIntroState()
        {
            for (uint8_t id : BUTTONS)
            {
                bindInput(id, [this, id](const InputContext &) { _press(id); });
            }
        }

        void setOnComplete(std::function<void()> fn) { _onComplete = std::move(fn); }

        void onEnter(const StateTransferData &) override
        {
            _pressed = 0;
            for (uint8_t id : BUTTONS)
            {
                bindInput(id, "Press");
            }

            clearDrawCommands();
            uint8_t mid = Utilities::selectBottomTextLine() / 2;
            const char *lines[] = { "Each corner label", "matches the button", "nearest to it.", "", "Press all four." };
            for (uint8_t i = 0; i < 5; ++i)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    lines[i], TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(mid - 2 + i) }));
            }
        }

    private:
        static constexpr uint8_t BUTTONS[] = { InputID::BUTTON_1, InputID::BUTTON_2, InputID::BUTTON_3, InputID::BUTTON_4 };
        static constexpr uint8_t ALL_PRESSED = (1 << InputID::BUTTON_1) | (1 << InputID::BUTTON_2) |
                                               (1 << InputID::BUTTON_3) | (1 << InputID::BUTTON_4);

        uint8_t               _pressed = 0;
        std::function<void()> _onComplete;

        void _press(uint8_t id)
        {
            _pressed |= (1 << id);
            bindInput(id, ""); // clears the corner label, keeps the action

            if (_pressed == ALL_PRESSED && _onComplete)
            {
                _onComplete();
            }
        }
    };

} // namespace DisplayModule
