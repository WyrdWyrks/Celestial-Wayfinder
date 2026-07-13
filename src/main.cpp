#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "EventDeclarations.h"
#include <ESP32Encoder.h>
#include "globalDefines.h"

#include <ArduinoJson.hpp>

#include "NavigationManager.h"

#include "CompassUtils.h"

#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "RingPoint.hpp"
#include "IlluminateButton.hpp"
#include "RingPulse.hpp"

#include "HelperClasses/PingMessage.hpp"

#include <unordered_map>

#if HARDWARE_VERSION == 1
    #include "Bootstrap/V1/BootstrapMicrocontroller.hpp"
    #include "Bootstrap/V1/BootstrapLeds.hpp"
    #include "Bootstrap/V1/BootstrapNavigation.hpp"
    #include "Bootstrap/V1/BootstrapDisplay.hpp"
    #include "Bootstrap/V1/BootstrapLora.hpp"
#elif HARDWARE_VERSION == 2
    #include "Bootstrap/V2/BootstrapMicrocontroller.hpp"
    #include "Bootstrap/V2/BootstrapLeds.hpp"
    #include "Bootstrap/V2/BootstrapNavigation.hpp"
    #include "Bootstrap/V2/BootstrapDisplay.hpp"
    #include "Bootstrap/V2/BootstrapLora.hpp"
#elif HARDWARE_VERSION == 3
    #include "Bootstrap/V3/BootstrapMicrocontroller.hpp"
    #include "Bootstrap/V3/BootstrapLeds.hpp"
    #include "Bootstrap/V3/BootstrapNavigation.hpp"
    #include "Bootstrap/V3/BootstrapDisplay.hpp"
    #include "Bootstrap/V3/BootstrapLora.hpp"
#else
    #error "Unknown HARDWARE_VERSION. Must be 1, 2, or 3."
#endif

#include "Bootstrap/Common/BootstrapRpc.hpp"

extern "C"
{
#include "bootloader_random.h"
}

void enableInterruptsHandler();
void disableInterruptsHandler();
void enterUselessLoop();
void Bootstrap();

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(3000));

  bootloader_random_enable();
  
  // Boostrap hardware modules and utilities
  Bootstrap();
  
  vTaskDelay(300);

  // CompassUtils::WireFunctions();

  // vTaskDelay(1000 / portTICK_PERIOD_MS);

  System_Utils::getEnableInterrupts() += BootstrapMicrocontroller::EnableInterrupts;
  System_Utils::getDisableInterrupts() += BootstrapMicrocontroller::DisableInterrupts;

  System_Utils::enableInterruptsInvoke();

}

void loop()
{
  vTaskDelay(600000 / portTICK_PERIOD_MS);
}

void enterUselessLoop()
{
  auto counter = 0;

  while(true)
  {
    ESP_LOGI("SETUP", "Infinite loop: %d\n", counter);
    vTaskDelay(3000);
  }
}

void Bootstrap()
{
  ESP_LOGI(TAG, "Initializing Hardware Version %d", HARDWARE_VERSION);

  CompassUtils::InitializeSettings();

  BootstrapMicrocontroller::Initialize();
  BootstrapLeds::Initialize();
  BootstrapNavigation::Initialize();
  BootstrapDisplay::Inititalize();
  BootstrapLora::Initialize();
  BootstrapRpc::Initialize();
}

