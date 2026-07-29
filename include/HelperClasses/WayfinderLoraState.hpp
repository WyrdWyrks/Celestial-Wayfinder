#pragma once

#include <ArduinoJson.h>
#include <algorithm>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include "EventHandler.h"
#include "FilesystemUtils.h"
#include "LoraUtilities.hpp"
#include "PingMessage.hpp"

namespace
{
    const char* WAYFINDER_MESSAGE_LIST_FILENAME PROGMEM = "/SavedMessages.msgpk";
}

// Saved message list and unread message tracking for the Wayfinder application.
class WayfinderLoraState
{
public:
    static constexpr const char* TAG = "WayfinderLoraState";

    // -------------------------------------------------------------------------
    // Initialisation — call once from BootstrapLora::Initialize()
    // -------------------------------------------------------------------------

    static void Init()
    {
        LoadFromFlash();
        SavedMessageListUpdated() += SaveToFlash;

        // Subscribe to PingMessage events from the LoRa core
        LoraModule::Utilities::MessageTypeReceived(PingMessage::GUID) +=
            [](std::shared_ptr<LoraModule::LoraMessageInterface> msg, bool isNew)
            {
                auto ping = std::static_pointer_cast<PingMessage>(msg);
                if (!ping) { return; }

                if (xSemaphoreTake(MessageMutex(), portMAX_DELAY) == pdTRUE)
                {
                    RecordSenderLocked(ping->sender, ping->senderName);

                    if (isNew)
                    {
                        // New sender — add as unread
                        UnreadMessages()[msg->sender] = ping;
                    }
                    else
                    {
                        // Known sender — update in place only if still unread (not yet dismissed)
                        auto it = UnreadMessages().find(msg->sender);
                        if (it != UnreadMessages().end()) { it->second = ping; }
                    }
                    xSemaphoreGive(MessageMutex());
                }
            };
    }

    // -------------------------------------------------------------------------
    // Unread message map — thread-safe API
    // -------------------------------------------------------------------------

    // Blocking: returns a copy of all unread userId keys.
    static std::vector<uint32_t> GetUnreadUserIds()
    {
        std::vector<uint32_t> ids;
        if (xSemaphoreTake(MessageMutex(), portMAX_DELAY) == pdTRUE)
        {
            for (auto& kv : UnreadMessages()) { ids.push_back(kv.first); }
            xSemaphoreGive(MessageMutex());
        }
        return ids;
    }

    // Blocking: returns the PingMessage for userId, or nullptr if not unread.
    static std::shared_ptr<PingMessage> GetUnreadMessage(uint32_t userId)
    {
        if (xSemaphoreTake(MessageMutex(), portMAX_DELAY) == pdTRUE)
        {
            auto it = UnreadMessages().find(userId);
            auto result = (it != UnreadMessages().end()) ? it->second : nullptr;
            xSemaphoreGive(MessageMutex());
            return result;
        }
        return nullptr;
    }

    // Non-blocking: returns nullptr immediately if the mutex is unavailable.
    // Use from high-frequency render loops; fall back to cached value on nullptr.
    static std::shared_ptr<PingMessage> TryGetUnreadMessage(uint32_t userId)
    {
        if (xSemaphoreTake(MessageMutex(), 0) != pdTRUE) { return nullptr; }
        auto it = UnreadMessages().find(userId);
        auto result = (it != UnreadMessages().end()) ? it->second : nullptr;
        xSemaphoreGive(MessageMutex());
        return result;
    }

    // Blocking: remove userId from the unread map ("Mark Read").
    static void MarkRead(uint32_t userId)
    {
        if (xSemaphoreTake(MessageMutex(), portMAX_DELAY) == pdTRUE)
        {
            UnreadMessages().erase(userId);
            xSemaphoreGive(MessageMutex());
        }
    }

