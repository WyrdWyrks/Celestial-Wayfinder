#pragma once

#include "LedPatternInterface.hpp"

// Sweeps a comet along a short, straight LED trace.
//
// The v3 traces are only 4 LEDs each, so there isn't room for anything subtle:
// a bright head travels the length of the strip with a short fading tail behind
// it. The direction is what carries the meaning — a message arriving flows in,
// a message you send flows out.
//
// Config keys:
//   rOverride/gOverride/bOverride  flow color; falls back to the theme color
//                                  when all three are zero (same convention as
//                                  RingPulse).
//   outward                        true sweeps toward the far end of the strip,
//                                  false toward the near end.
class TraceFlow : public LedPatternInterface
{
public:
    // `reversed` corrects for a strip whose LED order runs opposite the one on
    // the other side of the device, so both traces can be handed the same
    // "outward" flag and still travel in mirrored physical directions.
    TraceFlow(LedSegment segment, bool reversed = false)
        : LedPatternInterface(std::move(segment)), _reversed(reversed)
    {
    }

    void configurePattern(JsonDocument &config) override
    {
        if (!config["rOverride"].isNull()) { _r = config["rOverride"]; }
        if (!config["gOverride"].isNull()) { _g = config["gOverride"]; }
        if (!config["bOverride"].isNull()) { _b = config["bOverride"]; }
        if (config["outward"].is<bool>())  { _outward = config["outward"].as<bool>(); }
    }

    bool iterateFrame() override
    {
        const size_t len = _segment.length();
        if (len == 0 || animationMS == 0) { return true; }

        if (startTime == 0) { startTime = xTaskGetTickCount(); }

        const size_t currMS = pdTICKS_TO_MS(xTaskGetTickCount() - startTime);
        const float progress = (float)currMS / (float)animationMS;

        // The head starts on the first pixel and runs a tail-length past the
        // last one, so the strip finishes dark instead of snapping off mid-sweep.
        const float head = progress * ((float)len + TAIL_LEN);

        const CRGB color = _FlowColor();
        const bool flip  = (_outward != _reversed);

        for (size_t i = 0; i < len; i++)
        {
            // i is the logical position along the flow; flip maps it onto the
            // physical LED order.
            const size_t idx = flip ? (len - 1 - i) : i;

            const float dist = head - (float)i;
            float brightness = 0.0f;
            if (dist >= 0.0f && dist <= TAIL_LEN)
            {
                brightness = 1.0f - (dist / TAIL_LEN);
            }

            _segment[idx] = CRGB(color.r * brightness,
                                 color.g * brightness,
                                 color.b * brightness);
        }

        if (currMS >= animationMS)
        {
            // Also resets startTime, so a repeat trigger starts a fresh sweep.
            clearPattern();
            return true;
        }
        return false;
    }

    void SetRegisteredPatternID(int patternID) override { _patternId = patternID; }
    int  patternID() const { return _patternId; }

protected:
    // Pixels of fading tail behind the head. ~1.5 on a 4-LED strip keeps the
    // head clearly brightest with one dim pixel chasing it.
    static constexpr float TAIL_LEN = 1.5f;

    CRGB _FlowColor() const
    {
        if (_r != 0 || _g != 0 || _b != 0) { return CRGB(_r, _g, _b); }
        return ThemeColor();
    }

    // Per-instance, unlike the class-level static the other patterns use: both
    // traces register their own TraceFlow, and one static could only remember
    // the second of the two IDs.
    int _patternId = -1;

    bool _reversed = false;
    bool _outward  = false;

    uint8_t _r = 0, _g = 0, _b = 0;
};
