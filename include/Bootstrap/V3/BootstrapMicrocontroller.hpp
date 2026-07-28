#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "EventDeclarations.h"
#include "CompassUtils.h"
#include "SystemUtilities.hpp"
#include "WiFi.h"
#include <BQ25672.h>

#define BUZZER_PIN 7

#define SDA_PIN 18
#define SCL_PIN 17

#define LORA_SPI_SCK  40
#define LORA_SPI_MISO 42
#define LORA_SPI_MOSI 41

class BootstrapMicrocontroller
{
public:

    constexpr static uint8_t ENC_A = 10;
    constexpr static uint8_t ENC_B = 9;

    constexpr static uint8_t BUTTON_1_PIN = 13;
    constexpr static uint8_t BUTTON_2_PIN = 3;
    constexpr static uint8_t BUTTON_3_PIN = 12;
    constexpr static uint8_t BUTTON_4_PIN = 11;
    // Currently unused. Will be reused for modules at some point
    constexpr static uint8_t BUTTON_SOS_PIN = 8;

    constexpr static uint8_t ENC_BUTTON_PIN = 21;

    constexpr static uint8_t CPU_CORE_LORA = 1;
    constexpr static uint8_t CPU_CORE_APP = 0;

    constexpr static uint8_t DISPLAY_RESET_PIN = 47;

    static void Initialize()
    {
        ESP_LOGI("BootstrapMicro", "Initializing IO...");
        pinMode(ENC_A, INPUT_PULLUP);
        pinMode(ENC_B, INPUT_PULLUP);
        // TODO: Rename this one
        pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
        pinMode(BUTTON_1_PIN, INPUT_PULLUP);
        pinMode(BUTTON_2_PIN, INPUT_PULLUP);
        pinMode(BUTTON_3_PIN, INPUT_PULLUP);
        pinMode(BUTTON_4_PIN, INPUT_PULLUP);
        pinMode(ENC_BUTTON_PIN, INPUT_PULLUP);
        // TODO: Add encoder button

        pinMode(BUZZER_PIN, OUTPUT);

        pinMode(DISPLAY_RESET_PIN, OUTPUT);
        digitalWrite(DISPLAY_RESET_PIN, LOW);
        delay(100);
        digitalWrite(DISPLAY_RESET_PIN, HIGH);
        delay(100);

        ESP_LOGI("BootstrapMicro", "Configuring encoder...");
        Encoder().attachFullQuad(ENC_A, ENC_B);
        Encoder().setFilter(1023);
        Encoder().setCount(0);
        
        inputEncoder = &Encoder();

        ESP_LOGI("BootstrapMicro", "Initializing SPI bus...");
        SpiBus().begin(LORA_SPI_SCK, LORA_SPI_MISO, LORA_SPI_MOSI, -1);

        ESP_LOGI("BootstrapMicro", "Initializing I2C bus...");
        auto wireSuccess = I2cBus().begin(SDA_PIN, SCL_PIN);
        if (wireSuccess)
        {
            ESP_LOGI(TAG, "Successfully initialized I2C");
        }
        else
        {
            ESP_LOGW(TAG, "Failed to init I2C");
        }

        ESP_LOGI("BootstrapMicro", "Scanning I2C addresses...");
        ScannedDevices() = CompassUtils::ScanI2cAddresses(I2cBus());
        ESP_LOGI("BootstrapMicro", "Found %d I2C devices", ScannedDevices().size());

        ESP_LOGI("BootstrapMicro", "Initializing BQ25672 PMIC...");
        auto pmicErr = Charger().begin();
        if (pmicErr != BQ25672::Error::OK)
        {
            ESP_LOGW(TAG, "BQ25672 init failed (err %d)", static_cast<int>(pmicErr));
        }
        else
        {
            ESP_LOGI(TAG, "BQ25672 initialized");
            Charger().setShipFetPresent(true);
        }

        System_Utils::getSystemShutdown() += []()
        {
            Charger().setSdrvDelayNo10s(true);
            Charger().setSdrvControl(BQ25672::SdrvCtrl::ShipMode);
        };

        System_Utils::getEnablePowerSavings() += []()
        {
            setCpuFrequencyMhz(80);
        };

        System_Utils::getDisablePowerSavings() += []()
        {
            setCpuFrequencyMhz(240);
        };

        System_Utils::registerBatteryCallback([]() -> long {
            uint16_t mv;
            if (!Charger().readVbat_mV(mv))
            {
                ESP_LOGW(TAG, "BQ25672 battery read failed");
                return 0;
            }
            // The pack tops out around 4.10 V at full charge (the PMIC's
            // charge-voltage regulation point), so treat >= 4.05 V as 100% —
            // the small margin lets a full cell read 100% both on the charger
            // (~4.10 V) and resting just after (~4.05 V). If the charge voltage
            // is ever raised toward 4.20 V, move this knee up to match.
            if (mv >= 4050) return 100;
            if (mv >= 3950) return 75 + (long)(mv - 3950) * 25 / 100;  // 3950-4050 -> 75-100
            if (mv >= 3800) return 50 + (long)(mv - 3800) * 25 / 150;  // 3800-3950 -> 50-75
            if (mv >= 3650) return 25 + (long)(mv - 3650) * 25 / 150;  // 3650-3800 -> 25-50
            if (mv >= 3500) return 10 + (long)(mv - 3500) * 15 / 150;  // 3500-3650 -> 10-25
            if (mv >= 3200) return      (long)(mv - 3200) * 10 / 300;  // 3200-3500 -> 0-10
            return 0;
        });
    }

    static void EnableInterrupts()
    {
        attachInterrupt(BUTTON_1_PIN, button1ISR, FALLING);
        attachInterrupt(BUTTON_2_PIN, button2ISR, FALLING);
        attachInterrupt(BUTTON_3_PIN, button3ISR, FALLING);
        attachInterrupt(BUTTON_4_PIN, button4ISR, FALLING);
        attachInterrupt(ENC_BUTTON_PIN, encButtonISR, FALLING);
        inputEncoder->resumeCount();
    }

    static void DisableInterrupts()
    {
        detachInterrupt(BUTTON_1_PIN);
        detachInterrupt(BUTTON_2_PIN);
        detachInterrupt(BUTTON_3_PIN);
        detachInterrupt(BUTTON_4_PIN);
        detachInterrupt(ENC_BUTTON_PIN);
        inputEncoder->pauseCount();
    }

    static TwoWire &I2cBus()
    {
        return Wire;
    }

    static ESP32Encoder &Encoder()
    {
        static ESP32Encoder encoder(true, enc_cb);
        return encoder;
    }

    static SPIClass& SpiBus()
    {
        static SPIClass spi;
        return spi;
    }

    static std::unordered_set<uint8_t>& ScannedDevices()
    {
        static std::unordered_set<uint8_t> scannedDevices;
        return scannedDevices;
    }

    static BQ25672 &Charger()
    {
        static BQ25672 charger(I2cBus());
        return charger;
    }
};