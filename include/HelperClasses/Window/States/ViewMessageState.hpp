#pragma once

#include <memory>
#include <vector>
#include <string>
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "../../DrawCommands/LineCompassDrawCommand.hpp"
#include "LoraUtils.h"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "HelperClasses/PingMessage.hpp"

namespace DisplayModule
{
    class ViewMessageState : public WindowState
    {
    public:
        ViewMessageState()
        {
            refreshIntervalMs = 15;

            bindInput(InputID::ENC_UP, "", [this](const InputContext &) {
                if (_currIndex > 0)
                {
                    _currIndex--;
                    _cachedPing = WayfinderLoraState::GetUnreadMessage(_userIds[_currIndex]);
                }
                else if (_requestExitStateCallback)
                {
                    LED_Utils::disablePattern(_ringPointId);
                    _requestExitStateCallback();
                }

                _configureLed();
                _rebuildDrawCommands();
            });

            bindInput(InputID::ENC_DOWN, "v", [this](const InputContext &) {
                if (_currIndex + 1 < _userIds.size())
                {
                    _currIndex++;
                    _cachedPing = WayfinderLoraState::GetUnreadMessage(_userIds[_currIndex]);
                }
                _configureLed();
                _rebuildDrawCommands();
            });
        }

        void onEnter(const StateTransferData &data) override
        {
            _userIds   = WayfinderLoraState::GetUnreadUserIds();
            _currIndex = 0;
            _cachedPing = nullptr;

            if (!_userIds.empty())
            {
                _cachedPing = WayfinderLoraState::GetUnreadMessage(_userIds[0]);
            }

            _ringPointId = RingPoint::RegisteredPatternID();
            ESP_LOGI(TAG, "Entering ViewMessageState with ring point ID %d", _ringPointId);

            _configureLed();
            LED_Utils::enablePattern(_ringPointId);

            _rebuildDrawCommands();
        }

        void onExit() override
        {
            LED_Utils::disablePattern(_ringPointId);
            _userIds.clear();
            _currIndex = 0;
        }

        void assignExitCallback(std::function<void()> cb)
        {
            _requestExitStateCallback = std::move(cb);
        }

        void onTick() override
        {
            // Non-blocking cache refresh — keeps renders fast at 15 ms interval.
            // Falls back to cached value if the LoRa task holds the mutex.
            if (_currIndex < _userIds.size())
            {
                auto fresh = WayfinderLoraState::TryGetUnreadMessage(_userIds[_currIndex]);
                if (fresh) { _cachedPing = fresh; }
            }

            _configureLed();
            _rebuildDrawCommands();
        }

        // Returns the currently displayed message (may be nullptr if list is empty).
        std::shared_ptr<PingMessage> currentMessage() const
        {
            return _cachedPing;
        }

        // Marks the current message as read, removes it from the local list, and
        // refreshes the cache. Returns true if more unread messages remain.
        bool markCurrentRead()
        {
            if (_userIds.empty()) { return false; }

            WayfinderLoraState::MarkRead(_userIds[_currIndex]);
            _userIds.erase(_userIds.begin() + _currIndex);

            if (_currIndex >= _userIds.size() && _currIndex > 0) { _currIndex--; }

            _cachedPing = _userIds.empty()
                ? nullptr
                : WayfinderLoraState::GetUnreadMessage(_userIds[_currIndex]);

            _configureLed();
            _rebuildDrawCommands();
            return !_userIds.empty();
        }

    private:
        std::vector<uint32_t> _userIds;
        size_t _currIndex = 0;
        std::shared_ptr<PingMessage> _cachedPing;

        int _ringPointId = -1;
        std::function<void()> _requestExitStateCallback;

        const uint8_t _LARGE_DISPLAY_MIN_LINES = 16;
        const uint8_t _MED_DISPLAY_MIN_LINES = 8;
        const uint8_t _MIN_DISPLAY_MIN_LINES = 4;

        void _configureLed()
        {
            if (!_cachedPing) { return; }

            JsonDocument cfg;
            cfg["rOverride"] = _cachedPing->color_R;
            cfg["gOverride"] = _cachedPing->color_G;
            cfg["bOverride"] = _cachedPing->color_B;

            double distance = NavigationUtils::GetDistanceTo(_cachedPing->lat, _cachedPing->lng);

            const size_t ledFxMin = 20;
            const size_t ledFxMax = 500;

            if (distance > ledFxMax)
            {
                distance = ledFxMax;
            }
            else if (distance < ledFxMin)
            {
                distance = ledFxMin;
            }

            auto directionDegrees = _getMessageHeading();
            float fadeDegrees = -0.075f * distance + 61.5;

            cfg["fadeDegrees"] = fadeDegrees;
            cfg["directionDegrees"] = directionDegrees;

            LED_Utils::configurePattern(_ringPointId, cfg);
            LED_Utils::iteratePattern(_ringPointId);
        }

