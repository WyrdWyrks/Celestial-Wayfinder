#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <math.h>
#include <unordered_set>
#include <ArduinoJson.hpp>

#include "Adafruit_MMC56x3.h"
#include "DFRobot_BMM350.h"

#include "7Semi_BMI323.h"
#include "Adafruit_LSM6DS.h"

static const char *TAG_COMPASS_V3 = "COMPASS_V3";


namespace
{
    // IMU Addresses
    static constexpr uint8_t BMI323_ADDR    = 0x69;
    static constexpr uint8_t LSM6DS_ADDR    = 0x6A;

    // Magnetometer Addresses
    static constexpr uint8_t MMC5603_ADDR   = 0x30;
    static constexpr uint8_t BMM350_ADDR    = 0x14;
}



enum class ImuV3Type
{
    NONE,
    BMI323,
    LSM6DS
};

enum class MagnetometerV3Type
{
    NONE,
    MMC5603,
    BMM350
};

class CompassV3 : public CompassInterface
{
public:
    explicit CompassV3(const std::unordered_set<uint8_t> &scannedDevices, TwoWire &i2cBus)
    {
        // Init Magnetometer
        if (scannedDevices.count(MMC5603_ADDR))
        {
            ESP_LOGI(TAG_COMPASS_V3, "Initializing MMC5603...");
            _magType = MagnetometerV3Type::MMC5603;
            _magMMC5603 = Adafruit_MMC5603(12340);
            if (!_magMMC5603.begin(MMC5603_ADDR, &i2cBus))
            {
                ESP_LOGE(TAG_COMPASS_V3, "ERROR: MMC5603 could not be initialized!");
                _magType = MagnetometerV3Type::NONE;
            }
        }
        else if (scannedDevices.count(BMM350_ADDR))
        {
            ESP_LOGI(TAG_COMPASS_V3, "Initializing BMM350...");
            _magType = MagnetometerV3Type::BMM350;
            _magBmm350 = new DFRobot_BMM350_I2C(&i2cBus, BMM350_ADDR);
            if (_magBmm350->begin())
            {
                ESP_LOGE(TAG_COMPASS_V3, "ERROR: BMM350 could not be initialized!");
                _magType = MagnetometerV3Type::NONE;
            }
            else
            {
                _magBmm350->setOperationMode(eBmm350NormalMode);
                _magBmm350->setPresetMode(BMM350_PRESETMODE_HIGHACCURACY,BMM350_DATA_RATE_25HZ);
                _magBmm350->setMeasurementXYZ();
            }
        }
        else
        {
            ESP_LOGE(TAG_COMPASS_V3, "No magnetometers were found in the I2C scan!");
        }

        // Init IMU
        if (scannedDevices.count(BMI323_ADDR))
        {
            ESP_LOGI(TAG_COMPASS_V3, "Initializing BMI323...");
            _imuType = ImuV3Type::BMI323;
            if (!_imuBMI323.beginI2C(BMI323_ADDR, i2cBus))
            {
                ESP_LOGE(TAG_COMPASS_V3, "ERROR: BMI323 failed to initialize!");
                _imuType = ImuV3Type::NONE;
            }
        }
        else if (scannedDevices.count(LSM6DS_ADDR))
        {
            ESP_LOGI(TAG_COMPASS_V3, "Initializing LSM6DS...");
            _imuType = ImuV3Type::LSM6DS;
            if (!_imuLSM6DS.begin_I2C(LSM6DS_ADDR, &i2cBus, 12342))
            {
                ESP_LOGE(TAG_COMPASS_V3, "ERROR: LSM6DS failed to initialize!");
                _imuType = ImuV3Type::NONE;
            }
            else
            {
                _imuLSM6DS.setAccelDataRate(LSM6DS_RATE_104_HZ);
                _imuLSM6DS.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
            }
        }
        else
        {
            ESP_LOGE(TAG_COMPASS_V3, "No IMUs were found in the I2C scan!");
        }

        _SetDefaultCalibration();

        ESP_LOGI(TAG_COMPASS_V3, "CompassV3 Initialized with mag: %s and accel: %s", _GetMagMoniker().c_str(), _GetAccelMoniker().c_str());
    }

    int GetAzimuth() override
    {
        Vector mag;
        Vector accel;

        _GetMagReading(mag);

        if (!_GetAccelReading(accel))
        {
            return _AzimuthNoCompensation(mag);
        }

        return _HeadingCompensated(mag, accel);
    }

