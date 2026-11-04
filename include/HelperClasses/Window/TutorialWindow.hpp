#pragma once

#include <functional>
#include <vector>
#include "Window.hpp"
#include "States/ScrollTextState.hpp"
#include "States/ButtonIntroState.hpp"
#include "States/AppQrState.hpp"
#include "States/EditStringState.hpp"
#include "States/EditEnumState.hpp"
#include "States/CompassCalibrateState.hpp"
#include "HomeWindow.hpp"
#include "DisplayUtilities.hpp"
#include "FilesystemUtils.h"

namespace DisplayModule
{
    // -------------------------------------------------------------------------
    // TutorialWindow
    // -------------------------------------------------------------------------
    // First-boot walkthrough, replayable from the main menu. Steps run in the
    // order they are added in the constructor; to add one, insert another
    // _addPage / _addSettingStep / _addStep call where it belongs.
    //
    // Navigation: BUTTON_4 next, BUTTON_3 back. Back on the first step skips
    // the tour. Finishing or skipping sets a hidden flag in the DeviceInfo
    // namespace (not a user setting) so the tour only auto-runs once.

    class TutorialWindow : public Window
    {
    public:
        static bool isDone()
        {
            auto &prefs = FilesystemModule::Utilities::DeviceInfo();
            prefs.begin(PREFS_NAMESPACE, true);
            bool done = prefs.getBool(DONE_KEY, false);
            prefs.end();
            return done;
        }

        TutorialWindow()
        {
            auto welcome = _addPage("Welcome to Wayfinder!\n\nThis short tour covers the controls and sets up your device.");
            welcome->bindInput(InputID::BUTTON_3, "Skip"); // back on the first step skips

            auto buttons = _addStep(std::make_shared<ButtonIntroState>());
            buttons->setOnComplete([this]() { _next(); });

            // Blank lines push the rest past the 14 visible lines on V3, so
            // the user has to scroll to read it.
            _addPage("The knob scrolls through lists and long text.\n\n"
                     "Scroll down for more"
                     "\n\n\n\n\n\n\n\n\n\n\n"
                     "On the home screen:\n"
                     "Up - your last broadcast\n"
                     "Down - unread messages\n\n"
                     "While typing, press the knob to jump between letters, numbers and symbols.");

            _addPage("Choose the name others see when you broadcast.");
            _addSettingStep(std::make_shared<EditStringState>(), "User Name");

            _addPage("Choose your theme color. Your LEDs use it, and others see it on your broadcasts.");
            _addSettingStep(std::make_shared<EditEnumState>(), "Theme Color");

            // Calibration is pushed from its prompt rather than being a step of
            // its own, so Back from the next page can't land in it — entering
            // calibration resets it, and leaving right away saves garbage.
            _calibrate = std::make_shared<CompassCalibrateState>();
            _calibrate->bindInput(InputID::BUTTON_3, "Done", [this](const InputContext &) {
                popState();
                _next();
            });
            auto calPrompt = _addPage("Calibrate the compass.\n\n"
                                      "Press Start, then slowly turn the device in every direction until the numbers stop changing.");
            calPrompt->unbindInput(InputID::BUTTON_4);
            calPrompt->bindInput(InputID::BUTTON_4, "Start", [this](const InputContext &) { pushState(_calibrate); });
            calPrompt->bindInput(InputID::BUTTON_2, "Skip", [this](const InputContext &) { _next(); });

            _addPracticeBroadcast();
            _addPracticeReceive();

            _addPage("Broadcasting needs a GPS fix. Until you have one you'll see \"No Location Data\".\n\n"
                     "Head outside with a clear view of the sky and give it a minute.");

            auto app = _addStep(std::make_shared<AppQrState>());
            app->bindInput(InputID::BUTTON_3, "Back",   [this](const InputContext &) { _back(); });
            app->bindInput(InputID::BUTTON_4, "Finish", [this](const InputContext &) { _next(); });

            _goTo(0);
        }

    private:
        static constexpr const char *PREFS_NAMESPACE = "DeviceInfo";
        static constexpr const char *DONE_KEY        = "TutorialDone";

        struct Step
        {
            std::shared_ptr<WindowState>       state;
            std::function<StateTransferData()> enterData; // built on entry so it reflects current values
        };

        std::vector<Step>                      _steps;
        size_t                                 _index = 0;
        std::shared_ptr<CompassCalibrateState> _calibrate;
        std::string                            _practiceLocation;

