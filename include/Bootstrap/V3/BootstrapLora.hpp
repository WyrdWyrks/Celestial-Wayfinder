#pragma once

#include "Bootstrap/V3/BootstrapMicrocontroller.hpp"
#include "HelperClasses/PingMessage.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "EventDeclarations.h"

namespace
{
    const uint8_t V3_LORA_CS   = 39;
    const uint8_t V3_LORA_RST  = 38;
    const uint8_t V3_LORA_DIO0 = 48;
    const uint8_t V3_LORA_TX   = 23;
}

#ifndef USE_MESHCORE_LORA
// =============================================================================
// Legacy stack: Blake-Ballew/arduino-LoRa fork + hand-rolled flood engine.
// =============================================================================

#include "LoraManager.hpp"
#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"

class BootstrapLora
{
public:
    static void Initialize()
    {
        auto& mgr = Manager();
        if (!mgr.Init())
        {
            ESP_LOGE("BootstrapLora", "Failed to initialize LoRa module");
            return;
        }

        Driver().SetSpreadingFactor(7);
        Driver().SetSignalBandwidth(500E3);
        Driver().SetTXPower(V3_LORA_TX);

        LoraModule::Utilities::RegisterMessageType(PingMessage::GUID, PingMessage::Create);

        WayfinderLoraState::Init();

        // Register the DIO0 ISR callback — StartReceiving() is deferred to RadioTask()
        // so the radio does not enter RX mode until the task handle is set.
        Driver().RegisterOnReceive(LoRaReceiveISR);

        System_Utils::registerTask(BootstrapLora::RadioTaskRunner,    "radio-task",      8192, nullptr, 3, BootstrapMicrocontroller::CPU_CORE_LORA);
        System_Utils::registerTask(BootstrapLora::SendQueueTaskRunner,"send-queue-task", 8192, nullptr, 2, BootstrapMicrocontroller::CPU_CORE_LORA);

        LoraModule::Utilities::MessageTypeReceived(PingMessage::GUID) += CompassUtils::PassMessageReceivedToDisplay;
    }

    static ArduinoLoRaDriver& Driver()
    {
        static ArduinoLoRaDriver driver(&BootstrapMicrocontroller::SpiBus(), V3_LORA_CS, V3_LORA_RST, V3_LORA_DIO0,
                                        LoraModule::ChannelToHz(LoraModule::LORA_CHANNEL_DEFAULT));
        return driver;
    }

    static LoraModule::Manager& Manager()
    {
        static LoraModule::Manager manager(&Driver());
        return manager;
    }

    static void RadioTaskRunner(void* pvParameters)
    {
        radioReadTaskHandle = xTaskGetCurrentTaskHandle();
        Manager().RadioTask();
    }

    static void SendQueueTaskRunner(void* pvParameters)
    {
        Manager().SendQueueTask();
    }
};

#else
// =============================================================================
// MeshCore stack (Phases 1-2). RadioLib SX1276 + MeshCore routing engine, one
// wait-discipline task. App PingMessages ride the group channel through the
// unchanged LoraModule::Utilities facade.
// =============================================================================

#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>

#include "HelperClasses/LoRaDriver/RadioLibLoRaDriver.hpp"
#include "HelperClasses/LoRaDriver/MeshBoard.hpp"
#include "ModuleManagers/LoraMeshManager.hpp"
#include "HelperClasses/Mesh/MeshTables.hpp"
#include "HelperClasses/Mesh/MeshTimeClock.hpp"
#include "FilesystemUtils.h"

class BootstrapLora
{
public:
    static void Initialize()
    {
        WayfinderLoraState::Init();

        // Seed the PRNG before identity generation — an unseeded StdRNG would
        // give every fresh device the same Ed25519 key and DeviceID.
        Rng().begin(static_cast<long>(esp_random()));

        // Begin() reads the persisted Channel Key itself; runtime changes flow
        // through LoraModule::Utilities::UpdateSettings(), which in this build is
        // the single consumer of "Channel Key" and routes straight to
        // MeshManager::ApplyChannelKey(). No subscription needed here.

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

    static ArduinoMillis&           MillisClock() { static ArduinoMillis c;                 return c; }
    static StdRNG&                  Rng()         { static StdRNG r;                          return r; }
    static LoraModule::MeshTimeClock& RtcClock()  { static LoraModule::MeshTimeClock c;      return c; }
    static StaticPoolPacketManager& Packets()     { static StaticPoolPacketManager m(16);    return m; }
    static LoraModule::MeshTables&  Tables()       { static LoraModule::MeshTables t;         return t; }

    static LoraModule::MeshManager& Manager()
    {
        static LoraModule::MeshManager manager(Driver(), MillisClock(), Rng(),
                                               RtcClock(), Packets(), Tables());
        return manager;
    }

    static void MeshTaskRunner(void* /*pvParameters*/)
    {
        Manager().Loop();
    }
};

#endif // USE_MESHCORE_LORA
