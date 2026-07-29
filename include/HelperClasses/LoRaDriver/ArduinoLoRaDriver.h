#include "LoraDriverInterface.h"
#include <LoRa.h>
#include <esp_log.h>
#include <freertos/semphr.h>

static const char *TAG_LORA = "LORA";

class ArduinoLoRaDriver : public LoraDriverInterface
{
public:
    ArduinoLoRaDriver(SPIClass *spi, int cs, int reset, int dio0, uint32_t loraFrequency, uint32_t spiFrequency = 1000000) :
        _spi(spi),
        _spiFrequency(spiFrequency),
        _cs(cs),
        _reset(reset),
        _dio0(dio0),
        _loraFrequency(loraFrequency)
    {
        _instance = this;
    }

    bool Init()
    {
        ESP_LOGI(TAG_LORA, "Initializing LoRa...");

        static StaticSemaphore_t cadSemBuf;
        _cadSemaphore = xSemaphoreCreateBinaryStatic(&cadSemBuf);
        if (_spi == nullptr)
        {
            ESP_LOGW(TAG_LORA, "No valid Spi bus detected");
            return false;
        }

        ESP_LOGI(TAG_LORA, "CS: %d", _cs);
        ESP_LOGI(TAG_LORA, "RESET: %d", _reset);
        ESP_LOGI(TAG_LORA, "DIO0: %d", _dio0);

        LoRa.setPins(_cs, _reset, _dio0);
        LoRa.setSPI(*_spi);

        ESP_LOGI(TAG_LORA, "Beginning LoRa...");

        auto result = LoRa.begin(_loraFrequency) == 1;

        if (result)
        {
            ESP_LOGI(TAG_LORA, "Success");
        }
        else
        {
            ESP_LOGE(TAG_LORA, "Failed");
        }

        LoRa.setCodingRate4(8);
        LoRa.setSpreadingFactor(7);
        LoRa.setSignalBandwidth(500E3);
        LoRa.setPreambleLength(12);
        LoRa.setSPIFrequency(_spiFrequency);
        return result;
    }

    bool ReceiveMessage(uint8_t* buffer, size_t& outLen, size_t timeout) override
    {
        // Interrupt mode: library ISR already buffered the packet into its internal state
        // (_rxPacketLength set, FIFO pointer ready, IRQ flags cleared).
        // LoRa.available() reflects this without needing parsePacket().
        if (LoRa.available() > 0)
        {
            outLen = 0;
            while (LoRa.available())
            {
                buffer[outLen++] = (uint8_t)(LoRa.read() & 0xFF);
            }
            return outLen > 0;
        }

        // Polling fallback (used when timeout > 0 or no ISR registered)
        auto startTime = xTaskGetTickCount();
        do
        {
            if (LoRa.parsePacket() == 0) { continue; }

            outLen = 0;
            while (LoRa.available())
            {
                buffer[outLen++] = (uint8_t)(LoRa.read() & 0xFF);
            }

            return outLen > 0;
        }
        while ((xTaskGetTickCount() - startTime) < timeout);

        return false;
    }

    void RegisterOnReceive(void(*callback)(int)) override
    {
        LoRa.onReceive(callback);
    }

    void StartReceiving() override
    {
        LoRa.receive();
    }

    int PacketRssi() override
    {
        return LoRa.packetRssi();
    }

    bool IsChannelBusy() override
    {
        xSemaphoreTake(_cadSemaphore, 0);  // drain any stale give from a previous scan

        // SX127x requires Standby before CAD — going directly from RX to CAD
        // can leave the radio in an undefined state on some silicon revisions.
        LoRa.idle();

        LoRa.onCadDone(_onCadDone);
        LoRa.channelActivityDetection();

        // Block until the DIO0 CadDone interrupt fires (~2 ms at SF7/125 kHz)
        if (xSemaphoreTake(_cadSemaphore, pdMS_TO_TICKS(50)) != pdTRUE)
        {
            ESP_LOGW(TAG_LORA, "CAD timeout — assuming clear");
            return false;
        }
        return _cadResult;
    }

    bool SendMessage(const uint8_t* buffer, size_t len) override
    {
        if (len > 255)
        {
            ESP_LOGE(TAG_LORA, "Payload %u bytes exceeds LoRa 255-byte limit — dropping", len);
            return false;
        }

        // Force Standby so beginPacket() never sees a non-TX busy state
        // (radio may be in CAD, RX, or an intermediate mode at this point)
        LoRa.idle();

        if (LoRa.beginPacket())
        {
            LoRa.write(buffer, len);
            return LoRa.endPacket() == 1;
        }

        ESP_LOGE(TAG_LORA, "beginPacket() returned 0 after idle()");
        return false;
    }

    void SetTXPower(int txPower)
    {
        LoRa.setTxPower(txPower);
    }

    void SetFrequency(uint32_t frequency) override
    {
        // The FRF registers only latch reliably out of standby — writing them
        // while the radio sits in RX continuous can leave it on the old
        // frequency. The caller re-enters RX immediately after this returns.
        LoRa.idle();
        LoRa.setFrequency(frequency);
        _loraFrequency = frequency;
        ESP_LOGI(TAG_LORA, "Retuned to %u Hz", frequency);
    }

    void SetSpreadingFactor(int sf)
    {
        LoRa.setSpreadingFactor(sf);
    }

    void SetSignalBandwidth(uint32_t sbw)
    {
        LoRa.setSignalBandwidth(sbw);
    }

protected:
    SPIClass *_spi = nullptr;
    uint32_t _spiFrequency;

    int _cs;
    int _reset;
    int _dio0;

    uint32_t _loraFrequency;

private:
    // Defined in EventDeclarations.cpp so IRAM_ATTR body is in a .cpp translation unit
    // (Xtensa l32r cannot reference flash literals from an inline IRAM function in a header)
    static void IRAM_ATTR _onCadDone(bool channelBusy);

    inline static SemaphoreHandle_t _cadSemaphore = nullptr;
    inline static ArduinoLoRaDriver* _instance = nullptr;
    bool _cadResult = false;
};