        void _rebuildDrawCommands()
        {
            // Show/hide ENC_DOWN arrow based on whether a next message exists
            if (_currIndex + 1 < _userIds.size())
            {
                bindInput(InputID::ENC_DOWN, "v");
            }
            else
            {
                bindInput(InputID::ENC_DOWN, "");
            }

            clearDrawCommands();

            auto displayLines = Utilities::selectBottomTextLine();

            addDrawCommand(std::make_shared<LineCompassDrawCommand>(
                static_cast<int>(_getMessageHeading()),
                _getMessageDistance(),
                1));

            if (displayLines >= _LARGE_DISPLAY_MIN_LINES)
            {
                TextFormat fmt(TextAlignH::CENTER, TextAlignV::LINE, 2);

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayIndex(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 3;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageHeader(),
                    fmt
                ));

                fmt.line = 4;
                fmt.hAlign = TextAlignH::LEFT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayUserName(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::RIGHT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageAge(),
                    fmt
                ));

                fmt.line = 5;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _getLineDivider(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 6;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageContent(),
                    fmt
                ));
            }
            else if (displayLines >= _MED_DISPLAY_MIN_LINES)
            {
                TextFormat fmt(TextAlignH::CENTER, TextAlignV::LINE, 2);

                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayIndex(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 3;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageHeader(),
                    fmt
                ));

                fmt.line = 4;
                fmt.hAlign = TextAlignH::LEFT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayUserName(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::RIGHT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageAge(),
                    fmt
                ));

                fmt.line = 5;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _getLineDivider(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 6;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageContent(),
                    fmt
                ));
            }
            else if (displayLines >= _MIN_DISPLAY_MIN_LINES)
            {
                TextFormat fmt(TextAlignH::LEFT, TextAlignV::LINE, 2);
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayUserName(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::RIGHT;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageAge(),
                    fmt
                ));

                fmt.hAlign = TextAlignH::CENTER;
                fmt.line = 3;
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    _displayMessageContent(),
                    fmt
                ));
            }
            else
            {
                ESP_LOGE(TAG, "Display height too small to show message content");
            }
        }

        std::string _displayIndex()
        {
            return "[" + std::to_string(_currIndex + 1) + " of " + std::to_string(_userIds.size()) + "]";
        }

        std::string _getLineDivider()
        {
            return "=====================";
        }

        std::string _displayUserName()
        {
            static constexpr size_t LINE_WIDTH = 21;
            if (!_cachedPing) { return ""; }

            char hex[5];
            snprintf(hex, sizeof(hex), "%04" PRIX32, _cachedPing->sender & 0xFFFF);
            std::string suffix = std::string("#") + hex;

            std::string name = _cachedPing->senderName;
            if (name.length() + suffix.length() > LINE_WIDTH)
            {
                name = name.substr(0, LINE_WIDTH - suffix.length());
            }

            return name + suffix;
        }

        std::string _displayMessageHeader()
        {
            return "From         Msg Age";
        }

        // TODO: Measure time from GPS date/time and message date/time
        std::string _displayMessageAge()
        {
            if (!_cachedPing) { return ""; }

            time_t now = 0;
            time_t msgTime = NavigationModule::Utilities::PackedToTimeT(_cachedPing->time, _cachedPing->date);

            if (msgTime <= 0 || now <= msgTime)
            {
                return "<1m";
            }

            time_t diffSec = now - msgTime;
            ESP_LOGV(TAG, "Time diff: %lld seconds", (long long)diffSec);

            if (diffSec >= 86400) { return ">1d"; }

            uint8_t diffHours   = diffSec / 3600;
            uint8_t diffMinutes = (diffSec % 3600) / 60;

            if (diffHours > 0)   { return std::to_string(diffHours) + "h"; }
            if (diffMinutes > 0) { return std::to_string(diffMinutes) + "m"; }

            return "<1m";
        }

        std::string _displayMessageContent()
        {
            if (!_cachedPing) { return ""; }
            return std::string(_cachedPing->status);
        }

        float _getMessageHeading()
        {
            if (!_cachedPing) { return 0; }

            double heading = NavigationUtils::GetHeadingTo(_cachedPing->lat, _cachedPing->lng);
            auto bearing = NavigationUtils::GetBearing(heading);

            ESP_LOGI(TAG, "Current Heading: %d, Message Heading: %.4f, Bearing: %f ", NavigationUtils::GetAzimuth(), heading, bearing);

            return bearing;
            // int azimuth = NavigationUtils::GetAzimuth();

            // float directionDegrees = heading - azimuth;
            // if (directionDegrees < 0) { directionDegrees += 360; }

            // return directionDegrees;
        }

        int _getMessageDistance()
        {
            if (!_cachedPing) { return 0; }
            double distance = NavigationUtils::GetDistanceTo(_cachedPing->lat, _cachedPing->lng);
            return static_cast<int>(distance);
        }
    };
}
