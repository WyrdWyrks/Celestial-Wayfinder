#pragma once

#include "Window.hpp"
#include "States/HomeState.hpp"
#include "States/TrackingState.hpp"
#include "States/SelectMessageState.hpp"
#include "States/SelectLocationState.hpp"
#include "States/SelectKeyValueState.hpp"
#include "States/LockState.hpp"
#include "States/DisplaySentMessageState.hpp"
#include "States/RepeatMessageState.hpp"
#include "States/ViewMessageState.hpp"
#include "DisplayUtilities.hpp"
#include "LoraUtils.h"
#include "NavigationUtils.h"
#include "FilesystemUtils.h"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "HelperClasses/PingMessage.hpp"
#include <functional>
#include "../Led/Patterns/Flashlight.hpp"

namespace DisplayModule
{
    class HomeWindow : public Window
    {
    public:
        // ------------------------------------------------------------------
        // Factory method — preferred over direct construction
        // ------------------------------------------------------------------
        static std::shared_ptr<HomeWindow> create(
            std::function<void(const InputContext &)> onMenuRequested = nullptr,
            std::function<void(const InputContext &)> onQuickActionRequested = nullptr)
        {
            auto win = std::make_shared<HomeWindow>();
            if (onMenuRequested)
                win->setOnMainMenuRequested(std::move(onMenuRequested));
            if (onQuickActionRequested)
                win->setQuickActionMenuCallback(std::move(onQuickActionRequested));
            return win;
        }

        // ------------------------------------------------------------------
        // Setter — configure the main-menu callback post-construction
        // ------------------------------------------------------------------
        void setOnMainMenuRequested(std::function<void(const InputContext &)> fn)
        {
            _homeState->bindInput(InputID::BUTTON_4, "Menu", std::move(fn));
        }

        void setQuickActionMenuCallback(std::function<void(const InputContext &)> fn)
        {
            _homeState->bindInput(InputID::BUTTON_1, "Actions", std::move(fn));
        }

        HomeWindow()
        {
            // Allocate all states
            _homeState         = std::make_shared<HomeState>();
            _lockState         = std::make_shared<LockState>();
            _selectMsgState    = std::make_shared<SelectMessageState>();
            _selectLocState    = std::make_shared<SelectLocationState>();
            // _unreadState       = std::make_shared<UnreadMessageState>();
            _selectKVState     = std::make_shared<SelectKeyValueState>();
            _sentMsgState      = std::make_shared<DisplaySentMessageState>();
            _repeatState       = std::make_shared<RepeatMessageState>();
            _actionMenuState   = std::make_shared<MenuState>();
            _saveMessageState  = std::make_shared<EditStringState>();
            _saveLocationState = std::make_shared<EditStringState>();
            _viewMessageState  = std::make_shared<ViewMessageState>();

            
            _wireHomeState();
            _wireQuickActionState();
            _wireSelectLocationState();
            _wireSelectMessageState();
            _wireSaveMessageState();
            _wireSaveLocationState();
            _wireViewMessageState();
            _wireSentMessageState();
            _wireLockState();
            _wireRepeatMessageState();
            _wireSelectKeyValueState();

            setInitialState(_homeState);
        }

        ~HomeWindow() = default;

        // Override to handle the "show unread messages" external trigger
        // void showUnreadMessages()
        // {
        //     if (Utilities::activeWindow().get() == this)
        //         switchState(_unreadState);
        // }

        // Override to handle the "show sent message" external trigger
        void showSentMessage()
        {
            if (Utilities::activeWindow().get() == this)
                pushState(_sentMsgState);
        }

        void onMessageReceived()
        {
            if (currentState() == _homeState)
            {
                _homeState->onMessageReceived();
            }

            // TODO: Logic to dynamically update message list
        }

