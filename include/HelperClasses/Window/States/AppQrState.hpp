#pragma once

#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "TextDrawCommand.hpp"
#include "QrCodeDrawCommand.hpp"
#include "HelperClasses/PwaQr.hpp"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // AppQrState
    // -------------------------------------------------------------------------
    // Companion-app QR code with the URL underneath. Scales the QR to 2x on
    // the 128x128 V3 panel and 1x on the 128x64 V2 panel.

    class AppQrState : public WindowState
    {
    public:
        void onEnter(const StateTransferData &) override
        {
            clearDrawCommands();

            auto bottom = Utilities::selectBottomTextLine();
            bool tall   = bottom >= 16;

            if (tall)
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "Set up in the app:", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, 3 }));
            }

            addDrawCommand(std::make_shared<QrCodeDrawCommand>(
                pwa_qr, PWA_QR_MODULES, tall ? 2 : 1,
                tall ? QrFormat(TextAlignH::CENTER, TextAlignV::CENTER)
                     : QrFormat(TextAlignH::CENTER, TextAlignV::TOP, 0, 8))); // below the top label row

            // Last line is shared with the bottom corner labels, so the URL
            // sits on the two lines above it.
            addDrawCommand(std::make_shared<TextDrawCommand>(
                "wayfinder.", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(bottom - 2) }));
            addDrawCommand(std::make_shared<TextDrawCommand>(
                "wyrdwyrks.com", TextFormat{ TextAlignH::CENTER, TextAlignV::LINE, static_cast<uint8_t>(bottom - 1) }));
        }
    };

} // namespace DisplayModule