        // Same location -> message flow as HomeWindow's Broadcast, minus the
        // GPS-fix requirement and the actual transmit.
        void _addPracticeBroadcast()
        {
            _addPage("Practice a broadcast.\n\n"
                     "On the home screen, Broadcast shares a location and a short message.\n\n"
                     "Nothing is actually sent here.");

            auto pickLoc = _addStep(std::make_shared<SelectLocationState>(), []() {
                StateTransferData d;
                d.payload = std::make_shared<ArduinoJson::JsonDocument>();
                auto arr  = (*d.payload)["Locations"].to<ArduinoJson::JsonArray>();
                auto here = arr.add<ArduinoJson::JsonObject>();
                here["Name"] = "<Current Location>"; // placeholder coords: practice works without a fix
                here["Lat"]  = 0.0;
                here["Lng"]  = 0.0;
                HomeWindow::AppendSavedLocations(arr);
                return d;
            });
            auto *loc = pickLoc.get(); // raw: the state owns this lambda
            pickLoc->bindInput(InputID::BUTTON_3, "Back",   [this](const InputContext &) { _back(); });
            pickLoc->bindInput(InputID::BUTTON_4, "Select", [this, loc](const InputContext &) {
                auto doc = std::make_shared<ArduinoJson::JsonDocument>();
                loc->buildSelectPayload(doc);
                std::string name = (*doc)["Name"] | "";
                _practiceLocation = (name == "<Current Location>") ? "Ping" : name;
                _next();
            });

            auto pickMsg = _addStep(std::make_shared<SelectMessageState>(), [this]() {
                StateTransferData d;
                d.payload = std::make_shared<ArduinoJson::JsonDocument>();
                auto arr  = (*d.payload)["Messages"].to<ArduinoJson::JsonArray>();
                arr.add(_practiceLocation);
                HomeWindow::AppendMessageOptions(arr);
                return d;
            });
            pickMsg->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &) { _back(); });
            pickMsg->bindInput(InputID::BUTTON_4, "Send", [this](const InputContext &) { _next(); });

            _addPage("Sent! (Not really.)\n\n"
                     "Anyone nearby would now see your message, and their LEDs would point toward you.");
        }

        // A canned message from due north, shown on the real message screen.
        void _addPracticeReceive()
        {
            _addPage("Messages from others look like this next one, sent from due north.\n\n"
                     "Turn around: the LEDs and the line on screen keep pointing at the sender.");

            auto ping = std::make_shared<PingMessage>();
            ping->sender     = 0xCAFE;
            ping->senderName = "Wayfinder";
            ping->status     = "Find me up north!";
            ping->color_R    = 184; // WyrdWyrks amber
            ping->color_G    = 74;
            ping->color_B    = 0;

            auto receive = _addStep(std::make_shared<ViewMessageState>());
            receive->setPreviewMessage(ping, 250);
            receive->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &) { _back(); });
            receive->bindInput(InputID::BUTTON_4, "Next", [this](const InputContext &) { _next(); });
        }

        template <typename T>
        std::shared_ptr<T> _addStep(std::shared_ptr<T> state, std::function<StateTransferData()> enterData = nullptr)
        {
            _steps.push_back({ state, std::move(enterData) });
            return state;
        }

        std::shared_ptr<ScrollTextState> _addPage(std::string text)
        {
            auto page = _addStep(std::make_shared<ScrollTextState>(std::move(text)));
            page->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &) { _back(); });
            page->bindInput(InputID::BUTTON_4, "Next", [this](const InputContext &) { _next(); });
            return page;
        }

        // Edits a registered setting with an EditStateBase editor, starting from
        // its current value, then saves it and reapplies settings to all modules.
        template <typename T>
        void _addSettingStep(std::shared_ptr<T> editor, const char *key)
        {
            _addStep(editor, [key]() {
                StateTransferData d;
                d.payload = std::make_shared<ArduinoJson::JsonDocument>();
                if (auto setting = FilesystemModule::Utilities::FindSetting(key))
                {
                    auto obj = d.payload->to<ArduinoJson::JsonObject>();
                    setting->toJson(obj);
                }
                return d;
            });

            editor->bindInput(InputID::BUTTON_3, "Back", [this](const InputContext &) { _back(); });
            editor->setConfirmAction([this, key](StateTransferData d) {
                auto setting = FilesystemModule::Utilities::FindSetting(key);
                if (setting && d.payload)
                {
                    auto ret = (*d.payload)[FilesystemModule::SettingsInterface::write_key];
                    if (ret.is<std::string>() && ret.as<std::string>().empty())
                    {
                        return; // keep editing rather than save a blank value
                    }

                    auto obj = d.payload->as<ArduinoJson::JsonObjectConst>();
                    setting->fromJson(obj);
                    setting->saveToPreferences(FilesystemModule::Utilities::SettingsPreference());
                    FilesystemModule::Utilities::RequestSettingsRefresh().Invoke();
                }
                _next();
            });
        }

        void _goTo(size_t i)
        {
            if (i >= _steps.size())
            {
                _finish();
                return;
            }
            _index = i;
            auto &step = _steps[i];
            switchState(step.state, step.enterData ? step.enterData() : StateTransferData{});
        }

        void _next() { _goTo(_index + 1); }

        void _back()
        {
            if (_index == 0) _finish();
            else             _goTo(_index - 1);
        }

        void _finish()
        {
            auto &prefs = FilesystemModule::Utilities::DeviceInfo();
            prefs.begin(PREFS_NAMESPACE, false);
            prefs.putBool(DONE_KEY, true);
            prefs.end();

            Utilities::popWindow();
        }
    };

} // namespace DisplayModule