    private:
        std::shared_ptr<HomeState>             _homeState;
        std::shared_ptr<LockState>             _lockState;
        std::shared_ptr<SelectMessageState>    _selectMsgState;
        std::shared_ptr<SelectLocationState>   _selectLocState;
        // std::shared_ptr<UnreadMessageState>    _unreadState;
        std::shared_ptr<SelectKeyValueState>   _selectKVState;
        std::shared_ptr<DisplaySentMessageState> _sentMsgState;
        std::shared_ptr<RepeatMessageState>    _repeatState;
        std::shared_ptr<MenuState>             _actionMenuState;
        std::shared_ptr<EditStringState>       _saveMessageState;
        std::shared_ptr<EditStringState>       _saveLocationState;
        std::shared_ptr<ViewMessageState>      _viewMessageState;

        // Pending reply recipient (set when BUTTON_2 from UnreadMessageState)
        uint64_t _pendingReplyRecipient = 0;
        bool     _directSend           = false;

        // ===================== Wire State Helpers =====================

        void _wireHomeState()
        {
            _homeState->bindInput(
                InputID::BUTTON_1, 
                "Actions", 
                [this](const InputContext &ctx)
                {
                    pushState(_actionMenuState);
                });

            _homeState->bindInput(
                InputID::BUTTON_2, 
                "Broadcast", 
                [this](const InputContext &ctx)
                {
                    _openLocationSelector(ctx);
                });

            _homeState->bindInput(
                InputID::BUTTON_3, 
                "Lock", 
                [this](const InputContext &ctx)
                {
                    pushState(_lockState);
                    System_Utils::enablePowerSavingsInvoke();
                });


            _homeState->bindInput(
                InputID::ENC_DOWN,
                [this](const InputContext &ctx)
                {
                    if (WayfinderLoraState::GetNumUnread() > 0)
                    {
                        pushState(_viewMessageState);
                    }
                });

            _homeState->bindInput(
                InputID::ENC_UP, 
                [this](const InputContext &ctx)
                {
                    if (LoraUtils::MyLastBroadcastExists())
                    {
                        pushState(_sentMsgState);
                    }
                });
        }

        void _wireQuickActionState()
        {
            // Build action menu items and input bindings
            std::vector<DisplayModule::MenuItem> menuItems;
            _buildActionMenu(menuItems);
            _actionMenuState->clearItems();
            for (auto &item : menuItems)
            {
                _actionMenuState->addItem(std::move(item));
            }

            _actionMenuState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx) {
                popState();
            });

