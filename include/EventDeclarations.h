#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ESP32Encoder.h>
#include <Arduino.h>
// #include "esp_event_base.h"

extern TaskHandle_t inputTaskHandle;
extern TaskHandle_t radioReadTaskHandle;
extern QueueHandle_t displayCommandQueue;

extern ESP32Encoder *inputEncoder;

// #define BIT_SHIFT(x) (1 << x)
#define DEBOUNCE_TIME 10
#define DEBOUNCE_TIME_BUTTONS 250
#define DEBOUNCE_TIME_ENC 100

// ESP_EVENT_DECLARE_BASE(EVENT_BUTTON_IO);

enum
{
    EVENT_BUTTON_1,
    EVENT_BUTTON_2,
    EVENT_BUTTON_3,
    EVENT_BUTTON_4,
    EVENT_ENCODER_UP,
    EVENT_ENCODER_DOWN,
    EVENT_BUTTON_SOS,
    EVENT_MESSAGE_RECEIVED,
};

void IRAM_ATTR button1ISR();
void IRAM_ATTR button2ISR();
void IRAM_ATTR button3ISR();
void IRAM_ATTR button4ISR();
void IRAM_ATTR encButtonISR();
// void IRAM_ATTR buttonSOSISR();
void IRAM_ATTR enc_cb(void *arg);
void IRAM_ATTR CompassDRDYISR();
void IRAM_ATTR LoRaReceiveISR(int packetSize);

void enableInterrupts();
void disableInterrupts();