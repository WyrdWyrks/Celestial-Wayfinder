#include "EventDeclarations.h"
#include "HelperClasses/LoRaDriver/ArduinoLoRaDriver.h"
#include "LED_Manager.h"
#include "Display_Manager.h"
#include "esp_log.h"
#include "DisplayUtilities.hpp"

TaskHandle_t inputTaskHandle;
TaskHandle_t radioReadTaskHandle;
QueueHandle_t displayCommandQueue;

ESP32Encoder *inputEncoder;

void IRAM_ATTR button1ISR()
{
    static TickType_t lastISRTime = 0;
    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_BUTTONS)
    {
        return;
    }

    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;
    command.commandData.inputCommand.inputID = DisplayModule::InputID::BUTTON_1;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button2ISR()
{
    static TickType_t lastISRTime = 0;
    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_BUTTONS)
    {
        return;
    }

    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;
    command.commandData.inputCommand.inputID = DisplayModule::InputID::BUTTON_2;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button3ISR()
{
    static TickType_t lastISRTime = 0;
    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_BUTTONS)
    {
        return;
    }

    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;
    command.commandData.inputCommand.inputID = DisplayModule::InputID::BUTTON_3;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR button4ISR()
{
    static TickType_t lastISRTime = 0;
    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_BUTTONS)
    {
        return;
    }

    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;
    command.commandData.inputCommand.inputID = DisplayModule::InputID::BUTTON_4;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR encButtonISR()
{
    static TickType_t lastISRTime = 0;
    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_BUTTONS)
    {
        return;
    }

    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;
    command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_BUTTON;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

// void IRAM_ATTR buttonSOSISR()
// {
//     static TickType_t lastISRTime = 0;
//     if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_BUTTONS)
//     {
//         return;
//     }

//     lastISRTime = xTaskGetTickCount();

//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     DisplayModule::DisplayCommandQueueItem command;
//     command.commandType = DisplayModule::CommandType::INPUT_COMMAND;
//     command.commandData.inputCommand.inputID = BUTTON_SOS;
//     xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
//     if (xHigherPriorityTaskWoken)
//     {
//         portYIELD_FROM_ISR();
//     }
// }

#if HARDWARE_VERSION == 1

void IRAM_ATTR enc_cb(void *arg)
{
    static int64_t prevCount = 0;
    static TickType_t lastISRTime = 0;

    ESP32Encoder *enc = ESP32Encoder::encoders[0];
    int64_t currCount = enc->getCount();

    if (currCount % 2 != 0) { return; }

    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_ENC) { return; }
    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;

    if (currCount > prevCount)
    {
        command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_DOWN;
    }
    else if (currCount < prevCount)
    {
        command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_UP;
    }
    else { return; }

    prevCount = currCount;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) { portYIELD_FROM_ISR(); }
}

#elif HARDWARE_VERSION == 2

void IRAM_ATTR enc_cb(void *arg)
{
    static int64_t prevCount = 0;
    static TickType_t lastISRTime = 0;

    ESP32Encoder *enc = ESP32Encoder::encoders[0];
    int64_t currCount = enc->getCount();

    if (currCount % 2 != 0) { return; }

    if (xTaskGetTickCount() - lastISRTime < DEBOUNCE_TIME_ENC) { return; }
    lastISRTime = xTaskGetTickCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;

    if (currCount > prevCount)
    {
        command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_UP;
    }
    else if (currCount < prevCount)
    {
        command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_DOWN;
    }
    else { return; }

    prevCount = currCount;
    xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) { portYIELD_FROM_ISR(); }
}

#elif HARDWARE_VERSION == 3

void IRAM_ATTR enc_cb(void *arg)
{
    // FullQuad counts every quadrature edge; this encoder yields COUNTS_PER_DETENT counts per
    // physical click. Emit one event per whole detent and advance prevCount by exactly that
    // amount so the reference stays aligned to the settled detent position — that keeps the
    // first click after a direction reversal from being lost. The PCNT hardware glitch filter
    // (setFilter) rejects contact bounce, so no time-based debounce is needed.
    static int64_t prevCount = 0;
    constexpr int64_t COUNTS_PER_DETENT = 2;

    ESP32Encoder *enc = ESP32Encoder::encoders[0];
    int64_t currCount = enc->getCount();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    DisplayModule::DisplayCommandQueueItem command;
    command.commandType = DisplayModule::CommandType::INPUT_COMMAND;

    bool emitted = false;

    // Drain whole detents in each direction; the loops let a fast spin emit multiple events
    // instead of dropping the extra motion.
    while (currCount - prevCount >= COUNTS_PER_DETENT)
    {
        prevCount += COUNTS_PER_DETENT;
        command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_UP;
        xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
        emitted = true;
    }
    while (currCount - prevCount <= -COUNTS_PER_DETENT)
    {
        prevCount -= COUNTS_PER_DETENT;
        command.commandData.inputCommand.inputID = DisplayModule::InputID::ENC_DOWN;
        xQueueSendFromISR(displayCommandQueue, &command, &xHigherPriorityTaskWoken);
        emitted = true;
    }

    if (emitted && xHigherPriorityTaskWoken) { portYIELD_FROM_ISR(); }
}

#endif

void IRAM_ATTR CompassDRDYISR()
{
#if DEBUG == 1
    // ESP_EARLY_LOGD(TAG, "CompassDRDYISR");
#endif
    // Navigation_Manager::read();
}

void IRAM_ATTR LoRaReceiveISR(int packetSize)
{
    ESP_EARLY_LOGI(TAG, "LoRaReceiveISR: %d", packetSize);
    if (radioReadTaskHandle != nullptr)
    {
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(radioReadTaskHandle, &higherPriorityTaskWoken);
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

void IRAM_ATTR ArduinoLoRaDriver::_onCadDone(bool channelBusy)
{
    if (_instance == nullptr) { return; }
    _instance->_cadResult = channelBusy;
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(_cadSemaphore, &woken);
    portYIELD_FROM_ISR(woken);
}

// void enableInterrupts()
// {
//     attachInterrupt(BUTTON_SOS_PIN, buttonSOSISR, FALLING);
//     attachInterrupt(BUTTON_1_PIN, button1ISR, FALLING);
//     attachInterrupt(BUTTON_2_PIN, button2ISR, FALLING);
//     attachInterrupt(BUTTON_3_PIN, button3ISR, FALLING);
//     attachInterrupt(BUTTON_4_PIN, button4ISR, FALLING);

//     inputEncoder->attachFullQuad(ENC_A, ENC_B);
//     inputEncoder->setFilter(1023);
// }

// void disableInterrupts()
// {
//     detachInterrupt(BUTTON_1_PIN);
//     detachInterrupt(BUTTON_2_PIN);
//     detachInterrupt(BUTTON_3_PIN);
//     detachInterrupt(BUTTON_4_PIN);
//     detachInterrupt(BUTTON_SOS_PIN);

//     inputEncoder->detatch();
// }