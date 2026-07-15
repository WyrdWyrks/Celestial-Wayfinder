#pragma once

#include <FastLED.h>

#include "LoraManager.hpp"
#include "FilesystemUtils.h"
#include "RpcUtils.h"
#include "LED_Utils.h"
#include "LED_Manager.h"
#include "FilesystemManager.h"
#include "EspNowManager.h"
#include "Display_Manager.h"
#include "DisplayManager.hpp"
#include "WindowFactories.hpp"

#include "RpcManager.h"
#include "SettingsInterface.hpp"
#include "BluetoothUtilities.hpp"

#include "HelperClasses/WayfinderLoraState.hpp"

#include "ArduinoJson.h"
#include "esp_log.h"

// Window Includes

#include "HelperClasses/Window/HomeWindow.hpp"
#include "HelperClasses/Window/CompassWindow.hpp"
#if HARDWARE_VERSION >= 3
    #include "HelperClasses/Window/BreakoutWindow.hpp"
#endif
#include "GeolocationDebugWindow.hpp"
#include "DiagnosticsWindow.hpp"
#include "EditSavedLocationsWindow.hpp"
#include "HelperClasses/Window/EditStatusMessagesWindow.hpp"
#include "HelperClasses/Window/WiFiRpcWindow.hpp"
#include "HelperClasses/Window/PairBluetoothWindow.hpp"

static const char *TAG_COMPASS = "COMPASS";

namespace
{
    static RpcModule::Manager RpcManagerInstance;
    static ConnectivityModule::EspNowManager EspNowManagerInstance;
    FilesystemModule::Manager filesystemManagerInstance;
    static AsyncWebServer WebServerInstance(80);
    static AsyncCorsMiddleware cors;
    static std::shared_ptr<DisplayModule::HomeWindow> _homeWindowInstance;
}

// Static class to help interface with esp32 utils compass functionality
// Eventually, all windows pertaining to the compass specific functionality will reference this class
class CompassUtils
{
public:

    static uint8_t MessageReceivedInputID;

    static void PassMessageReceivedToDisplay(std::shared_ptr<LoraModule::LoraMessageInterface> msg, bool isNew)
    {
        if (isNew)
        {
            #if DEBUG == 1
            ESP_LOGI(TAG_COMPASS, "PassMessageReceivedToDisplay: New message received");
            #endif
            
            // Check if we're on home window
            if (_homeWindowInstance == DisplayModule::Utilities::activeWindow())
            {
                ESP_LOGI(TAG, "Refreshing home window");
                _homeWindowInstance->onMessageReceived();
                DisplayModule::Utilities::render();
            }

            if (System_Utils::silentMode == false)
            {
                LED_Manager::buzzerNotification();
            }
        }
        #if DEBUG == 1
        else
        {
            ESP_LOGI(TAG_COMPASS, "PassMessageReceivedToDisplay: Old message received");
        }
        #endif

        // TODO: Maybe add refresh display command if not new
    }


    static void InitializeSettings()
    {   
        filesystemManagerInstance.InitializeFilesystem();

        CheckDeviceInfo();
        auto settings = GenerateSettings();
        FilesystemModule::Utilities::DeviceSettings() = std::move(settings);

        FilesystemModule::Utilities::SettingsUpdated() += ProcessSettingsFile;
        FilesystemModule::Utilities::SettingsUpdated() += ConnectivityModule::Utilities::ProcessSettings;
        FilesystemModule::Utilities::SettingsUpdated() += System_Utils::UpdateSettings;
        FilesystemModule::Utilities::SettingsUpdated() += Bluetooth_Utils::SettingsUpdated;
        FilesystemModule::Utilities::SettingsUpdated() += LoraModule::Utilities::UpdateSettings;

        FilesystemModule::Utilities::RequestSettingsRefresh() += []()
        {
            FilesystemModule::Utilities::InvokeSettingsUpdated(FilesystemModule::Utilities::DeviceSettings());
        };

        FilesystemModule::Utilities::RequestSettingsRefresh().Invoke();
    }

