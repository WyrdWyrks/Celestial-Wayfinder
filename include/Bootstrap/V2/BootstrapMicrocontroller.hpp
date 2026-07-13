#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "EventDeclarations.h"
#include "CompassUtils.h"
#include "SystemUtilities.hpp"

#define BUZZER_PIN 4
#define BATT_SENSE_PIN 39

#define SDA_PIN 21
#define SCL_PIN 22

class BootstrapMicrocontroller
{
public:

    constexpr static uint8_t ENC_A = 23;
    constexpr static uint8_t ENC_B = 2;

    constexpr static uint8_t BUTTON_1_PIN = 5;
    constexpr static uint8_t BUTTON_2_PIN = 26;
    constexpr static uint8_t BUTTON_3_PIN = 19;
    constexpr static uint8_t BUTTON_4_PIN = 34;
    constexpr static uint8_t BUTTON_SOS_PIN = 25;

    constexpr static uint8_t CPU_CORE_LORA = 1;
    constexpr static uint8_t CPU_CORE_APP = 0;

    static void Initialize()
    {
        pinMode(ENC_A, INPUT_PULLUP);
        pinMode(ENC_B, INPUT_PULLUP);
        // TODO: Rename this one
        pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
        pinMode(BUTTON_1_PIN, INPUT_PULLUP);
        pinMode(BUTTON_2_PIN, INPUT_PULLUP);
        pinMode(BUTTON_3_PIN, INPUT_PULLUP);
        pinMode(BUTTON_4_PIN, INPUT_PULLUP);
        // TODO: Add encoder button

        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(BATT_SENSE_PIN, INPUT);

        Encoder().attachFullQuad(ENC_A, ENC_B);
        Encoder().setFilter(1023);
        Encoder().setCount(0);
        
        inputEncoder = &Encoder();

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

        ScannedDevices() = CompassUtils::ScanI2cAddresses(I2cBus());

        System_Utils::getEnablePowerSavings() += []()
        {
            setCpuFrequencyMhz(80);
        };

        System_Utils::getDisablePowerSavings() += []()
        {
            setCpuFrequencyMhz(240);
        };

        System_Utils::registerBatteryCallback([]() -> long {
            uint32_t sum = 0;
            for (int i = 0; i < 8; i++) {
                sum += analogReadMilliVolts(BATT_SENSE_PIN);
                delayMicroseconds(100);
            }
            uint16_t mv = (uint16_t)(sum / 8);
            if (mv == 0)    return 100; // USB powered
            if (mv >= 2100) return 100;
            if (mv >= 2050) return 90 + (long)(mv - 2050) * 10 / 50;
            if (mv >= 1975) return 75 + (long)(mv - 1975) * 15 / 75;
            if (mv >= 1900) return 50 + (long)(mv - 1900) * 25 / 75;
            if (mv >= 1825) return 25 + (long)(mv - 1825) * 25 / 75;
            if (mv >= 1750) return 10 + (long)(mv - 1750) * 15 / 75;
            if (mv >= 1600) return      (long)(mv - 1600) * 10 / 150;
            return 0;
        });
    }

    static void EnableInterrupts()
    {
        attachInterrupt(BUTTON_1_PIN, button1ISR, FALLING);
        attachInterrupt(BUTTON_2_PIN, button2ISR, FALLING);
        attachInterrupt(BUTTON_3_PIN, button3ISR, FALLING);
        attachInterrupt(BUTTON_4_PIN, button4ISR, FALLING);
        inputEncoder->resumeCount();
    }

    static void DisableInterrupts()
    {
        detachInterrupt(BUTTON_1_PIN);
        detachInterrupt(BUTTON_2_PIN);
        detachInterrupt(BUTTON_3_PIN);
        detachInterrupt(BUTTON_4_PIN);
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
        static SPIClass spi(HSPI);
        return spi;
    }

    static std::unordered_set<uint8_t>& ScannedDevices()
    {
        static std::unordered_set<uint8_t> scannedDevices;
        return scannedDevices;
    }

private:

};