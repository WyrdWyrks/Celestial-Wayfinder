#pragma once

#include "FastLED.h"

#include "LedPatternInterface.hpp"
#include "LedSegment.hpp"
#include "LedManager.hpp"
#include "DisplayUtilities.hpp"

// Patterns

#include "ButtonFlash.hpp"
#include "IlluminateButton.hpp"
#include "RingPoint.hpp"
#include "RingPulse.hpp"
#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "../../HelperClasses/Led/Patterns/Flashlight.hpp"

#define NUM_COMPASS_LEDS 16
#define NUM_FLASHLIGHT_LEDS 9
#define NUM_LEDS 31
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
23-31: Flashlight
*/

class BootstrapLeds
{
public:
    static void Initialize()
    {
        FastLED.addLeds<LED_TYPE, LED_PIN, LED_ORDER>(LEDBuffer(), NUM_LEDS);

        using UxModule::LedPriority;
        UxModule::LedUtilities::registerPattern(&ButtonFlashPattern(),      LedPriority::FEEDBACK);
        UxModule::LedUtilities::registerPattern(&IlluminateButtonPattern(), LedPriority::MODAL);
        UxModule::LedUtilities::registerPattern(&RingPointPattern(),        LedPriority::BACKGROUND);
        UxModule::LedUtilities::registerPattern(&RingPulsePattern(),        LedPriority::BACKGROUND);
        UxModule::LedUtilities::registerPattern(&ScrollWheelPattern(),      LedPriority::BACKGROUND);
        UxModule::LedUtilities::registerPattern(&FlashlightPattern(),       LedPriority::OVERLAY);

        // Stays enabled; it only claims its LEDs while switched on.
        UxModule::LedUtilities::enablePattern(UxModule::Flashlight::RegisteredPatternID());

        UxModule::LedManager::init(NUM_LEDS, LEDBuffer(), LED_TASK_CPU_CORE);

        // Initialize button flashing animation

        auto buttonFlashPatternID = UxModule::ButtonFlash::RegisteredPatternID();
        UxModule::LedUtilities::enablePattern(buttonFlashPatternID);
        UxModule::LedUtilities::setAnimationLengthMS(buttonFlashPatternID, 300);

        DisplayModule::Utilities::getInputRaised() += [](const DisplayModule::InputContext &ctx) {
            ESP_LOGI(TAG, "Button flash input: %d", ctx.inputID);
            JsonDocument cfg;
            cfg["inputID"] = ctx.inputID;
            auto buttonFlashPatternID = UxModule::ButtonFlash::RegisteredPatternID();
            UxModule::LedUtilities::configurePattern(buttonFlashPatternID, cfg);
            UxModule::LedUtilities::loopPattern(buttonFlashPatternID, 1);
        };
    }

    static CRGB *LEDBuffer() 
    {
        static CRGB leds[NUM_LEDS];
        return leds;
    }

#pragma region LED_Segments

    static UxModule::LedSegment &CompassRingSegment()
    {
        static UxModule::LedSegment compassRing(LEDBuffer(), 0, NUM_COMPASS_LEDS);
        return compassRing;
    }

    static UxModule::LedSegment &InputLedSegment()
    {
        // Initialize input LEDs in order of InputID
        static UxModule::LedSegment inputLeds(LEDBuffer(), {22, 19, 18, 17, 16, 20, 21});
        return inputLeds;
    }

    static UxModule::LedSegment &FlashlightSegment()
    {
        static UxModule::LedSegment flashlight(LEDBuffer(), 23, NUM_FLASHLIGHT_LEDS);
        return flashlight;
    }

#pragma endregion

#pragma region LED_Patterns

    static UxModule::ButtonFlash &ButtonFlashPattern()
    {
        static UxModule::ButtonFlash buttonFlash(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
                DisplayModule::InputID::ENC_UP,
                DisplayModule::InputID::ENC_DOWN
            });
        return buttonFlash;
    }

    static UxModule::IlluminateButton &IlluminateButtonPattern()
    {
        static UxModule::IlluminateButton illuminateButton(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
                DisplayModule::InputID::ENC_UP,
                DisplayModule::InputID::ENC_DOWN
            });
        return illuminateButton;
    }

    static UxModule::RingPoint &RingPointPattern()
    {
        static UxModule::RingPoint ringPoint(CompassRingSegment());
        return ringPoint;
    }

    static UxModule::RingPulse &RingPulsePattern()
    {
        static UxModule::RingPulse ringPulse(CompassRingSegment());
        return ringPulse;
    }

    static UxModule::ScrollWheel &ScrollWheelPattern()
    {
        static UxModule::ScrollWheel scrollWheel(CompassRingSegment());
        return scrollWheel;
    }

    static UxModule::Flashlight &FlashlightPattern()
    {
        static UxModule::Flashlight flashlight(FlashlightSegment());
        return flashlight;
    }

#pragma endregion
};