    static void CheckDeviceInfo()
    {
        auto& deviceInfo = FilesystemModule::Utilities::DeviceInfo();
        deviceInfo.begin("DeviceInfo", false);

        if (!deviceInfo.isKey("UserID"))
        {
            uint32_t userID = esp_random();
            deviceInfo.putUInt("UserID", userID);
            ESP_LOGI(TAG_COMPASS, "Generated new UserID: %u", userID);
        }

        // Not sure these are necessary
        if (!deviceInfo.isKey("Firmware") || deviceInfo.getString("Firmware").c_str() != std::string(FIRMWARE_VERSION_STRING))
        {
            deviceInfo.putString("Firmware", FIRMWARE_VERSION_STRING);
            ESP_LOGI(TAG_COMPASS, "Set Firmware Version: %s", FIRMWARE_VERSION_STRING);
        }

        if (deviceInfo.getInt("Hardware", -1) != HARDWARE_VERSION)
        {
            deviceInfo.putInt("Hardware", HARDWARE_VERSION);
            ESP_LOGI(TAG_COMPASS, "Set Hardware Version: %d", HARDWARE_VERSION);
        }

        System_Utils::DeviceID = deviceInfo.getUInt("UserID");
        deviceInfo.end();
    }

    static std::vector<std::shared_ptr<FilesystemModule::SettingsInterface>> GenerateSettings()
    {
        std::vector<std::shared_ptr<FilesystemModule::SettingsInterface>> settings;

        std::string defaultDeviceName;
        {
            char devicenamebuffer[20];
            FilesystemModule::Utilities::DeviceInfo().begin("DeviceInfo", true);
            sprintf(devicenamebuffer, "Beacon_%04X", (unsigned int)(FilesystemModule::Utilities::DeviceInfo().getUInt("UserID") & 0xFFFF));
            FilesystemModule::Utilities::DeviceInfo().end();
            defaultDeviceName = devicenamebuffer;
        }

        auto userName = std::make_shared<FilesystemModule::StringSetting>("User Name", "User", 12);
        settings.push_back(userName);

        auto deviceName = std::make_shared<FilesystemModule::StringSetting>("Device Name", defaultDeviceName, 20);
        settings.push_back(deviceName);

        std::vector<std::string> colorThemeOptions = {"Custom", "Red", "Green", "Blue", "Purple", "Yellow", "Cyan", "White", "Orange"};
        std::vector<int> colorThemeValues = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        auto colorTheme = std::make_shared<FilesystemModule::EnumSetting>("Theme Color", 2, colorThemeOptions, colorThemeValues);
        settings.push_back(colorTheme);

        auto themeRed = std::make_shared<FilesystemModule::IntSetting>("Theme Color Red", 0, 0, 255, 1);
        settings.push_back(themeRed);

        auto themeGreen = std::make_shared<FilesystemModule::IntSetting>("Theme Color Green", 255, 0, 255, 1);
        settings.push_back(themeGreen);

        auto themeBlue = std::make_shared<FilesystemModule::IntSetting>("Theme Color Blue", 0, 0, 255, 1);
        settings.push_back(themeBlue);        

        System_Utils::GenerateDefaultSettings(settings);
        LoraModule::Utilities::GenerateDefaultSettings(settings);

        std::vector<std::string> wifiOptions = {"Off", "AP Mode", "Station Mode"};
        std::vector<int> wifiValues = {0, 1, 2};
        auto wifiProvisioning = std::make_shared<FilesystemModule::EnumSetting>("WiFi Mode", 0, wifiOptions, wifiValues);
        settings.push_back(wifiProvisioning);

        auto wifiapPassword = std::make_shared<FilesystemModule::StringSetting>("WiFi AP Password", "esp-pass", 21);
        settings.push_back(wifiapPassword);

        FilesystemModule::Utilities::SettingsPreference().begin(FilesystemModule::SettingsInterface::preference_namespace, true);
        for (const auto& setting : settings)
        {
            setting->loadFromPreferences(FilesystemModule::Utilities::SettingsPreference());
        }
        FilesystemModule::Utilities::SettingsPreference().end();

        return settings;
    }

