#pragma once

// V3 LoRa bring-up: RadioLib SX1276 + the MeshCore routing engine, one
// wait-discipline task. App PingMessages ride the group channel through the
// LoraModule::Utilities façade.

#include "Bootstrap/V3/BootstrapMicrocontroller.hpp"
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
    const uint8_t V3_LORA_CS   = 39;
    const uint8_t V3_LORA_RST  = 38;
    const uint8_t V3_LORA_DIO0 = 48;
    const uint8_t V3_LORA_TX   = 23;

    // MeshCore's isChannelActive() treats RSSI > noise_floor + threshold as
    // busy; threshold 0 (its default) is always true on a live RX chain and
    // stalls every send until Dispatcher's 4 s CAD-busy override. Per-rev
    // tuning knob — belongs with the pins. (The LORA_* compile stubs RadioLib
    // headers need are in RadioLibLoRaDriver.hpp.)
    constexpr int V3_LORA_TX_THRESHOLD = 12;
}

class BootstrapLora
{
public:
    static void Initialize()
    {
        WayfinderLoraState::Init();

        // Seed the PRNG before identity generation — an unseeded StdRNG would
        // give every fresh device the same Ed25519 key and DeviceID.
        Rng().begin(static_cast<long>(esp_random()));

        // The library's Manager retunes the radio on a "LoRa Channel" change
        // through this seam (it cannot include the app's driver header).
        LoraModule::RegisterRadioRetuner([](mesh::Radio* radio, float freqMHz)
        {
            static_cast<LoraModule::RadioLibLoRaDriver*>(radio)->setFrequencyMHz(freqMHz);
        });

        // Always relay other nodes' traffic — not a user setting (mobile devices;
        // a wandered leaf node would silently degrade the mesh). Bench builds flip it.
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
        static LoraModule::RadioLibLoRaDriver driver(
            BootstrapMicrocontroller::SpiBus(),
            V3_LORA_CS, V3_LORA_RST, V3_LORA_DIO0, RADIOLIB_NC,
            Board(),
            LoraModule::ChannelToHz(LoraModule::LORA_CHANNEL_DEFAULT) / 1000000.0f,
            static_cast<int8_t>(V3_LORA_TX));
        return driver;
    }

    static ArduinoMillis&            MillisClock() { static ArduinoMillis c;              return c; }
    static StdRNG&                   Rng()         { static StdRNG r;                      return r; }
    static LoraModule::MeshTimeClock& RtcClock()   { static LoraModule::MeshTimeClock c;  return c; }
    static StaticPoolPacketManager&  Packets()     { static StaticPoolPacketManager m(16); return m; }
    static LoraModule::MeshTables&   Tables()      { static LoraModule::MeshTables t;      return t; }

    // Declared before Manager() (the Meyers singleton instantiates it). Overrides
    // the interference threshold per rev — see V3_LORA_TX_THRESHOLD above.
    class BootstrapManager : public LoraModule::Manager
    {
    public:
        using LoraModule::Manager::Manager;

    protected:
        int getInterferenceThreshold() const override { return V3_LORA_TX_THRESHOLD; }
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
