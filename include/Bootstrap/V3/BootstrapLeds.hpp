#pragma once

#include "FastLED.h"

#include "LedPatternInterface.hpp"
#include "LedSegment.hpp"
#include "LedManager.hpp"
#include "DisplayUtilities.hpp"
#include "LoraUtils.h"

#include "../../HelperClasses/PingMessage.hpp"

// Patterns

#include "ButtonFlash.hpp"
#include "IlluminateButton.hpp"
#include "RingPoint.hpp"
#include "RingPulse.hpp"
#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "RingShutdown.hpp"
#include "../../HelperClasses/Led/Patterns/Flashlight.hpp"
#include "../../HelperClasses/Led/Patterns/TraceFlow.hpp"



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

The flashlight covers the union of the named segments above (0-52). Indices
past 52 belong to no segment and stay dark.
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

        // Priority decides who owns an LED when segments overlap: the lock
        // screen guide beats everything, the flashlight beats everything else,
        // and press/trace feedback sits above the per-screen indicators.
        using UxModule::LedPriority;
        UxModule::LedUtilities::registerPattern(&ButtonFlashPattern(),      LedPriority::FEEDBACK);
        UxModule::LedUtilities::registerPattern(&IlluminateButtonPattern(), LedPriority::MODAL);
        UxModule::LedUtilities::registerPattern(&RingPointPattern(),        LedPriority::BACKGROUND);
        UxModule::LedUtilities::registerPattern(&RingPulsePattern(),        LedPriority::BACKGROUND);
        UxModule::LedUtilities::registerPattern(&ScrollWheelPattern(),      LedPriority::BACKGROUND);
        UxModule::LedUtilities::registerPattern(&LeftTraceFlowPattern(),    LedPriority::FEEDBACK);
        UxModule::LedUtilities::registerPattern(&RightTraceFlowPattern(),   LedPriority::FEEDBACK);
        UxModule::LedUtilities::registerPattern(&FlashlightPattern(),       LedPriority::OVERLAY);

        UxModule::LedManager::init(NUM_LEDS, LEDBuffer(), LED_TASK_CPU_CORE);

        // Initialize button flashing animation

        auto buttonFlashPatternID = UxModule::ButtonFlash::RegisteredPatternID();
        UxModule::LedUtilities::enablePattern(buttonFlashPatternID);
        UxModule::LedUtilities::setAnimationLengthMS(buttonFlashPatternID, 300);

        // The flashlight stays enabled for the whole session and is switched by
        // its own on/off state: it only claims the strip (isActive) while on,
        // so the Actions-menu toggle just configures it and requests a frame.
        UxModule::LedUtilities::enablePattern(UxModule::Flashlight::RegisteredPatternID());

        // Trace flows — armed here, fired by the LoRa events wired below.
        for (int id : { LeftTraceFlowPattern().patternID(), RightTraceFlowPattern().patternID() })
        {
            UxModule::LedUtilities::enablePattern(id);
            UxModule::LedUtilities::setAnimationLengthMS(id, TRACE_FLOW_MS);
        }

        _WireTraceFlows();

        // The flash always animates; under the flashlight or the lock screen
        // guide it is simply hidden by their higher priority.
        DisplayModule::Utilities::getInputRaised() += [](const DisplayModule::InputContext &ctx) {
            ESP_LOGI(TAG, "Button flash input: %d", ctx.inputID);
            JsonDocument cfg;
            cfg["inputID"] = ctx.inputID;
            auto buttonFlashPatternID = UxModule::ButtonFlash::RegisteredPatternID();
            UxModule::LedUtilities::configurePattern(buttonFlashPatternID, cfg);
            UxModule::LedUtilities::loopPattern(buttonFlashPatternID, 1);
        };

        System_Utils::getEnablePowerSavings() += []() {
            digitalWrite(LED_EN_PIN, LOW);
        };

        System_Utils::getDisablePowerSavings() += []() {
            digitalWrite(LED_EN_PIN, HIGH);

            // Cutting LED_EN drops the strip's state, so whatever was lit
            // (a flashlight left on across a lock, a screen indicator) has to
            // be pushed again on wake.
            UxModule::LedUtilities::refresh();
        };

        // Play the shutdown fade before any other shutdown subscriber (e.g. the
        // MCU bootstrap entering ship mode). PushFront keeps it first regardless
        // of which bootstrap initializes first.
        System_Utils::getSystemShutdown().PushFront([]() {
            UxModule::LedUtilities::playBlocking(ShutdownPattern());
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

    static UxModule::LedSegment &CompassRingSegment()
    {
        static UxModule::LedSegment compassRing(LEDBuffer(), CompassRingIndicies());
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

    static UxModule::LedSegment &EncoderRingSegment()
    {
        static UxModule::LedSegment encoderRing(LEDBuffer(), EncoderRingIndicies());
        return encoderRing;
    }

    static UxModule::LedSegment &InputLedSegment()
    {
        // Initialize input LEDs in order of InputID
        static UxModule::LedSegment inputLeds(
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

    // v3 has no dedicated flashlight LEDs the way v1 and v2 did, so the
    // flashlight is the union of every logical segment — buttons, traces,
    // compass and knob ring all go white together to get as much light out of
    // the device as possible. Registered at OVERLAY priority, it covers every
    // other pattern except the MODAL lock screen guide on the buttons.
    static UxModule::LedSegment &FlashlightSegment()
    {
        static UxModule::LedSegment flashlight = UxModule::LedSegment::Composite({
            InputLedSegment(),
            LeftTraceSegment(),
            CompassRingSegment(),
            EncoderRingSegment(),
            RightTraceSegment(),
        });
        return flashlight;
    }

    static UxModule::LedSegment LeftTraceSegment()
    {
        static UxModule::LedSegment leftTrace(LEDBuffer(), LED_IDX_LEFT_TRACE, NUM_TRACE_LEDS);
        return leftTrace;
    }

    static UxModule::LedSegment RightTraceSegment()
    {
        static UxModule::LedSegment rightTrace(LEDBuffer(), LED_IDX_RIGHT_TRACE, NUM_TRACE_LEDS);
        return rightTrace;
    }

    // The two traces run up opposite sides of the device. Both strips turned
    // out to be wired the same way round — increasing LED index is "outward" on
    // each — so both carry the reversed flag. It stays per-strip rather than
    // being folded into the pattern so a board revision that flips one side
    // only has to change the flag here. Verified on hardware.
    static UxModule::TraceFlow &LeftTraceFlowPattern()
    {
        static UxModule::TraceFlow leftFlow(LeftTraceSegment(), /*reversed=*/true);
        return leftFlow;
    }

    static UxModule::TraceFlow &RightTraceFlowPattern()
    {
        static UxModule::TraceFlow rightFlow(RightTraceSegment(), /*reversed=*/true);
        return rightFlow;
    }

    // Fires both traces together. outward = a message leaving this device,
    // inward = one arriving. A fully black color falls back to the theme color.
    // Runs on the mesh task; safe because UxModule::LedUtilities locks. Under the
    // flashlight the flow still runs, hidden by its higher priority.
    static void PlayTraceFlow(CRGB color, bool outward)
    {
        JsonDocument cfg;
        cfg["rOverride"] = color.r;
        cfg["gOverride"] = color.g;
        cfg["bOverride"] = color.b;
        cfg["outward"]   = outward;

        for (int id : { LeftTraceFlowPattern().patternID(), RightTraceFlowPattern().patternID() })
        {
            UxModule::LedUtilities::configurePattern(id, cfg);
            UxModule::LedUtilities::loopPattern(id, 1);
        }
    }

    static UxModule::ButtonFlash &ButtonFlashPattern()
    {
        static UxModule::ButtonFlash buttonFlash(
            InputLedSegment(), 
            {
                DisplayModule::InputID::BUTTON_1,
                DisplayModule::InputID::BUTTON_2,
                DisplayModule::InputID::BUTTON_3,
                DisplayModule::InputID::BUTTON_4,
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
            });
        return illuminateButton;
    }

    static UxModule::RingPoint &RingPointPattern()
    {
        static UxModule::RingPoint ringPoint(CompassRingSegment());
        return ringPoint;
    }

    static UxModule::RingPoint &EncoderPointPattern()
    {
        static UxModule::RingPoint encoderPoint(CompassRingSegment());
        return encoderPoint;
    }

    static UxModule::RingPulse &RingPulsePattern()
    {
        static UxModule::RingPulse ringPulse(CompassRingSegment());
        return ringPulse;
    }

    static UxModule::RingPulse &EncoderPulsePattern()
    {
        static UxModule::RingPulse encoderPulse(EncoderRingSegment());
        return encoderPulse;
    }

    static UxModule::ScrollWheel &ScrollWheelPattern()
    {
        static UxModule::ScrollWheel scrollWheel(EncoderRingSegment());
        return scrollWheel;
    }

    static UxModule::Flashlight &FlashlightPattern()
    {
        static UxModule::Flashlight flashlight(FlashlightSegment());
        return flashlight;
    }

    static UxModule::RingShutdown &ShutdownPattern()
    {
        static UxModule::RingShutdown shutdown(CompassRingSegment());
        return shutdown;
    }

private:
    static constexpr size_t TRACE_FLOW_MS = 500;

    // Subscribing here rather than at the send/receive call sites keeps this
    // entirely inside the v3 bootstrap — v1 and v2 have no trace LEDs and need
    // no version guards sprinkled through the shared code.
    static void _WireTraceFlows()
    {
        // Inbound: a new message flows in wearing the sender's color.
        LoraModule::Utilities::MessageTypeReceived(PingMessage::GUID) +=
            [](std::shared_ptr<LoraModule::LoraMessageInterface> msg, bool isNew)
            {
                if (!isNew) { return; }

                auto ping = std::static_pointer_cast<PingMessage>(msg);
                if (!ping) { return; }

                PlayTraceFlow(CRGB(ping->color_R, ping->color_G, ping->color_B),
                              /*outward=*/false);
            };

        // Outbound: fires when this device queues a broadcast of its own.
        // Retransmit attempts happen inside the send task without re-entering
        // SendMessage, so one user-initiated send is one flow.
        LoraModule::Utilities::MyLastBroadcastChanged() += []()
        {
            PlayTraceFlow(UxModule::LedUtilities::ThemeColor(), /*outward=*/true);
        };
    }
};