    static void ProcessSettingsFile(JsonDocument &doc)
    {
        ESP_LOGI(TAG_COMPASS, "Processing settings file update");

        if (!doc.isNull())
        {
            std::string debugStr = doc.as<std::string>();
            ESP_LOGI(TAG_COMPASS, "Settings JSON: %s", debugStr.c_str());

            // LED Module
            int colorTheme = doc["Theme Color"].as<int>();

            uint8_t red = doc["Theme Color Red"].as<uint8_t>();
            uint8_t green = doc["Theme Color Green"].as<uint8_t>();
            uint8_t blue = doc["Theme Color Blue"].as<uint8_t>();

            std::unordered_map<int, CRGB> presetThemeColors = 
            {
                /* Custom */ {0, CRGB(red, green, blue)},
                /* Red */ {1, CRGB(CRGB::HTMLColorCode::Red)},
                /* Green */ {2, CRGB(CRGB::HTMLColorCode::Green)},
                /* Blue */ {3, CRGB(CRGB::HTMLColorCode::Blue)},
                /* Purple */ {4, CRGB(CRGB::HTMLColorCode::Purple)},
                /* Yellow */ {5, CRGB(CRGB::HTMLColorCode::Yellow)},
                /* Cyan */ {6, CRGB(CRGB::HTMLColorCode::Cyan)},
                /* White */ {7, CRGB(255, 255, 255)},
                /* Orange */ {8, CRGB(CRGB::HTMLColorCode::Orange)},
            };

            CRGB color;

            if (presetThemeColors.find(colorTheme) != presetThemeColors.end())
            {
                color = presetThemeColors[colorTheme];
            }
            else
            {
                color = CRGB(red, green, blue);
            }

            LED_Utils::setThemeColor(color);
            auto interfaceColor = LedPatternInterface::ThemeColor();
            ESP_LOGI(TAG_COMPASS, "LED Interface::ThemeColor: %d, %d, %d", interfaceColor.r, interfaceColor.g, interfaceColor.b);

            ESP_LOGD(TAG_COMPASS, "ProcessSettingsFile: Done");
        }
    }

    static void InitializeRpc(size_t rpcTaskPriority, size_t rpcTaskCore)
    {
        // Allow CORS requests for RPC server.
        WebServerInstance.addMiddleware(&cors);

        RpcManagerInstance.Init(rpcTaskPriority, rpcTaskCore);
        RpcManagerInstance.RegisterWebServerRpc(WebServerInstance); 

        WiFi.onEvent(EnableServerOnWiFiConnected);

        RegisterRpcFunctions();
    }

    static void WireFunctions()
    {
        ConnectivityModule::Utilities::InitializeEspNow() += [](esp_now_recv_cb_t receiveFunction, esp_now_send_cb_t sendFunction) 
        { 
            if (ConnectivityModule::Utilities::RpcChannelID() == -1)
            {
                ConnectivityModule::Utilities::RpcChannelID() = RpcModule::Utilities::AddRpcChannel(
                    512, 
                    std::bind(
                        &ConnectivityModule::EspNowManager::
                        ReceiveRpcQueue, 
                        &EspNowManagerInstance, 
                        std::placeholders::_1, 
                        std::placeholders::_2), 
                        RpcModule::Utilities::RpcResponseNullDestination);
            }

            RpcModule::Utilities::EnableRpcChannel(ConnectivityModule::Utilities::RpcChannelID());
            
            EspNowManagerInstance.Initialize(receiveFunction, sendFunction); 
        };
        
        ConnectivityModule::Utilities::DeinitializeEspNow() += [](bool disableRadio) 
        { 
            RpcModule::Utilities::DisableRpcChannel(ConnectivityModule::Utilities::RpcChannelID());
            EspNowManagerInstance.Deinitialize(disableRadio); 
        };
    }

