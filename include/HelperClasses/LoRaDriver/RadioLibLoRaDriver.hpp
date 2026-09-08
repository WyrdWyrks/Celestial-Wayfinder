#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <esp_log.h>

#include <helpers/radiolib/CustomSX1276.h>
#include <helpers/radiolib/CustomSX1276Wrapper.h>
#include "MeshBoard.hpp"

// RadioLib-backed radio for the MeshCore routing engine. Replaces
// ArduinoLoRaDriver (the Blake-Ballew/arduino-LoRa fork). This is a
// mesh::Radio, not a LoraDriverInterface -- LoraModule::Manager : mesh::Mesh
// owns it directly and drives it from mesh.loop().
//
// Modem config is fixed for the closed network (migration plan section 2 / 5.5):
//   SF7 / BW 500 kHz / CR 4:8 / preamble 12 / CRC on / private sync word.
// Frequency is passed in from LoraModule::ChannelToHz() at construction.
//
// CustomSX1276::std_init() is deliberately NOT used: it takes pins and modem
// params from compile-time macros (P_LORA_*, LORA_*), which fights the runtime
// per-HARDWARE_VERSION bootstrap. We construct Module() with runtime pins and
// call RadioLib's begin() + setCRC(1) directly (plan 5.5). DIO0 covers both
// RX-done and TX-done; DIO1 (CAD/preamble) is optional and passed as
// RADIOLIB_NC when the board doesn't wire it.
namespace LoraModule
{
    namespace detail
    {
        // CustomSX1276 must be constructed before the CustomSX1276Wrapper base
        // can bind a reference to it. A private base initialised first (by
        // declaration order) gives us that ordering without a static.
        struct SX1276Holder
        {
            CustomSX1276 radio;
            explicit SX1276Holder(Module* mod) : radio(mod) {}
        };
    }

    class RadioLibLoRaDriver : private detail::SX1276Holder, public CustomSX1276Wrapper
    {
    public:
        static constexpr const char* TAG = "RadioLibLoRa";

        // freqMHz: e.g. 915.0. txPowerDbm: SX1276 PA_BOOST range ~2..20 (V1) or
        // up to 23 with the fork's over-range handling; RadioLib clamps.
        RadioLibLoRaDriver(SPIClass& spi,
                           int csPin, int rstPin, int dio0Pin, int dio1Pin,
                           mesh::MainBoard& board,
                           float freqMHz, int8_t txPowerDbm)
            : detail::SX1276Holder(new Module(static_cast<uint32_t>(csPin),
                                              static_cast<uint32_t>(dio0Pin),
                                              static_cast<uint32_t>(rstPin),
                                              static_cast<uint32_t>(dio1Pin),
                                              spi)),
              CustomSX1276Wrapper(radio, board),
              _freqMHz(freqMHz),
              _txPowerDbm(txPowerDbm)
        {
        }

        // mesh::Radio::begin() -- configure the modem, then let RadioLibWrapper
        // wire the DIO0 action and enter RX.
        void begin() override
        {
            // RadioLib's SX1276 PA_BOOST path allows 2..20 dBm; anything above
            // returns RADIOLIB_ERR_INVALID_OUTPUT_POWER (-13). The legacy
            // arduino-LoRa fork's "23" never delivered a real +23 dBm.
            int8_t power = _txPowerDbm;
            if (power > 20) { power = 20; }
            if (power < 2)  { power = 2; }
            if (power != _txPowerDbm)
            {
                ESP_LOGW(TAG, "TX power %d dBm out of range - clamped to %d", _txPowerDbm, power);
                _txPowerDbm = power;
            }

            int16_t st = radio.begin(_freqMHz,
                                     500.0f,   // BW kHz
                                     7,        // SF
                                     8,        // CR denominator (4:8)
                                     RADIOLIB_SX127X_SYNC_WORD,
                                     _txPowerDbm,
                                     12);      // preamble symbols
            if (st != RADIOLIB_ERR_NONE)
            {
                ESP_LOGE(TAG, "SX1276 begin() failed: %d", st);
            }
            else
            {
                ESP_LOGI(TAG, "SX1276 up @ %.3f MHz  SF7 BW500 CR4:8  %d dBm",
                         _freqMHz, _txPowerDbm);
            }

            radio.setCRC(1);

            RadioLibWrapper::begin();
        }

        // Runtime retune for the channel setting. Keeps the fixed SF/BW/CR.
        void setFrequencyMHz(float freqMHz)
        {
            _freqMHz = freqMHz;
            int16_t st = radio.setFrequency(freqMHz);
            if (st != RADIOLIB_ERR_NONE)
            {
                ESP_LOGE(TAG, "setFrequency(%.3f) failed: %d", freqMHz, st);
            }
        }

        float frequencyMHz() const { return _freqMHz; }

    private:
        float  _freqMHz;
        int8_t _txPowerDbm;
    };
}
