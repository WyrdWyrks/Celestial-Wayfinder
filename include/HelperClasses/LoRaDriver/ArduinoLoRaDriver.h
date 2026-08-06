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
    }

    bool Init()
    {
        ESP_LOGI(TAG_LORA, "Initializing LoRa...");

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
        LoRa.enableCrc();

        // Append a payload CRC and set the CRC-present bit in the explicit
        // header, so corrupted packets are dropped by the modem instead of
        // being handed up as valid. Receivers recover the flag from the header,
        // so this is safe to roll out to a fleet one device at a time.
        //
        // setSpreadingFactor() above masks the low nibble of REG_MODEM_CONFIG_2
        // when it writes, so this survives the later SetSpreadingFactor() call
        // from the bootstrap.
        LoRa.enableCrc();

        return result;
    }

    bool ReceiveMessage(uint8_t* buffer, size_t capacity, size_t& outLen, size_t timeout) override
    {
        outLen = 0;

        // parsePacket() does the work the interrupt used to do — read and clear
        // the IRQ flags, latch the payload length, point the FIFO at the packet
        // — but on this task, where nothing can move the cursors underneath us.
        // It also skips packets the modem flagged as CRC errors.
        auto startTime = xTaskGetTickCount();
        do
        {
            int len = LoRa.parsePacket();
            if (len > 0)
            {
                size_t want = (size_t)len;
                if (want > capacity)
                {
                    ESP_LOGW(TAG_LORA, "Packet of %d bytes exceeds %u-byte buffer — truncating",
                             len, (unsigned)capacity);
                    want = capacity;
                }

                int got = LoRa.readPacket(buffer, (int)want);
                outLen = (got > 0) ? (size_t)got : 0;
                return outLen > 0;
            }
        }
        while ((xTaskGetTickCount() - startTime) < timeout);

        return false;
    }

    void RegisterOnReceive(void(*callback)()) override
    {
        // Deferred mode: the interrupt performs no SPI and mutates no driver
        // state, so it cannot corrupt a read in progress here.
        LoRa.onDio0Deferred(callback);
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
        // SX127x requires Standby before CAD — going directly from RX to CAD
        // can leave the radio in an undefined state on some silicon revisions.
        LoRa.idle();

        LoRa.channelActivityDetection();

        // Poll for completion rather than waiting on the CAD interrupt: in
        // deferred mode the interrupt does no SPI, so it cannot tell us whether
        // this was CadDone or RxDone. CAD takes ~2 ms at SF7, and the 50 ms
        // budget matches the timeout this replaced.
        const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(50);
        while (xTaskGetTickCount() < deadline)
        {
            int cad = LoRa.parseCad();
            if (cad >= 0) { return cad == 1; }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        ESP_LOGW(TAG_LORA, "CAD timeout — assuming clear");
        return false;
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
};