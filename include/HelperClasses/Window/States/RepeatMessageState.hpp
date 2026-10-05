#pragma once

#include <string>
#include <vector>
#include <memory>
#include "WindowState.hpp"
#include "TextDrawCommand.hpp"
#include "LoraUtils.h"
#include "HelperClasses/PingMessage.hpp"
#include "NavigationUtils.h"
#include "LedUtilities.hpp"
#include "RingPulse.hpp"

namespace DisplayModule
{
    class RepeatMessageState : public WindowState
    {
    public:
        static constexpr uint32_t MESSAGE_REPEAT_INTERVAL_MS = 15000;
        static constexpr uint32_t LED_ANIMATION_MS = 3000;

        void onEnter(const StateTransferData &data) override
        {
            _message = nullptr;
            _ringPulseID = UxModule::RingPulse::RegisteredPatternID();

            if (data.payload)
            {
                auto &doc = *data.payload;
                auto payloadObj = doc[LoraModule::LoraMessageInterface::KEY_PAYLOAD].as<JsonObject>();
                if (!payloadObj.isNull())
                {
                    auto msg = PingMessage::Create(payloadObj);
                    if (msg)
                    {
                        msg->deserialize(doc);
                        _message = std::static_pointer_cast<PingMessage>(msg);
                    }
                }
            }

            if (_message)
            {
                ArduinoJson::JsonDocument cfg;
                cfg["rOverride"] = _message->color_R;
                cfg["gOverride"] = _message->color_G;
                cfg["bOverride"] = _message->color_B;
                UxModule::LedUtilities::setAnimationLengthMS(_ringPulseID, LED_ANIMATION_MS);
                UxModule::LedUtilities::configurePattern(_ringPulseID, cfg);
                UxModule::LedUtilities::enablePattern(_ringPulseID);
                UxModule::LedUtilities::loopPattern(_ringPulseID, -1);
                refreshIntervalMs = MESSAGE_REPEAT_INTERVAL_MS;
            }
            else
            {
                refreshIntervalMs = 0;
            }

            _rebuildDrawCommands();
        }

        void onExit() override
        {
            _releaseLed();
            _message = nullptr;
            WindowState::onExit();
        }

        void onPause() override { UxModule::LedUtilities::clearPattern(_ringPulseID); }

        void onResume() override
        {
            if (_message) { UxModule::LedUtilities::loopPattern(_ringPulseID, -1); }
        }

        void onTick() override
        {
            if (!_message) { return; }

            NavigationModule::Utilities::GetCurrentLocation(_message->lat, _message->lng);

            time_t t;
            System_Utils::GetCurrentUTC(t);
            NavigationUtils::TimeTToGpsPacked(t, _message->time, _message->date);

            LoraUtils::SendMessage(_message);
            _rebuildDrawCommands();
        }

    private:
        std::shared_ptr<PingMessage> _message;
        int _ringPulseID = -1;

        void _releaseLed()
        {
            if (_ringPulseID >= 0)
            {
                UxModule::LedUtilities::disablePattern(_ringPulseID);
                UxModule::LedUtilities::loopPattern(_ringPulseID, 0);
                UxModule::LedUtilities::resetPattern(_ringPulseID);
            }
        }

        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            if (!_message)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "No message",
                    TextFormat{ TextAlignH::CENTER, TextAlignV::CENTER }
                ));
                return;
            }

            addDrawCommand(std::make_shared<TextDrawCommand>(
                "Retransmitting...",
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 2 }
            ));

            addDrawCommand(std::make_shared<TextDrawCommand>(
                _message->status,
                TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 }
            ));
        }
    };

} // namespace DisplayModule