    void PrintRawValues() override
    {
        Vector mag;
        Vector accel;

        _GetMagReading(mag);
        _GetAccelReading(accel);

        ESP_LOGD(TAG_COMPASS_V3, "Mag reading: x: %f, y: %f, z: %f", mag.x, mag.y, mag.z);
        ESP_LOGD(TAG_COMPASS_V3, "Accel reading: x: %f, y: %f, z: %f", accel.x, accel.y, accel.z);
    }

    // ================== Calibration =====================

    void BeginCalibration() override
    {
        _IsCalibrated = false;

        _xMin = 1000000000;
        _xMax = -1000000000;

        _yMin = 1000000000;
        _yMax = -1000000000;

        _zMin = 1000000000;
        _zMax = -1000000000;
    }

    void IterateCalibration() override
    {
        Vector magReading;
        _GetMagReading(magReading);

        _xMin = min(_xMin, magReading.x);
        _xMax = max(_xMax, magReading.x);

        _yMin = min(_yMin, magReading.y);
        _yMax = max(_yMax, magReading.y);

        _zMin = min(_zMin, magReading.z);
        _zMax = max(_zMax, magReading.z);
    }

    void EndCalibration() override
    {
        _IsCalibrated = true;
    }

    void GetCalibrationData(JsonDocument &doc) override
    {
        doc["xMin"] = _xMin;
        doc["xMax"] = _xMax;

        doc["yMin"] = _yMin;
        doc["yMax"] = _yMax;

        doc["zMin"] = _zMin;
        doc["zMax"] = _zMax;

        doc["magMoniker"] = _GetMagMoniker();
    }
    // I (1793) COMPASS_V3: Calibration loaded: X[-38.63,60.37] Y[-3.16,105.16] Z[-76.06,43.47]
    void SetCalibrationData(JsonDocument &doc) override
    {
        if (!doc["xMin"].is<float>() || !doc["xMax"].is<float>() ||
            !doc["yMin"].is<float>() || !doc["yMax"].is<float>() ||
            !doc["zMin"].is<float>() || !doc["zMax"].is<float>())
        {
            ESP_LOGW(TAG_COMPASS_V3, "SetCalibrationData: missing keys, ignoring");
            return;
        }
        
        if ((doc["magMoniker"] | "notfound") == _GetMagMoniker())
        {
            ESP_LOGI(TAG_COMPASS_V3, "Calibration data for Mag %s found.", _GetMagMoniker().c_str());
            _xMin = doc["xMin"].as<float>();
            _xMax = doc["xMax"].as<float>();

            _yMin = doc["yMin"].as<float>();
            _yMax = doc["yMax"].as<float>();

            _zMin = doc["zMin"].as<float>();
            _zMax = doc["zMax"].as<float>();

            // Sanity check: if every value is zero the data is meaningless
            // (e.g. loaded from a stale file written by a different sensor).
            if (_xMin == 0 && _xMax == 0 && _yMin == 0 && _yMax == 0 && _zMin == 0 && _zMax == 0)
            {
                ESP_LOGW(TAG_COMPASS_V3, "SetCalibrationData: all values are zero, ignoring");
                return;
            }

            ESP_LOGI(TAG_COMPASS_V3, "Calibration loaded: X[%.2f,%.2f] Y[%.2f,%.2f] Z[%.2f,%.2f]",
                _xMin, _xMax, _yMin, _yMax, _zMin, _zMax);

            _IsCalibrated = true;
        }
        
    }

private:
    ImuV3Type _imuType = ImuV3Type::NONE;
    MagnetometerV3Type _magType = MagnetometerV3Type::NONE;

    Adafruit_MMC5603 _magMMC5603;
    DFRobot_BMM350_I2C *_magBmm350 = nullptr;

    BMI323_7Semi _imuBMI323;
    Adafruit_LSM6DS _imuLSM6DS;

    struct Vector
    {
        float x;
        float y;
        float z;
    };

    // Callibration data
    float _xMin = 0;
    float _xMax = 0;

    float _yMin = 0;
    float _yMax = 0;

    float _zMin = 0;
    float _zMax = 0;

    bool _IsCalibrated = false;

    bool _GetMagReading(Vector &reading)
    {
        if (_magType == MagnetometerV3Type::MMC5603)
        {
            sensors_event_t magEvent;
            _magMMC5603.getEvent(&magEvent);
            reading.x = magEvent.magnetic.x;
            reading.y = magEvent.magnetic.y;
            reading.z = magEvent.magnetic.z;
        }
        else if (_magType == MagnetometerV3Type::BMM350)
        {
            auto magData = _magBmm350->getGeomagneticData();
            reading.x = magData.float_x;
            reading.y = magData.float_y;
            reading.z = magData.float_z;
        }
        else
        {
            ESP_LOGW(TAG_COMPASS_V3, "Attempted to read compass with no working magnetometer! (%d)", (int)_magType);
            return false;
        }

        _AdjustMagReading(reading.x, reading.y, reading.z);
        return true;
    }

