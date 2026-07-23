#pragma once

#include "WindowState.hpp"
#include "QrCodeDrawCommand.hpp"
#include "BluetoothUtilities.hpp"

#include "esp_task_wdt.h"


namespace DisplayModule
{
    namespace
    {
        const unsigned char pwa_qr [] PROGMEM = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x97, 0xd0, 0xfe, 0x00, 0x20, 
            0xbc, 0x50, 0x82, 0x00, 0x2e, 0xbe, 0xa2, 0xba, 0x00, 0x2e, 0xb2, 0xd2, 0xba, 0x00, 0x2e, 0x9b, 
            0xd6, 0xba, 0x00, 0x20, 0x97, 0xa8, 0x82, 0x00, 0x3f, 0xaa, 0xaa, 0xfe, 0x00, 0x00, 0x2f, 0x04, 
            0x00, 0x00, 0x20, 0xa2, 0xd3, 0x9c, 0x00, 0x3c, 0x3c, 0x84, 0x6c, 0x00, 0x09, 0xb6, 0x1b, 0x80, 
            0x00, 0x39, 0x30, 0xe9, 0x70, 0x00, 0x14, 0x8f, 0x80, 0xc2, 0x00, 0x35, 0x1f, 0x9e, 0xe6, 0x00, 
            0x23, 0xe3, 0xfa, 0xf8, 0x00, 0x0e, 0x2f, 0xa4, 0x0a, 0x00, 0x0a, 0xec, 0xd2, 0x58, 0x00, 0x31, 
            0x42, 0x26, 0xee, 0x00, 0x39, 0xd1, 0xae, 0xb2, 0x00, 0x23, 0x60, 0xe6, 0x20, 0x00, 0x25, 0x82, 
            0x1f, 0xee, 0x00, 0x00, 0x37, 0x3a, 0x30, 0x00, 0x3f, 0x8d, 0xf6, 0xb8, 0x00, 0x20, 0x99, 0x2a, 
            0x26, 0x00, 0x2e, 0x8d, 0x27, 0xf0, 0x00, 0x2e, 0x99, 0x6b, 0x1c, 0x00, 0x2e, 0x8a, 0xf9, 0xfc, 
            0x00, 0x20, 0x81, 0x45, 0xda, 0x00, 0x3f, 0xa2, 0x14, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00
        };
    }

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

                auto qrCode = std::make_shared<DisplayModule::QrCodeDrawCommand>(
                    pwa_qr,
                    33,
                    1,
                    DisplayModule::QrFormat(DisplayModule::TextAlignH::CENTER, DisplayModule::TextAlignV::CENTER)
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