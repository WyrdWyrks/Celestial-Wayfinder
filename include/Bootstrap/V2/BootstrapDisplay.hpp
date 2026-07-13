#pragma once

#include "BootstrapMicrocontroller.hpp"
#include "CompassUtils.h"
#include "RpcUtils.h"

#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"

#include "DisplayManager.hpp"

#define OLED_ADDR 0x3C

class BootstrapDisplay
{
public:

    static constexpr uint8_t AUX_BUTTON_INPUT_ID = 16;

    static constexpr uint16_t OLED_WIDTH = 128;
    static constexpr uint16_t OLED_HEIGHT = 64;

    static void Inititalize()
    {
        // Initialize Driver
        if (BootstrapMicrocontroller::ScannedDevices().count(OLED_ADDR))
        {
            ESP_LOGI(TAG, "Initializing SSD1306...");

            auto result = OledDisplay().begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

            if (result)
            {
                ESP_LOGI(TAG, "SSD1306 Initialized.");
                OledDisplay().clearDisplay();
                OledDisplay().setTextColor(SSD1306_WHITE);
                OledDisplay().display();
            }
            else
            {
                ESP_LOGW(TAG, "SSD1306 Failed to initialize.");
            }

            DisplayDriver() = std::shared_ptr<Adafruit_GFX>(&OledDisplay(), [](Adafruit_GFX*){});

            DisplayModule::DrawCommand::DrawColorPrimary() = SSD1306_WHITE;

            DisplayModule::Utilities::onRenderComplete += []()
            {
                OledDisplay().display();
            };
        }
        else
        {
            ESP_LOGI(TAG, "Initializing Virtual display driver...");

            VirtualDisplay().setTextColor(WHITE);
            DisplayModule::DrawCommand::DrawColorPrimary() = SSD1306_WHITE;

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

    static Adafruit_SSD1306 &OledDisplay()
    {
        static Adafruit_SSD1306 oledDisplay(OLED_WIDTH, OLED_HEIGHT, &BootstrapMicrocontroller::I2cBus(), -1);
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
        uint8_t* displayBuffer = nullptr;
        if (gfx == static_cast<Adafruit_GFX*>(&OledDisplay()))
        {
            displayBuffer = OledDisplay().getBuffer();
        }
        else
        {
            displayBuffer = VirtualDisplay().getBuffer();
        }

        if (!displayBuffer)
        {
            ESP_LOGE(TAG, "GetDisplayContents: could not obtain display buffer");
            return;
        }

        // SSD1306 / GFXcanvas1 are both 1bpp
        size_t bufferLength = (gfx->width() * gfx->height()) / 8;

        // V2: 128x64 = 1024 bytes → ~1368 b64 chars; 3000 is sufficient.
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