    bool _GetAccelReading(Vector &reading)
    {
        if (_imuType == ImuV3Type::BMI323)
        {
            if (!_imuBMI323.readAccel(reading.x, reading.y, reading.z))
            {
                ESP_LOGW(TAG_COMPASS_V3, "Failed to read BMI323. Returning basic reading");
                return false;
            }
        }
        else if (_imuType == ImuV3Type::LSM6DS)
        {
            sensors_event_t accel, _, __;
            if (!_imuLSM6DS.getEvent(&accel, &_, &__))
            {
                return false;
            }
            reading.x = accel.acceleration.x;
            reading.y = accel.acceleration.y;
            reading.z = accel.acceleration.z;
        }
        else
        {
            ESP_LOGV(TAG_COMPASS_V3, "No IMU found. Compass reading will not be tilt compensated.");
            return false;
        }

        return true;
    }

    void _SetDefaultCalibration()
    {
        if (_magType == MagnetometerV3Type::MMC5603)
        {
            _xMin = -44.03f;
            _xMax = 58.56f;
            _yMin = -123.86f;
            _yMax = -20.79f;
            _zMin = -34.01f;
            _zMax = 69.21f;

            _IsCalibrated = true;
        }
        else if (_magType == MagnetometerV3Type::BMM350)
        {
            _xMin = -52.57f;
            _xMax = 43.67f;
            _yMin = -28.63f;
            _yMax = 74.38f;
            _zMin = -98.16f;
            _zMax = -0.71f;

            _IsCalibrated = true;
        }
    }

    void _AdjustMagReading(float &x, float &y, float &z)
    {
        if (_IsCalibrated)
        {
            ESP_LOGV(TAG_COMPASS_V3, "Adjusting mag reading with calibration data: X[%.2f,%.2f] Y[%.2f,%.2f] Z[%.2f,%.2f]",
                _xMin, _xMax, _yMin, _yMax, _zMin, _zMax);
                
            x -= (_xMin + _xMax) / 2.0f;
            y -= (_yMin + _yMax) / 2.0f;
            z -= (_zMin + _zMax) / 2.0f;
        }
    }

    int _AzimuthNoCompensation(Vector &magReading)
    {
        float heading = atan2(magReading.y, magReading.x) * (180.0f / M_PI);
        heading += _AzimuthOffset();
        if (heading < 0)
        {
            heading += 360.0f;
        }
        return (int)(360.0f - heading) % 360;
    }

    float _HeadingCompensated(Vector &mag, Vector &accel)
    {
        Vector from = {1, 0, 0};
        Vector east;
        Vector north;

        _VectorCross(mag, accel, east);
        _VectorNormalize(east);

        _VectorCross(accel, east, north);
        _VectorNormalize(north);

        float heading = atan2(_VectorDot(east, from), _VectorDot(north, from)) * (180.0 / M_PI);
        heading += _AzimuthOffset();
        if (heading < 0)
        {
            heading += 360.0;
        }

        if (_magType == MagnetometerV3Type::BMM350)
        {
            heading = 360.0f - heading;
        }

        return heading;
    }

    void _VectorCross(const Vector &a, const Vector &b, Vector &result)
    {
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
    }

    float _VectorDot(const Vector &a, const Vector &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    void _VectorNormalize(Vector &v)
    {
        float length = sqrt(_VectorDot(v, v));
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }

    float _AzimuthOffset()
    {
        if (_magType == MagnetometerV3Type::BMM350)
        {
            return 0.0f;
        }
        else if (_magType == MagnetometerV3Type::MMC5603)
        {
            return 180.0f;
        }
        else
        {
            return 0.0f;
        }
    }

    std::string _GetMagMoniker()
    {
        if (_magType == MagnetometerV3Type::BMM350)
        {
            return "bmm350";
        }
        else if (_magType == MagnetometerV3Type::MMC5603)
        {
            return "mmc5603";
        }
        else
        {
            return "none";
        }
    }

    std::string _GetAccelMoniker()
    {
        if (_imuType == ImuV3Type::BMI323)
        {
            return "bmi323";
        }
        else if (_imuType == ImuV3Type::LSM6DS)
        {
            return "lsm6ds";
        }
        else
        {
            return "none";
        }
    }
};
