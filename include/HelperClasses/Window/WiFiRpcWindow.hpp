#pragma once

#include "Window.hpp"
#include "States/AwaitWifiState.hpp"
#include "States/BroadcastTcpState.hpp"
#include "DisplayUtilities.hpp"
#include "RadioUtils.h"
#include "FilesystemUtils.h"

namespace DisplayModule
{
    class WiFiRpcWindow : public Window
    {
    public:
        WiFiRpcWindow(bool useApMode = true) : _useApMode(useApMode)
        {
            _state = std::make_shared<WindowState>();

            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(100);

            if (useApMode)
            {
                _initWiFiAp();
            }
            else
            {
                _initWiFiSta();
            }
            _buildDrawCommands();

            _state->bindInput(InputID::BUTTON_3, "Back",
                [](const InputContext &) { Utilities::popWindow(); });

            _state->refreshIntervalMs = 5000; // Refresh every 5 seconds to update IP address, etc.

            setInitialState(_state);
        }

        ~WiFiRpcWindow()
        {
            ConnectivityModule::RadioUtils::StopAccessPoint();

            if (ConnectivityModule::RadioUtils::IsRadioActive())
            {
                ConnectivityModule::RadioUtils::DisableRadio();
            }            
        }

        void onTick() override
        {
            _buildDrawCommands();
        }

    private:
        std::shared_ptr<WindowState>        _state;
        std::string                         _ssid;
        std::string                         _password;
        bool                                _useApMode;

        void _initWiFiAp()
        {
            // TODO: Make the password somewhat more persistent
            _generateApCredentials();
            ConnectivityModule::RadioUtils::ApSSID() = _ssid;
            ConnectivityModule::RadioUtils::ApPassword() = _password;
            ConnectivityModule::RadioUtils::StartAccessPoint();
        }

        // Testing purposes
        void _initWiFiSta()
        {
            _ssid = "";
            _password = "";
            ConnectivityModule::RadioUtils::ConnectToAccessPoint(_ssid, _password);
        }

        void _generateApCredentials()
        {
            // SSID = ESP32-Utils-<DeviceID Hex>
            auto deviceID = System_Utils::DeviceID;
            char ssidBuffer[20];
            sprintf(ssidBuffer, "ESP-CFG-%04X", deviceID & 0xFFFF);
            _ssid = ssidBuffer;
            _password = FilesystemModule::Utilities::FetchStringSetting("WiFi AP Password", "esp-pass");
        }

        void _buildDrawCommands()
        {
            if (_useApMode)
            {
                _buildApModeDrawCommands();
            }
            else
            {
                _buildStaModeDrawCommands();
            }
        }

        void _buildApModeDrawCommands()
        {
            _state->clearDrawCommands();

            _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                "Access Point Active", TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 1}));

            _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                "SSID: " + _ssid, TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 2}));

            _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                "Password: " + _password, TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 3}));

            _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                "Ip: " + ConnectivityModule::RadioUtils::GetWiFiIpAddress(), TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 4}));
        }

        void _buildStaModeDrawCommands()
        {
            _state->clearDrawCommands();

            _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                "Station Mode Active", TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 1}));

            _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                "SSID: " + _ssid, TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 2}));

            auto ipAddress = ConnectivityModule::RadioUtils::GetWiFiIpAddress();
            if (ipAddress != "0.0.0.0")
            {
                _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Connected with IP: " + ipAddress, TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 3}));
            }
            else
            {
                _state->addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Not Connected", TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 3}));
            }
        }
    };

} // namespace DisplayModule
