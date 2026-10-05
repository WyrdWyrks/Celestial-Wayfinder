#pragma once

#include <string>
#include <vector>
#include <memory>
#include "WindowState.hpp"
#include "TextDrawCommand.hpp"
#include "HelperClasses/WayfinderLoraState.hpp"
#include "HelperClasses/PingMessage.hpp"
#include "LedUtilities.hpp"
#include "ScrollWheel.hpp"

namespace DisplayModule
{
    class SavedStatusMsgListState : public WindowState
    {
    public:
        enum class PendingAction { None, Edit, Create };

        SavedStatusMsgListState()
        {
            bindInput(InputID::BUTTON_1, "Delete", [this](const InputContext &) {
                WayfinderLoraState::DeleteSavedMessage(_selectedIt);
                if (_selectedIt == WayfinderLoraState::SavedMessageListEnd()
                    && WayfinderLoraState::GetSavedMessageListSize() > 0)
                {
                    --_selectedIt;
                }
                _rebuildDrawCommands();
            });
            bindInput(InputID::BUTTON_2, "Create");
            bindInput(InputID::BUTTON_3, "Back");
            bindInput(InputID::BUTTON_4, "Edit");
            bindInput(InputID::ENC_UP, "", [this](const InputContext &) {
                if (WayfinderLoraState::GetSavedMessageListSize() > 0)
                {
                    if (_selectedIt == WayfinderLoraState::SavedMessageListBegin())
                        _selectedIt = WayfinderLoraState::SavedMessageListEnd();
                    --_selectedIt;
                }
                _rebuildDrawCommands();
            });
            bindInput(InputID::ENC_DOWN, "", [this](const InputContext &) {
                if (WayfinderLoraState::GetSavedMessageListSize() > 0)
                {
                    ++_selectedIt;
                    if (_selectedIt == WayfinderLoraState::SavedMessageListEnd())
                        _selectedIt = WayfinderLoraState::SavedMessageListBegin();
                }
                _rebuildDrawCommands();
            });
        }

        void onEnter(const StateTransferData &data) override
        {
            if (data.payload && data.payload->operator[]("return").is<std::string>())
            {
                std::string newStr = (*data.payload)["return"].as<std::string>();

                if (_pendingAction == PendingAction::Edit
                    && _selectedIt != WayfinderLoraState::SavedMessageListEnd())
                {
                    WayfinderLoraState::UpdateSavedMessage(_selectedIt, newStr);
                }
                else if (_pendingAction == PendingAction::Create)
                {
                    WayfinderLoraState::AddSavedMessage(newStr);
                    _selectedIt = WayfinderLoraState::SavedMessageListBegin();
                }
            }
            else
            {
                _selectedIt = WayfinderLoraState::SavedMessageListBegin();
            }

            _pendingAction = PendingAction::None;
            _scrollWheelID = UxModule::ScrollWheel::RegisteredPatternID();
            UxModule::LedUtilities::enablePattern(_scrollWheelID);

            _rebuildDrawCommands();
        }

        void onExit() override
        {
            UxModule::LedUtilities::disablePattern(_scrollWheelID);
            WindowState::onExit();
        }

        std::shared_ptr<ArduinoJson::JsonDocument> buildEditPayload()
        {
            _pendingAction = PendingAction::Edit;
            auto doc = std::make_shared<ArduinoJson::JsonDocument>();
            if (WayfinderLoraState::GetSavedMessageListSize() > 0
                && _selectedIt != WayfinderLoraState::SavedMessageListEnd())
            {
                (*doc)["cfgVal"] = *_selectedIt;
            }
            (*doc)["maxLen"] = static_cast<int>(PingMessage::STATUS_LENGTH);
            return doc;
        }

        std::shared_ptr<ArduinoJson::JsonDocument> buildCreatePayload()
        {
            _pendingAction = PendingAction::Create;
            auto doc = std::make_shared<ArduinoJson::JsonDocument>();
            (*doc)["maxLen"] = static_cast<int>(PingMessage::STATUS_LENGTH);
            return doc;
        }

    private:
        std::vector<std::string>::iterator _selectedIt;
        int                                _scrollWheelID = -1;
        PendingAction                      _pendingAction = PendingAction::None;

        void _rebuildDrawCommands()
        {
            clearDrawCommands();

            size_t count = WayfinderLoraState::GetSavedMessageListSize();
            if (count > 0 && _selectedIt != WayfinderLoraState::SavedMessageListEnd())
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    *_selectedIt,
                    TextFormat{ TextAlignH::CENTER, TextAlignV::CENTER }
                ));

                if (_scrollWheelID >= 0)
                {
                    ArduinoJson::JsonDocument cfg;
                    cfg["numItems"] = count;
                    cfg["currItem"] = std::distance(
                        WayfinderLoraState::SavedMessageListBegin(), _selectedIt);
                    UxModule::LedUtilities::configurePattern(_scrollWheelID, cfg);
                    UxModule::LedUtilities::iteratePattern(_scrollWheelID);
                }
            }
            else
            {
                addDrawCommand(std::make_shared<TextDrawCommand>(
                    "No Saved Messages",
                    TextFormat{ TextAlignH::CENTER, TextAlignV::CENTER }
                ));
            }
        }
    };

} // namespace DisplayModule
