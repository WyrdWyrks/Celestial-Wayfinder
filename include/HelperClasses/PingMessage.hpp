#pragma once

#include "LoraMessageInterface.hpp"

class PingMessage : public LoraModule::LoraMessageInterface
{
public:
    static constexpr const char* TAG = "PingMessage";
    static constexpr size_t STATUS_LENGTH = 21;

    static constexpr const char* KEY_SENDER_NAME = "n";
    static constexpr const char* KEY_COLOR_R     = "r";
    static constexpr const char* KEY_COLOR_G     = "g";
    static constexpr const char* KEY_COLOR_B     = "b";
    static constexpr const char* KEY_LAT         = "a";
    static constexpr const char* KEY_LNG         = "o";
    static constexpr const char* KEY_STATUS      = "s";

    // FNV-1a hash of sorted required payload field keys: a b g n o r s → "abgnors"
    inline static const uint32_t GUID = LoraModule::schemaHash("abgnors");
    uint32_t SchemaGuid() const override { return GUID; }

    bool serializePayload(JsonObject& payload) override
    {
        payload[KEY_SENDER_NAME] = senderName;
        payload[KEY_COLOR_R]     = color_R;
        payload[KEY_COLOR_G]     = color_G;
        payload[KEY_COLOR_B]     = color_B;
        payload[KEY_LAT]         = lat;
        payload[KEY_LNG]         = lng;
        payload[KEY_STATUS]      = status;
        return true;
    }

    void deserializePayload(JsonObject& payload) override
    {
        senderName = payload[KEY_SENDER_NAME] | "";
        if (senderName.length() > NAME_LENGTH) { senderName.resize(NAME_LENGTH); }

        color_R = payload[KEY_COLOR_R] | uint8_t(0);
        color_G = payload[KEY_COLOR_G] | uint8_t(0);
        color_B = payload[KEY_COLOR_B] | uint8_t(0);
        lat     = payload[KEY_LAT]     | 0.0;
        lng     = payload[KEY_LNG]     | 0.0;

        status = payload[KEY_STATUS] | "";
        if (status.length() > STATUS_LENGTH) { status.resize(STATUS_LENGTH); }
    }

    std::shared_ptr<LoraMessageInterface> clone() const override
    {
        return std::make_shared<PingMessage>(*this);
    }

    void GetPrintableInformation(std::vector<MessagePrintInformation>& info) override
    {
        info.emplace_back(senderName.c_str());
        info.emplace_back(status.c_str());
    }

    // Creator function registered with LoraModule::Utilities::RegisterMessageType.
    // Deserializes the payload; base fields are filled by the caller after creation.
    static std::shared_ptr<LoraMessageInterface> Create(JsonObject& payload)
    {
        if (payload.isNull()) { return nullptr; }
        auto msg = std::make_shared<PingMessage>();
        msg->deserializePayload(payload);
        return msg;
    }

    std::string senderName;
    std::string status;
    uint8_t color_R = 0;
    uint8_t color_G = 0;
    uint8_t color_B = 0;
    double  lat     = 0.0;
    double  lng     = 0.0;
};