    static void RegisterRpcFunctions()
    {
        // Saved Locations
        RpcModule::Utilities::RegisterRpc("AddSavedLocation", NavigationUtils::RpcAddSavedLocation);
        RpcModule::Utilities::RegisterRpc("AddSavedLocations", NavigationUtils::RpcAddSavedLocations);
        RpcModule::Utilities::RegisterRpc("DeleteSavedLocation", NavigationUtils::RpcRemoveSavedLocation);
        RpcModule::Utilities::RegisterRpc("ClearSavedLocations", NavigationUtils::RpcClearSavedLocations);
        RpcModule::Utilities::RegisterRpc("UpdateSavedLocation", NavigationUtils::RpcUpdateSavedLocation);
        RpcModule::Utilities::RegisterRpc("GetSavedLocation", NavigationUtils::RpcGetSavedLocation);
        RpcModule::Utilities::RegisterRpc("GetSavedLocations", NavigationUtils::RpcGetSavedLocations);

        // Saved Messages
        RpcModule::Utilities::RegisterRpc("AddSavedMessage", WayfinderLoraState::RpcAddSavedMessage);
        RpcModule::Utilities::RegisterRpc("AddSavedMessages", WayfinderLoraState::RpcAddSavedMessages);
        RpcModule::Utilities::RegisterRpc("DeleteSavedMessage", WayfinderLoraState::RpcDeleteSavedMessage);
        RpcModule::Utilities::RegisterRpc("DeleteSavedMessages", WayfinderLoraState::RpcDeleteSavedMessages);
        RpcModule::Utilities::RegisterRpc("GetSavedMessage", WayfinderLoraState::RpcGetSavedMessage);
        RpcModule::Utilities::RegisterRpc("GetSavedMessages", WayfinderLoraState::RpcGetSavedMessages);
        RpcModule::Utilities::RegisterRpc("UpdateSavedMessage", WayfinderLoraState::RpcUpdateSavedMessage);

        // Settings
        RpcModule::Utilities::RegisterRpc("GetSettings", FilesystemModule::Utilities::RpcGetSettingsFile);
        RpcModule::Utilities::RegisterRpc("UpdateSetting", FilesystemModule::Utilities::RpcUpdateSetting);
        RpcModule::Utilities::RegisterRpc("UpdateSettings", FilesystemModule::Utilities::RpcUpdateSettings);

        // OTA
        RpcModule::Utilities::RegisterRpc("BeginOTA", System_Utils::StartOtaRpc);
        RpcModule::Utilities::RegisterRpc("UploadOTAChunk", System_Utils::UploadOtaChunkRpc);
        RpcModule::Utilities::RegisterRpc("EndOTA", System_Utils::EndOtaRpc);

        // System
        RpcModule::Utilities::RegisterRpc("RestartSystem", [](JsonDocument &_) { ESP.restart();  vTaskDelay(1000 / portTICK_PERIOD_MS); });
        RpcModule::Utilities::RegisterRpc("GetSystemInfo", System_Utils::GetSystemInfoRpc);
    }

    static void ClearLocations(uint8_t inputID)
    {
        NavigationUtils::ClearSavedLocations();
    }

    static void ClearMessages(uint8_t inputID)
    {
        WayfinderLoraState::ClearSavedMessages();
    }

    // Display Module

    static void InitializeHomeWindow()
    {
        ESP_LOGI(TAG_COMPASS, "InitializeHomeWindow");
        _homeWindowInstance = DisplayModule::HomeWindow::create(MainMenuFactory);

        DisplayModule::Utilities::pushWindow(_homeWindowInstance);
    }

