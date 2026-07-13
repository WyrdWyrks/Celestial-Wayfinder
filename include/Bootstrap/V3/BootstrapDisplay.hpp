#pragma once

#include "BootstrapMicrocontroller.hpp"
#include "CompassUtils.h"
#include "RpcUtils.h"

#include "Adafruit_GFX.h"
#include "Adafruit_SH110X.h"

#include "DisplayManager.hpp"

#define OLED_ADDR 0x3C

class BootstrapDisplay
{
public:

    static constexpr uint8_t AUX_BUTTON_INPUT_ID = 16;

    static constexpr uint16_t OLED_WIDTH = 128;
    static constexpr uint16_t OLED_HEIGHT = 128;

    static void Inititalize()
    {
        // Initialize Driver
        if (BootstrapMicrocontroller::ScannedDevices().count(OLED_ADDR))
        {
            ESP_LOGI(TAG, "Initializing SH1107...");

            auto result = OledDisplay().begin(OLED_ADDR, true);

            DisplayModule::DrawCommand::DrawColorPrimary() = SH110X_WHITE;

            if (result)
            {
                ESP_LOGI(TAG, "SH1107 Initialized.");
                OledDisplay().clearDisplay();
                OledDisplay().setContrast(0x7F);
                OledDisplay().setTextColor(DisplayModule::DrawCommand::DrawColorPrimary());
                OledDisplay().display();
            }
            else
            {
                ESP_LOGW(TAG, "SH1107 Failed to initialize.");
            }

            DisplayDriver() = std::shared_ptr<Adafruit_GFX>(&OledDisplay(), [](Adafruit_GFX*){});

            DisplayModule::Utilities::onRenderComplete += []()
            {
                OledDisplay().display();
            };
        }
        else
        {
            ESP_LOGI(TAG, "Initializing Virtual display driver...");

            VirtualDisplay().setTextColor(0x1);
            DisplayModule::DrawCommand::DrawColorPrimary() = 0x1;

            DisplayDriver() = std::shared_ptr<Adafruit_GFX>(&VirtualDisplay(), [](Adafruit_GFX*){});
        }

        DisplayDriver()->setTextSize(1);
        DisplayDriver()->setCursor(0, 0);

        DisplayManagerInstance().Initialize(DisplayDriver().get(), OLED_WIDTH, OLED_HEIGHT);
        DisplayModule::initDefaultLayers();

        // Wire up input draw commands
        auto windowLayer = std::static_pointer_cast<DisplayModule::WindowLayer>(DisplayModule::Utilities::getLayer(DisplayModule::LayerID::WINDOW));

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_1, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_2, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_3, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::LEFT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::BUTTON_4, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::RIGHT;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_UP, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::TOP;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        windowLayer->registerFactory(DisplayModule::InputID::ENC_DOWN, [](DisplayModule::DrawContext &drawCtx, const std::string &inputText)
        {
            if (inputText.empty()) return;

            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::BOTTOM;

            DisplayModule::TextDrawCommand cmd(inputText, fmt);
            cmd.draw(drawCtx);
        });

        displayCommandQueue = DisplayModule::Utilities::getDisplayCommandQueue();

        // Initialize UI
        CompassUtils::InitializeHomeWindow();

        RpcModule::Utilities::RegisterRpc("GetDisplayContents", _GetDisplayContentsRpc);
    }

    // Display Drivers

    static std::shared_ptr<Adafruit_GFX> &DisplayDriver()
    {
        static std::shared_ptr<Adafruit_GFX> displayDriver = nullptr;
        return displayDriver;
    }

    static GFXcanvas1 &VirtualDisplay()
    {
        static GFXcanvas1 virtualDisplay(OLED_WIDTH, OLED_HEIGHT);
        return virtualDisplay;
    }

    static Adafruit_SH1107 &OledDisplay()
    {
        static Adafruit_SH1107 oledDisplay(OLED_WIDTH, OLED_HEIGHT, &BootstrapMicrocontroller::I2cBus(), BootstrapMicrocontroller::DISPLAY_RESET_PIN);
        return oledDisplay;
    }

    // DisplayModule

    static DisplayModule::Manager &DisplayManagerInstance()
    {
        static DisplayModule::Manager displayManager;
        return displayManager;
    }

private:

    static void _GetDisplayContentsRpc(JsonDocument &doc)
    {
        auto* gfx = DisplayDriver().get();

        doc["width"] = gfx->width();
        doc["height"] = gfx->height();

        // Cast to the concrete type to access getBuffer() — not present on Adafruit_GFX.
        // SH1107 is 1bpp greyscale; GFXcanvas1 is 1bpp — buffer sizes differ.
        uint8_t* displayBuffer = nullptr;
        size_t bufferLength = 0;

        if (gfx == static_cast<Adafruit_GFX*>(&OledDisplay()))
        {
            displayBuffer = OledDisplay().getBuffer();
            // 1 bit per pixel
            bufferLength = (gfx->width() * gfx->height()) / 8;
        }
        else
        {
            displayBuffer = VirtualDisplay().getBuffer();
            // 1 bit per pixel
            bufferLength = (gfx->width() * gfx->height()) / 8;
        }

        if (!displayBuffer)
        {
            ESP_LOGE(TAG, "GetDisplayContents: could not obtain display buffer");
            return;
        }

        // Worst case: SSD1327 128x128 @ 4bpp = 8192 bytes → ~10924 b64 chars.
        unsigned char contents[3000];
        size_t b64_len = 0;
        auto res = mbedtls_base64_encode(contents, sizeof(contents), &b64_len, displayBuffer, bufferLength);
        ESP_LOGI(TAG, "Encoded buffer of size %d into %d b64 bytes (res=%d)", bufferLength, b64_len, res);

        if (res == 0)
        {
            doc["buffer"] = String(contents, b64_len);
        }
    }

};