#pragma once

#include "WindowState.hpp"
#include "QrCodeDrawCommand.hpp"
#include "BluetoothUtilities.hpp"
#include "HelperClasses/PwaQr.hpp"

#include "esp_task_wdt.h"


namespace DisplayModule
{
    class BtState : public WindowState
    {
    public:
        static constexpr uint32_t REFRESH_MS = 500;

        BtState()
        {
            refreshIntervalMs = REFRESH_MS;
        }

        void onEnter(const StateTransferData &) override
        {
            ESP_LOGI("BtState", "Entering Bluetooth state");
            ESP_LOGI("BT", "Free heap: %d, min free: %d", ESP.getFreeHeap(), ESP.getMinFreeHeap());
            // esp_task_wdt_add(NULL);  // NULL = calling task
            Bluetooth_Utils::initBluetooth();
            _rebuild();
        }

        void onTick() override { _rebuild(); }

        void onExit() override
        {
            ESP_LOGI("BtState", "Exiting Bluetooth state; releasing radio");
            Bluetooth_Utils::deinitBluetooth();
            WindowState::onExit();
        }

    private:
        void _rebuild()
        {
            clearDrawCommands();

            if (!Bluetooth_Utils::bluetoothConnected())
            {
                ESP_LOGI("BtState", "Waiting for Bluetooth connection...");
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Waiting for", TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 2}));
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Connection...", TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 3}));
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Visit", TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 13}));
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "wayfinder.",
                    TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 14}));
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "wyrdwyrks.com",
                    TextFormat{TextAlignH::CENTER, TextAlignV::LINE, 15}));

                // 2x scale (66px) fills the gap between the header text (ends at
                // y=24) and the "Visit" text (starts at y=96); nudge up to clear it.
                auto qrCode = std::make_shared<DisplayModule::QrCodeDrawCommand>(
                    pwa_qr,
                    PWA_QR_MODULES,
                    2,
                    DisplayModule::QrFormat(DisplayModule::TextAlignH::CENTER, DisplayModule::TextAlignV::CENTER, 0, -4)
                );
                addDrawCommand(qrCode);
            }
            else if (!Bluetooth_Utils::bluetoothPaired())
            {
                std::string pin = "PIN: "
                    + std::to_string(Bluetooth_Utils::bluetoothPin());
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    pin,
                    TextFormat{TextAlignH::CENTER, TextAlignV::CENTER}));
            }
            else
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Connected!",
                    TextFormat{TextAlignH::CENTER, TextAlignV::CENTER}));
            }
        }
    };
}