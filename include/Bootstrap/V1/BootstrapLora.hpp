#pragma once

#include "Bootstrap/V1/BootstrapMicrocontroller.hpp"
#include "LoraManager.hpp"
#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"
#include "HelperClasses/PingMessage.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "EventDeclarations.h"

namespace
{
    const uint8_t V1_LORA_CS   = 15;
    const int     V1_LORA_RST  = -1;
    const uint8_t V1_LORA_DIO0 = 18;
    const uint8_t V1_LORA_TX   = 20;
}

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
        Driver().SetTXPower(V1_LORA_TX);

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
        static ArduinoLoRaDriver driver(&BootstrapMicrocontroller::SpiBus(), V1_LORA_CS, V1_LORA_RST, V1_LORA_DIO0,
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
