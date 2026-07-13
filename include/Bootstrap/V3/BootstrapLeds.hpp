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



#undef NUM_LEDS
#define NUM_LEDS 61
#define LED_EN_PIN 15
#define LED_PIN 16
#define LED_ORDER GRB
#define LED_TYPE WS2812B
#define LED_TASK_CPU_CORE 0

#define LED_IDX_ENCODER_RING 41
#define NUM_ENCODER_LEDS 8

#define LED_IDX_COMPASS_RING 17
#undef NUM_COMPASS_LEDS
#define NUM_COMPASS_LEDS 32

#define LED_IDX_LEFT_TRACE 5
#define LED_IDX_RIGHT_TRACE 49
#define NUM_TRACE_LEDS 4

#define LED_IDX_POWER_BUTTON 0
#define LED_IDX_BUTTON_1 4
#define LED_IDX_BUTTON_2 3
#define LED_IDX_BUTTON_3 1
#define LED_IDX_BUTTON_4 2

/*
LED Mappings:
0:      Power Button
1:      Button 3
2:      Button 4
3:      Button 2
4:      Button 1
5-12:   Screen Light
13-16:  Left Trace
17-48:  Compass (Counter-clockwise)
49-56:  Knob ring (Counter-clockwise)
57-60:  Right Trace

LED Mappings Post Screen Light Removal:
TODO: Remap LEDs after bodging
0:      Power Button
1:      Button 3
2:      Button 4
3:      Button 2
4:      Button 1
5-8:  Left Trace
9-40:  Compass (Counter-clockwise)
41-48:  Knob ring (Counter-clockwise)
49-52:  Right Trace

*/

class BootstrapLeds
{
public:
    static void Initialize()
    {
        ESP_LOGI(TAG, "Initializing LEDs");
        FastLED.addLeds<LED_TYPE, LED_PIN, LED_ORDER>(LEDBuffer(), NUM_LEDS);
        pinMode(LED_EN_PIN, OUTPUT);
        digitalWrite(LED_EN_PIN, HIGH);

        LED_Utils::registerPattern(&ButtonFlashPattern());
        LED_Utils::registerPattern(&IlluminateButtonPattern());
        LED_Utils::registerPattern(&RingPointPattern());
        LED_Utils::registerPattern(&RingPulsePattern());
        LED_Utils::registerPattern(&ScrollWheelPattern());

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

        System_Utils::getEnablePowerSavings() += []() {
            digitalWrite(LED_EN_PIN, LOW);
        };

        System_Utils::getDisablePowerSavings() += []() {
            digitalWrite(LED_EN_PIN, HIGH);
        };

        // Play the shutdown fade before any other shutdown subscriber (e.g. the
        // MCU bootstrap entering ship mode). PushFront keeps it first regardless
        // of which bootstrap initializes first.
        System_Utils::getSystemShutdown().PushFront([]() {
            LedPatternInterface::PlayBlocking(ShutdownPattern());
        });
    }

    static CRGB *LEDBuffer() 
    {
        static CRGB leds[NUM_LEDS];
        return leds;
    }

    static std::vector<size_t> &CompassRingIndicies()
    {
        static std::vector<size_t> compassRingIndicies = 
        {
            9, 40, 39, 38, 37, 36, 35, 34, 
            33, 32, 31, 30, 29, 28, 27, 26, 
            25, 24, 23, 22, 21, 20, 19, 18,
            17, 16, 15, 14, 13, 12, 11, 10
        };

        return compassRingIndicies;
    }

    static LedSegment &CompassRingSegment()
    {
        static LedSegment compassRing(LEDBuffer(), CompassRingIndicies());
        return compassRing;
    }

    static std::vector<size_t> EncoderRingIndicies()
    {
        static std::vector<size_t> encoderRingIndicies = 
        {
            41, 48, 47, 46,
            45, 44, 43, 42
        };

        return encoderRingIndicies;
    }

    static LedSegment &EncoderRingSegment()
    {
        static LedSegment encoderRing(LEDBuffer(), EncoderRingIndicies());
        return encoderRing;
    }

    static LedSegment &InputLedSegment()
    {
        // Initialize input LEDs in order of InputID
        static LedSegment inputLeds(
            LEDBuffer(),
            {
                LED_IDX_BUTTON_1, 
                LED_IDX_BUTTON_2, 
                LED_IDX_BUTTON_3, 
                LED_IDX_BUTTON_4, 
                LED_IDX_POWER_BUTTON
            });
        return inputLeds;
    }

    static LedSegment LeftTraceSegment()
    {
        static LedSegment leftTrace(LEDBuffer(), LED_IDX_LEFT_TRACE, NUM_TRACE_LEDS);    
        return leftTrace;
    }

    static LedSegment RightTraceSegment()
    {
        static LedSegment rightTrace(LEDBuffer(), LED_IDX_RIGHT_TRACE, NUM_TRACE_LEDS);    
        return rightTrace;
    }

    static ButtonFlash &ButtonFlashPattern()
    {
        static ButtonFlash buttonFlash(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
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
            });
        return illuminateButton;
    }

    static RingPoint &RingPointPattern()
    {
        static RingPoint ringPoint(CompassRingSegment());
        return ringPoint;
    }

    static RingPoint &EncoderPointPattern()
    {
        static RingPoint encoderPoint(CompassRingSegment());
        return encoderPoint;
    }

    static RingPulse &RingPulsePattern()
    {
        static RingPulse ringPulse(CompassRingSegment());
        return ringPulse;
    }

    static RingPulse &EncoderPulsePattern()
    {
        static RingPulse encoderPulse(EncoderRingSegment());
        return encoderPulse;
    }

    static ScrollWheel &ScrollWheelPattern()
    {
        static ScrollWheel scrollWheel(EncoderRingSegment());
        return scrollWheel;
    }

    static RingShutdown &ShutdownPattern()
    {
        static RingShutdown shutdown(CompassRingSegment());
        return shutdown;
    }
};