#pragma once

// V2 LoRa bring-up: RadioLib SX1276 + the MeshCore routing engine. Same shape
// as V3; only the pins, the missing RESET line, and the 1 MHz SPI differ.

#include "Bootstrap/V2/BootstrapMicrocontroller.hpp"
#include "HelperClasses/PingMessage.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "EventDeclarations.h"

#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>

#include "HelperClasses/LoRaDriver/RadioLibLoRaDriver.hpp"
#include "HelperClasses/LoRaDriver/MeshBoard.hpp"
#include "ModuleManagers/LoraManager.hpp"
#include "HelperClasses/Mesh/MeshTables.hpp"
#include "HelperClasses/Mesh/MeshTimeClock.hpp"
#include "FilesystemUtils.h"

namespace
{
    const uint8_t V2_LORA_CS   = 15;
    const uint8_t V2_LORA_DIO0 = 18;
    const uint8_t V2_LORA_TX   = 23;

    // Same THRESHOLD note as V3 — see V3/BootstrapLora.hpp. Same starting value
    // until measured on this rev's front end.
    constexpr int V2_LORA_TX_THRESHOLD = 12;
}

class BootstrapLora
{
public:
    // Classic-ESP32 HSPI pins for V2. The legacy arduino-LoRa path relied on a
    // bare spi->begin() picking these up as defaults; V2's BootstrapMicrocontroller
    // never begins the bus itself, so do it here (as V3 does in its own micro
    // bootstrap) before RadioLib touches the radio.
    static constexpr uint8_t V2_LORA_SCK  = 14;
    static constexpr uint8_t V2_LORA_MISO = 12;
    static constexpr uint8_t V2_LORA_MOSI = 13;

    static void Initialize()
    {
        WayfinderLoraState::Init();

        BootstrapMicrocontroller::SpiBus().begin(V2_LORA_SCK, V2_LORA_MISO, V2_LORA_MOSI, V2_LORA_CS);

        // Seed the PRNG before identity generation — an unseeded StdRNG would
        // give every fresh device the same Ed25519 key and DeviceID.
        Rng().begin(static_cast<long>(esp_random()));

        LoraModule::RegisterRadioRetuner([](mesh::Radio* radio, float freqMHz)
        {
            static_cast<LoraModule::RadioLibLoRaDriver*>(radio)->setFrequencyMHz(freqMHz);
        });

        Manager().SetRepeat(true);

        if (!Manager().Begin())
        {
            ESP_LOGE("BootstrapLora", "MeshCore init failed");
            return;
        }

        LoraModule::Utilities::RegisterMessageType(PingMessage::GUID, PingMessage::Create);
        LoraModule::Utilities::MessageTypeReceived(PingMessage::GUID) += CompassUtils::PassMessageReceivedToDisplay;

        System_Utils::registerTask(BootstrapLora::MeshTaskRunner, "mesh-task", 8192, nullptr,
                                   3, BootstrapMicrocontroller::CPU_CORE_LORA);
    }

    static MeshBoard& Board()
    {
        static MeshBoard board;
        return board;
    }

    static LoraModule::RadioLibLoRaDriver& Driver()
    {
        // rst and dio1 both RADIOLIB_NC: V2 wires neither. 1 MHz SPI to match the
        // legacy arduino-LoRa driver — this rev returns -2 CHIP_NOT_FOUND at
        // RadioLib's default 2 MHz.
        static LoraModule::RadioLibLoRaDriver driver(
            BootstrapMicrocontroller::SpiBus(),
            V2_LORA_CS, RADIOLIB_NC, V2_LORA_DIO0, RADIOLIB_NC,
            Board(),
            LoraModule::ChannelToHz(LoraModule::LORA_CHANNEL_DEFAULT) / 1000000.0f,
            static_cast<int8_t>(V2_LORA_TX),
            1000000);
        return driver;
    }

    static ArduinoMillis&            MillisClock() { static ArduinoMillis c;              return c; }
    static StdRNG&                   Rng()         { static StdRNG r;                      return r; }
    static LoraModule::MeshTimeClock& RtcClock()   { static LoraModule::MeshTimeClock c;  return c; }
    static StaticPoolPacketManager&  Packets()     { static StaticPoolPacketManager m(16); return m; }
    static LoraModule::MeshTables&   Tables()      { static LoraModule::MeshTables t;      return t; }

    class BootstrapManager : public LoraModule::Manager
    {
    public:
        using LoraModule::Manager::Manager;

    protected:
        int getInterferenceThreshold() const override { return V2_LORA_TX_THRESHOLD; }
    };

    static LoraModule::Manager& Manager()
    {
        static BootstrapManager manager(Driver(), MillisClock(), Rng(),
                                        RtcClock(), Packets(), Tables());
        return manager;
    }

    static void MeshTaskRunner(void* /*pvParameters*/)
    {
        Manager().Loop();
    }
};