    static size_t GetNumUnread()
    {
        if (xSemaphoreTake(MessageMutex(), portMAX_DELAY) == pdTRUE)
        {
            size_t n = UnreadMessages().size();
            xSemaphoreGive(MessageMutex());
            return n;
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // Encountered senders
    // -------------------------------------------------------------------------
    // Everyone we have heard from since power-on, in first-seen order.
    // Deliberately separate from the unread map so a sender outlives the message
    // that introduced them — you can still @ someone after dismissing their ping.
    // RAM only: the list is meant to describe "who is around right now", so it
    // starts empty on every boot.

    // Blocking: returns the "Name#TAG" label for every encountered sender —
    // the same label their messages are displayed under.
    static std::vector<std::string> GetEncounteredSenderNames()
    {
        std::vector<std::string> names;
        if (xSemaphoreTake(MessageMutex(), portMAX_DELAY) == pdTRUE)
        {
            auto& senders = EncounteredSenders();
            names.reserve(senders.size());
            for (const auto& sender : senders) { names.push_back(sender.display); }
            xSemaphoreGive(MessageMutex());
        }
        return names;
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // Saved status message list
    // -------------------------------------------------------------------------

    static EventHandler<>& SavedMessageListUpdated()
    {
        static EventHandler<> handler;
        return handler;
    }

    static std::vector<std::string>::iterator SavedMessageListBegin()
    {
        return SavedMessageList().begin();
    }

    static std::vector<std::string>::iterator SavedMessageListEnd()
    {
        return SavedMessageList().end();
    }

    static size_t GetSavedMessageListSize()
    {
        return SavedMessageList().size();
    }

    static void AddSavedMessage(std::string message, bool flash = true)
    {
        SavedMessageList().push_back(message);
        ESP_LOGI(TAG, "Added saved message: %s", message.c_str());
        if (flash) { SavedMessageListUpdated().Invoke(); }
    }

    static void DeleteSavedMessage(std::vector<std::string>::iterator& it)
    {
        it = SavedMessageList().erase(it);
        SavedMessageListUpdated().Invoke();
    }

    static void ClearSavedMessages()
    {
        auto& list = SavedMessageList();
        list.clear();
        list.push_back("Meet here");
        list.push_back("Point of interest");
        SavedMessageListUpdated().Invoke();
    }

    static void UpdateSavedMessage(std::vector<std::string>::iterator& it, std::string message)
    {
        *it = message;
    }

    static void FlashDefaultMessages()
    {
        ClearSavedMessages();
    }

    // -------------------------------------------------------------------------
    // Flash persistence
    // -------------------------------------------------------------------------

    static void LoadFromFlash()
    {
        JsonDocument doc;
        auto rc = FilesystemModule::Utilities::ReadFile(WAYFINDER_MESSAGE_LIST_FILENAME, doc);
        if (rc != FilesystemModule::FilesystemReturnCode::FILESYSTEM_OK)
        {
            ESP_LOGE(TAG, "Failed to load message list. Error: %d", (int)rc);
            return;
        }
        DeserializeSavedMessageList(doc);
    }

    static void SaveToFlash()
    {
        JsonDocument doc;
        SerializeSavedMessageList(doc);
        auto rc = FilesystemModule::Utilities::WriteFile(WAYFINDER_MESSAGE_LIST_FILENAME, doc);
        if (rc != FilesystemModule::FilesystemReturnCode::FILESYSTEM_OK)
        {
            ESP_LOGE(TAG, "Failed to save message list. Error: %d", (int)rc);
        }
    }

    static void SerializeSavedMessageList(JsonDocument& doc)
    {
        JsonArray arr = doc["Messages"].is<JsonArray>()
                        ? doc["Messages"].as<JsonArray>()
                        : doc["Messages"].to<ArduinoJson::JsonArray>();
        for (auto& msg : SavedMessageList()) { arr.add(msg); }
    }

    static void DeserializeSavedMessageList(JsonDocument& doc)
    {
        auto& list = SavedMessageList();
        list.clear();
        for (auto msg : doc["Messages"].as<JsonArray>())
        {
            list.push_back(msg.as<std::string>());
        }
    }

    // -------------------------------------------------------------------------
    // RPC handlers
    // -------------------------------------------------------------------------

    static void RpcGetSavedMessage(JsonDocument& doc)
    {
        if (!doc["Idx"].is<int>()) { return; }
        auto idx = doc["Idx"].as<int>();
        auto& list = SavedMessageList();
        doc.clear();
        doc["Message"] = (idx >= 0 && idx < (int)list.size()) ? list[idx] : "";
    }

    static void RpcGetSavedMessages(JsonDocument& doc)
    {
        doc.clear();
        auto arr = doc["Messages"].to<ArduinoJson::JsonArray>();
        for (auto& msg : SavedMessageList()) { arr.add(msg); }
    }

    static void RpcAddSavedMessage(JsonDocument& doc)
    {
        if (doc["Message"].is<std::string>())
        {
            AddSavedMessage(doc["Message"].as<std::string>(), true);
        }
        doc.clear();
        doc["Success"] = true;
    }

    static void RpcAddSavedMessages(JsonDocument& doc)
    {
        if (doc["Messages"].is<JsonArray>())
        {
            for (auto msg : doc["Messages"].as<JsonArray>())
            {
                AddSavedMessage(msg.as<std::string>(), false);
            }
            SavedMessageListUpdated().Invoke();
        }
        doc.clear();
    }

    static void RpcDeleteSavedMessage(JsonDocument& doc)
    {
        bool success = false;
        if (doc["Idx"].is<int>())
        {
            auto idx = doc["Idx"].as<int>();
            auto& list = SavedMessageList();
            if (idx >= 0 && idx < (int)list.size())
            {
                auto it = list.begin() + idx;
                DeleteSavedMessage(it);
                success = true;
            }
        }
        doc.clear();
        doc["Success"] = success;
    }

    static void RpcDeleteSavedMessages(JsonDocument& doc)
    {
        SavedMessageList().clear();
        SavedMessageListUpdated().Invoke();
        doc.clear();
    }

    static void RpcUpdateSavedMessage(JsonDocument& doc)
    {
        bool success = false;
        if (doc["Idx"].is<int>() && doc["Message"].is<std::string>())
        {
            auto idx = doc["Idx"].as<int>();
            auto& list = SavedMessageList();
            if (idx >= 0 && idx < (int)list.size())
            {
                auto it = list.begin() + idx;
                UpdateSavedMessage(it, doc["Message"].as<std::string>());
                success = true;
            }
        }
        doc.clear();
        doc["Success"] = success;
    }

private:
    struct EncounteredSender
    {
        uint32_t    id;
        std::string display;  // "Name#TAG"
    };

    // Upper bound on the encountered-sender list. Caps RAM growth in a crowded
    // mesh and keeps the @-mention list scrollable; the oldest entry is evicted
    // once we're full, so the people we've heard from most recently stay.
    static constexpr size_t MAX_ENCOUNTERED_SENDERS = 32;

    // Caller must already hold MessageMutex().
    static void RecordSenderLocked(uint32_t senderId, const std::string& name)
    {
        if (name.empty()) { return; }

        auto display = PingMessage::TaggedName(name, senderId);
        auto& senders = EncounteredSenders();

        // Keyed on sender ID rather than name: two people who both left their
        // name at the default still get separate entries (their tags differ),
        // and someone who renames mid-session updates in place instead of
        // showing up twice under the same tag.
        auto it = std::find_if(senders.begin(), senders.end(),
            [senderId](const EncounteredSender& s) { return s.id == senderId; });

        if (it != senders.end())
        {
            if (it->display != display)
            {
                ESP_LOGI(TAG, "Sender renamed: %s -> %s",
                         it->display.c_str(), display.c_str());
                it->display = display;
            }
            return;
        }

        if (senders.size() >= MAX_ENCOUNTERED_SENDERS)
        {
            senders.erase(senders.begin());
        }
        senders.push_back({ senderId, display });
        ESP_LOGI(TAG, "Encountered new sender: %s", display.c_str());
    }

    static std::map<uint32_t, std::shared_ptr<PingMessage>>& UnreadMessages()
    {
        static std::map<uint32_t, std::shared_ptr<PingMessage>> m;
        return m;
    }

    static std::vector<EncounteredSender>& EncounteredSenders()
    {
        static std::vector<EncounteredSender> senders;
        return senders;
    }


    static SemaphoreHandle_t& MessageMutex()
    {
        static StaticSemaphore_t buf;
        static SemaphoreHandle_t h = xSemaphoreCreateMutexStatic(&buf);
        return h;
    }

    static std::vector<std::string>& SavedMessageList()
    {
        static std::vector<std::string> list;
        return list;
    }
};
