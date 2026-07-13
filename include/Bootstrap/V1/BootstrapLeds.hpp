#pragma once

#include "FastLED.h"

#include "LedPatternInterface.hpp"
#include "LedSegment.hpp"
#include "LED_Manager.h"
#include "DisplayUtilities.hpp"

// Patterns

#include "ButtonFlash.hpp"
#include "IlluminateButton.hpp"
#include "RingPoint.hpp"
#include "RingPulse.hpp"
#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "RingShutdown.hpp"
#include "../../HelperClasses/Led/Patterns/Flashlight.hpp"

#define NUM_COMPASS_LEDS 16
#define NUM_FLASHLIGHT_LEDS 8
#define NUM_LEDS 30
#define LED_PIN 27
#define LED_ORDER GRB
#define LED_TYPE WS2812B
#define LED_TASK_CPU_CORE 0

/*
LED Mappings:
0-15: compass ring
16: SOS button
17: Button 4
18: Button 3
19: Button 2
20: Encoder up
21: Encoder down
22: Button 1
23-30: Flashlight
*/

class BootstrapLeds
{
public:
    static void Initialize()
    {
        FastLED.addLeds<LED_TYPE, LED_PIN, LED_ORDER>(LEDBuffer(), NUM_LEDS);

        LED_Utils::registerPattern(&ButtonFlashPattern());
        LED_Utils::registerPattern(&IlluminateButtonPattern());
        LED_Utils::registerPattern(&RingPointPattern());
        LED_Utils::registerPattern(&RingPulsePattern());
        LED_Utils::registerPattern(&ScrollWheelPattern());
        LED_Utils::registerPattern(&FlashlightPattern());

        LED_Manager::init(NUM_LEDS, LEDBuffer(), LED_TASK_CPU_CORE);

        // Initialize button flashing animation

        auto buttonFlashPatternID = ButtonFlash::RegisteredPatternID();
        LED_Utils::enablePattern(buttonFlashPatternID);
        LED_Utils::setAnimationLengthMS(buttonFlashPatternID, 300);

        DisplayModule::Utilities::getInputRaised() += [](const DisplayModule::InputContext &ctx) {
            ESP_LOGI(TAG, "Button flash input: %d", ctx.inputID);
            JsonDocument cfg;
            cfg["inputID"] = ctx.inputID;
            auto buttonFlashPatternID = ButtonFlash::RegisteredPatternID();
            LED_Utils::configurePattern(buttonFlashPatternID, cfg);
            LED_Utils::loopPattern(buttonFlashPatternID, 1);
        };

        // Play the shutdown fade before any other shutdown subscriber (e.g. the
        // MCU bootstrap cutting power). PushFront keeps it first regardless of
        // which bootstrap initializes first.
        System_Utils::getSystemShutdown().PushFront([]() {
            LedPatternInterface::PlayBlocking(ShutdownPattern());
        });
    }

    static CRGB *LEDBuffer() 
    {
        static CRGB leds[NUM_LEDS];
        return leds;
    }

#pragma region LED_Segments

    static LedSegment &CompassRingSegment()
    {
        static LedSegment compassRing(LEDBuffer(), 0, NUM_COMPASS_LEDS);
        return compassRing;
    }

    static LedSegment &InputLedSegment()
    {
        // Initialize input LEDs in order of InputID
        static LedSegment inputLeds(LEDBuffer(), {22, 19, 18, 17, 16, 20, 21});
        return inputLeds;
    }

    static LedSegment &FlashlightSegment()
    {
        static LedSegment flashlight(LEDBuffer(), 23, NUM_FLASHLIGHT_LEDS);
        return flashlight;
    }

#pragma endregion

#pragma region LED_Patterns

    static ButtonFlash &ButtonFlashPattern()
    {
        static ButtonFlash buttonFlash(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
                BUTTON_SOS,
                DisplayModule::InputID::ENC_UP,
                DisplayModule::InputID::ENC_DOWN
            });
        return buttonFlash;
    }

    static IlluminateButton &IlluminateButtonPattern()
    {
        static IlluminateButton illuminateButton(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
                BUTTON_SOS,
                DisplayModule::InputID::ENC_UP,
                DisplayModule::InputID::ENC_DOWN
            });
        return illuminateButton;
    }

    static RingPoint &RingPointPattern()
    {
        static RingPoint ringPoint(CompassRingSegment());
        return ringPoint;
    }

    static RingPulse &RingPulsePattern()
    {
        static RingPulse ringPulse(CompassRingSegment());
        return ringPulse;
    }

    static ScrollWheel &ScrollWheelPattern()
    {
        static ScrollWheel scrollWheel(CompassRingSegment());
        return scrollWheel;
    }

    static Flashlight &FlashlightPattern()
    {
        static Flashlight flashlight(FlashlightSegment());
        return flashlight;
    }

    static RingShutdown &ShutdownPattern()
    {
        static RingShutdown shutdown(CompassRingSegment());
        return shutdown;
    }

#pragma endregion
};