            _actionMenuState->bindInput(InputID::BUTTON_4, "Select", [this](const InputContext &ctx) {
                _actionMenuState->selectCurrent();
            });
        }

        void _wireSelectLocationState()
        {
            _selectLocState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx) {
                popState();
            });
            _selectLocState->bindInput(InputID::BUTTON_4, "Select", [this](const InputContext &ctx) {

                // TODO: Dynamically allocate to a cap based on number of messages?
                std::shared_ptr<JsonDocument> payload = std::make_shared<JsonDocument>();
                _selectLocState->buildSelectPayload(payload);
                _openMessageSelector(ctx, payload);
            });
        }

        void _wireSelectMessageState()
        {
            _selectMsgState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx) {
                popState();
            });

            _selectMsgState->bindInput(InputID::BUTTON_4, "Select", [this](const InputContext &ctx) {

                auto payload = JsonDocument();
                _selectMsgState->buildSelectPayload(payload);

                double myLat, myLon = 0.0;
                if (!NavigationUtils::GetCurrentLocation(myLat, myLon))
                {
                    DisplayModule::Utilities::clearAndDisplay(TextDrawCommand::createCenteredMessage("No Location Data"));
                    popState();
                    return;
                }

                time_t currentTime;
                uint32_t gpsTime, gpsDate;
                System_Utils::GetCurrentUTC(currentTime);
                NavigationUtils::TimeTToGpsPacked(currentTime, gpsTime, gpsDate);

                auto ping = std::make_shared<PingMessage>();
                ping->msgID       = esp_random();
                ping->sender      = System_Utils::DeviceID;
                ping->bouncesLeft = 3;
                ping->time        = gpsTime;
                ping->date        = gpsDate;
                ping->senderName  = LoraUtils::UserName();
                ping->color_R     = LED_Utils::ThemeColor().r;
                ping->color_G     = LED_Utils::ThemeColor().g;
                ping->color_B     = LED_Utils::ThemeColor().b;
                ping->lat         = myLat;
                ping->lng         = myLon;
                ping->status      = payload["Message"].as<std::string>();

                ESP_LOGI(TAG, "Sending ping message as %u", ping->sender);

                // Send Payload
                auto success = LoraUtils::SendMessage(ping);

                auto &drawCtx = Utilities::drawContext();
                drawCtx.display->fillScreen(BLACK);

                popState(); // back to HomeState

                if (success)
                {
                    ESP_LOGI(TAG, "Message sent successfully");
                    
                    auto successMsg = TextDrawCommand("Broadcast sent", TextFormat{TextAlignH::CENTER, TextAlignV::CENTER});              
                    successMsg.draw(drawCtx);
                    
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to send message");
                    auto errorMsg = TextDrawCommand("Failed to send", TextFormat{TextAlignH::CENTER, TextAlignV::CENTER});
                    errorMsg.draw(drawCtx);
                }

                Utilities::onRenderComplete();

                vTaskDelay(pdMS_TO_TICKS(2000));
            });
        }

        void _wireSaveMessageState()
        {
            _saveMessageState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx) {
                popState();
            });

            _saveMessageState->bindInput(InputID::BUTTON_2, "Save", [this](const InputContext &ctx) {
                auto payload = _saveMessageState->buildResultPayload();

                std::string returnMsg;

                if (payload->operator[]("return").is<std::string>())
                {
                    auto savedMsg = payload->operator[]("return").as<std::string>();
                    WayfinderLoraState::AddSavedMessage(savedMsg, true);
                    returnMsg = "Message saved";
                    ESP_LOGI(TAG, "Saved message: %s", savedMsg.c_str());
                }
                else
                {
                    ESP_LOGW(TAG, "SaveMessageState entered with invalid payload");
                    returnMsg = "Failed to save";
                }

                auto &drawCtx = Utilities::drawContext();
                drawCtx.display->fillScreen(BLACK);
                auto resultMsg = TextDrawCommand(returnMsg.c_str(), TextFormat{TextAlignH::CENTER, TextAlignV::CENTER});
                resultMsg.draw(drawCtx);
                Utilities::onRenderComplete();
                vTaskDelay(pdMS_TO_TICKS(2000));

                popState(); // back to HomeState
            });
        }

        void _wireSaveLocationState()
        {
            _saveLocationState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx) {
                popState();
            });

            _saveLocationState->bindInput(InputID::BUTTON_2, "Save", [this](const InputContext &ctx) {
                auto payload = _saveLocationState->buildResultPayload();

                if (payload->operator[]("return").is<std::string>())
                {
                    std::string newLocName = payload->operator[]("return").as<std::string>();

                    double myLat, myLon;
                    if (!NavigationUtils::GetCurrentLocation(myLat, myLon))
                    {
                        DisplayModule::Utilities::clearAndDisplay(TextDrawCommand::createCenteredMessage("No Location Data"));
                        popState();
                        return;
                    }

                    SavedLocation newLoc;
                    newLoc.Name = newLocName;
                    newLoc.Latitude = myLat;
                    newLoc.Longitude = myLon;

                    NavigationUtils::AddSavedLocation(newLoc, true);
                    ESP_LOGI(TAG, "Saved location: %s (%f, %f)", newLocName.c_str(), myLat, myLon);

                    auto &drawCtx = Utilities::drawContext();
                    drawCtx.display->fillScreen(BLACK);
                    auto successMsg = TextDrawCommand("Location saved", TextFormat{TextAlignH::CENTER, TextAlignV::CENTER});
                    successMsg.draw(drawCtx);
                    Utilities::onRenderComplete();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    popState(); // back to HomeState
                }
            });
        }

        void _wireViewMessageState()
        {
            _viewMessageState->assignExitCallback([this]() {
                popState();
            });

            _viewMessageState->bindInput(InputID::BUTTON_3, "Mark Read", [this](const InputContext &ctx) {
                if (!_viewMessageState->markCurrentRead())
                {
                    popState();
                }
            });

            _viewMessageState->bindInput(InputID::BUTTON_4, "Save", [this](const InputContext &ctx) {
                pushState(_selectKVState);
            });
        }

        void _wireSentMessageState()
        {
            _sentMsgState->bindInput(InputID::BUTTON_4, "Resend", [this](const InputContext &ctx) {
                auto payload = _sentMsgState->buildRetransmitPayload();
                if (payload)
                {
                    StateTransferData d;
                    d.inputID = ctx.inputID;
                    d.payload = payload;
                    pushState(_repeatState, d);
                }
            });

            _sentMsgState->bindInput(InputID::ENC_DOWN, "v", [this](const InputContext &ctx) {
                popState();
            });
        }

        void _wireLockState()
        {
            _lockState->setOnSequenceComplete([this]() {
                System_Utils::disablePowerSavingsInvoke();
                popState(); // back to HomeState
            });

            _lockState->bindInput(InputID::BUTTON_3, "", [this](const InputContext &ctx) {
                _lockState->processButton(ctx);
            });

            _lockState->bindInput(InputID::BUTTON_4, "", [this](const InputContext &ctx) {
                _lockState->processButton(ctx);
            });

            _lockState->bindInput(InputID::BUTTON_1, "", [this](const InputContext &ctx) {
                _lockState->processButton(ctx);
            });

            _lockState->bindInput(InputID::BUTTON_2, "", [this](const InputContext &ctx) {
                _lockState->processButton(ctx);
            });
        }

        void _wireRepeatMessageState()
        {
            _repeatState->bindInput(InputID::BUTTON_3, "Stop", [this](const InputContext &ctx) {
                popState();
            });
        }

        void _wireSelectKeyValueState()
        {
            _selectKVState->setPrompt("Save:");
            _selectKVState->addItem("Message", 0);
            _selectKVState->addItem("Location", 1);

            _selectKVState->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &ctx) {
                popState();
            });

            _selectKVState->bindInput(InputID::BUTTON_4, "Select", [this](const InputContext &ctx) {
                auto selectedKey = _selectKVState->selectedKey();
                
                if (selectedKey == "Message")
                {
                    ESP_LOGI(TAG, "Selected 'Message' for saving");
                    auto pingMsg = _viewMessageState->currentMessage();

                    ESP_LOGI(TAG, "Saving message: %s", pingMsg->status.c_str());
                    WayfinderLoraState::AddSavedMessage(pingMsg->status);
                     
                    auto &drawCtx = Utilities::drawContext();
                    drawCtx.display->fillScreen(BLACK);
                    auto successMsg = TextDrawCommand("Message saved", TextFormat{TextAlignH::CENTER, TextAlignV::CENTER});
                    successMsg.draw(drawCtx);
                    popState();
                    Utilities::onRenderComplete();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
                else if (selectedKey == "Location")
                {
                    auto msg = _viewMessageState->currentMessage();
                    auto pingMsg = std::static_pointer_cast<PingMessage>(msg);

                    SavedLocation loc;
                    loc.Name = pingMsg->status;
                    loc.Latitude = pingMsg->lat;
                    loc.Longitude = pingMsg->lng;

                    NavigationUtils::AddSavedLocation(loc, true);
                    
                    auto &drawCtx = Utilities::drawContext();
                    drawCtx.display->fillScreen(BLACK);
                    auto successMsg = TextDrawCommand("Location saved", TextFormat{TextAlignH::CENTER, TextAlignV::CENTER});
                    successMsg.draw(drawCtx);
                    Utilities::onRenderComplete();
                    popState();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }

                popState();
                popState();
            });
        }

        void _openLocationSelector(const InputContext &ctx)
        {
            auto doc = std::make_shared<ArduinoJson::JsonDocument>();
            auto arr = (*doc)["Locations"].to<ArduinoJson::JsonArray>();

            double myLat, myLon;
            if (!NavigationUtils::GetCurrentLocation(myLat, myLon))
            {
                // I don't think we necessarily need to block this action since we have saved locations
                // but I can't find a good way to indicate "unknown location" in the UI, so for now we'll just block broadcasting if we don't have a GPS fix
                DisplayModule::Utilities::clearAndDisplay(TextDrawCommand::createCenteredMessage("No Location Data"));
                return;
            }

            JsonObject currentLoc = arr.add<ArduinoJson::JsonObject>();
            currentLoc["Name"] = "Ping";
            currentLoc["Lat"] = myLat;
            currentLoc["Lng"] = myLon;

            for (auto it = NavigationUtils::GetSavedLocationsBegin();
                it != NavigationUtils::GetSavedLocationsEnd(); ++it)
            {
                JsonObject locObj = arr.add<ArduinoJson::JsonObject>();
                locObj["Name"] = it->Name;
                locObj["Lat"] = it->Latitude;
                locObj["Lng"] = it->Longitude;
            }

            StateTransferData d;
            d.inputID = ctx.inputID;
            d.payload = doc;
            pushState(_selectLocState, d);
        }

        void _openMessageSelector(const InputContext &ctx, std::shared_ptr<ArduinoJson::JsonDocument> preselectPayload)
        {
            auto arr = (*preselectPayload)["Messages"].to<ArduinoJson::JsonArray>();
            if (preselectPayload->operator[]("Name").is<std::string>())
            {
                arr.add(preselectPayload->operator[]("Name").as<std::string>());
            }

            for (auto it = WayfinderLoraState::SavedMessageListBegin();
                 it != WayfinderLoraState::SavedMessageListEnd(); ++it)
            {
                arr.add(*it);
            }

            StateTransferData d;
            d.inputID = ctx.inputID;
            d.payload = preselectPayload;
            switchState(_selectMsgState, d);
        }

        void _buildActionMenu(std::vector<DisplayModule::MenuItem> &menuItems)
        {
            // Register Quick Action Menu Items
            #if HARDWARE_VERSION == 1 || HARDWARE_VERSION == 2
            menuItems.push_back(DisplayModule::MenuItem("Flashlight", []()
            {
                auto flashlightId = Flashlight::RegisteredPatternID();
                JsonDocument doc;
                doc["toggle"] = true;
                LED_Utils::configurePattern(flashlightId, doc);
                LED_Utils::iteratePattern(flashlightId);
            }));
            #endif

            menuItems.push_back(DisplayModule::MenuItem("Create Status Message", [this]()
            {
                auto doc = std::make_shared<ArduinoJson::JsonDocument>();
                (*doc)["maxLen"] = 21;
                (*doc)["cfgVal"] = "";
                StateTransferData d;
                d.payload = doc;
                pushState(_saveMessageState, std::move(d));
            }));

            menuItems.push_back(DisplayModule::MenuItem("Save Current Location", [this]()
            {
                auto doc = std::make_shared<ArduinoJson::JsonDocument>();
                (*doc)["maxLen"] = 21;
                (*doc)["cfgVal"] = "";
                StateTransferData d;
                d.payload = doc;
                pushState(_saveLocationState, std::move(d));
            }));

            menuItems.push_back(DisplayModule::MenuItem("Toggle Silent Mode", []()
            {
                System_Utils::silentMode = !System_Utils::silentMode;
                auto raw = FilesystemModule::Utilities::FindSetting("Silent Mode");
                if (raw)
                {
                    auto setting = std::static_pointer_cast<FilesystemModule::BoolSetting>(raw);
                    setting->value = System_Utils::silentMode;
                    setting->saveToPreferences(FilesystemModule::Utilities::SettingsPreference());
                }
            }));

            if (System_Utils::getSystemShutdown().Count())
            {
                menuItems.push_back(DisplayModule::MenuItem("Shutdown", []()
                {
                    System_Utils::systemShutdownInvoke();
                }));
            }            
        }
    };

} // namespace DisplayModule