    static void MainMenuFactory(const DisplayModule::InputContext &ctx)
    {
        std::vector<DisplayModule::MenuItem> menuItems;

        // Register Main Menu Items
        menuItems.push_back(DisplayModule::MenuItem("Settings", SettingsWindowFactory));

        // #if HARDWARE_VERSION < 3
        menuItems.push_back(DisplayModule::MenuItem("Configure via WiFi", []()
        {
            auto wifiRpcWindow = std::make_shared<DisplayModule::WiFiRpcWindow>(true);
            DisplayModule::Utilities::pushWindow(wifiRpcWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Configure via BT", []()
        {
            auto btRpcWindow = std::make_shared<DisplayModule::PairBluetoothWindow>();
            DisplayModule::Utilities::pushWindow(btRpcWindow);
        }));
        // #endif

        menuItems.push_back(DisplayModule::MenuItem("Status Messages", []()
        {
            auto editStatusMessagesWindow = std::make_shared<DisplayModule::EditStatusMessagesWindow>();
            DisplayModule::Utilities::pushWindow(editStatusMessagesWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Saved Locations", []()
        {
            auto editSavedLocationsWindow = std::make_shared<DisplayModule::EditSavedLocationsWindow>();
            DisplayModule::Utilities::pushWindow(editSavedLocationsWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Debug Compass", []()
        {
            auto compassWindow = std::make_shared<DisplayModule::CompassWindow>();
            DisplayModule::Utilities::pushWindow(compassWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Debug Geolocation", []()
        {
            auto geolocationDebugWindow = std::make_shared<DisplayModule::GeolocationDebugWindow>();
            DisplayModule::Utilities::pushWindow(geolocationDebugWindow);
        }));
        menuItems.push_back(DisplayModule::MenuItem("Diagnostic Info", []()
        {
            auto diagnosticsWindow = std::make_shared<DisplayModule::DiagnosticsWindow>();
            DisplayModule::Utilities::pushWindow(diagnosticsWindow);
        }));
        #if HARDWARE_VERSION >= 3
        menuItems.push_back(DisplayModule::MenuItem("Breakout", []()
        {
            DisplayModule::Utilities::pushWindow(std::make_shared<DisplayModule::BreakoutWindow>());
        }));
        #endif
        menuItems.push_back(DisplayModule::MenuItem("Reboot", []()
        {
            auto drawCtx = DisplayModule::Utilities::drawContext();
            drawCtx.display->fillScreen(BLACK);
            DisplayModule::TextFormat fmt;
            fmt.hAlign = DisplayModule::TextAlignH::CENTER;
            fmt.vAlign = DisplayModule::TextAlignV::CENTER;
            DisplayModule::TextDrawCommand cmd("Rebooting...", fmt);
            cmd.draw(drawCtx);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            ESP.restart();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }));

        if (System_Utils::getSystemShutdown().Count())
        {
            menuItems.push_back(DisplayModule::MenuItem("Shutdown", []()
            {
                System_Utils::systemShutdownInvoke();
            }));
        }

        auto mainMenuWindowPtr = DisplayModule::makeMenuWindow(menuItems);

        DisplayModule::Utilities::pushWindow(mainMenuWindowPtr);
    }

    static void SettingsWindowFactory()
    {
        auto settingsWindowPtr = DisplayModule::makeSettingsWindow();   
        DisplayModule::Utilities::pushWindow(settingsWindowPtr);
    }

    // Microcontroller Utils

    static std::unordered_set<uint8_t> ScanI2cAddresses(TwoWire &wire = Wire) 
    {
        constexpr uint8_t MIN_ADDRESS = 0x01;
        constexpr uint8_t MAX_ADDRESS = 0x7F;

        std::unordered_set<uint8_t> devicesScanned;

        for (auto addr = MIN_ADDRESS; addr <= MAX_ADDRESS; addr++)
        {
            wire.beginTransmission(addr);
            uint8_t rc = wire.endTransmission();

            if (rc == 0)
            {
                ESP_LOGI("TAG", "Found I2C device at address: %2X", addr);
                devicesScanned.insert(addr);
            }
        }

        return devicesScanned;
    }

private:

    const char * _TAG = "CompassUtils";

    static void EnableServerOnWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        if ((int)event == (int)ARDUINO_EVENT_WIFI_STA_GOT_IP ||
            (int)event == (int)ARDUINO_EVENT_WIFI_AP_START)
        {
            ESP_LOGI(TAG, "WiFi event fired, waiting for stack to stabilize...");
            vTaskDelay(pdMS_TO_TICKS(500));  // Critical: let AP interface fully initialize
            ESP_LOGI(TAG, "WiFi connected, starting RPC server");
            WebServerInstance.begin();
        }
        else if ((int)event == (int)ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
        {
            // server.end();
        }
    }

    
};
