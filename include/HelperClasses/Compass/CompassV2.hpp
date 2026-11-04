#pragma once

#include "CompassInterface.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM303_Accel.h>
#include "esp_log.h"
#include "ArduinoJson.hpp"

static const char *TAG_COMPASS_LSM = "COMPASS";

namespace
{
    const float AZIMUTH_OFFSET = -90.0f;
};

struct Vector
{
    float x;
    float y;
    float z;
};

// Reads magnetometer to compute azimuth with z correction from accelerometer
class LSM303AGR : public CompassInterface
{
public:
    LSM303AGR()
    {
        if (!Wire.available())
        {
            ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Initializing Wire");
        }
        else
        {
            ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Wire already initialized");
        }

        ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Initializing");

        if (!_CompassMagnetometer.begin(0x1E, &Wire))
        {
            ESP_LOGE(TAG_COMPASS_LSM, "LSM303AGR: Magnetometer not found");
        }

        ESP_LOGI(TAG_COMPASS_LSM, "CompassMagnetometer Initialized");

        if (!_CompassAccelerometer.begin())
        {
            ESP_LOGE(TAG_COMPASS_LSM, "LSM303AGR: Accelerometer not found");
        }

        ESP_LOGI(TAG_COMPASS_LSM, "LSM303AGR: Initialized");
    }

    ~LSM303AGR()
    {
    }

    int GetAzimuth()
    {
        sensors_event_t magEvent;
        sensors_event_t accelEvent;

        Vector mag;
        Vector accel;

        _CompassMagnetometer.getEvent(&magEvent);
        _CompassAccelerometer.getEvent(&accelEvent);

        float Mx = magEvent.magnetic.x;
        float My = magEvent.magnetic.y;
        float Mz = magEvent.magnetic.z;

        if (_IsCalibrated)
        {
            Mx -= (_xMin + _xMax) / 2.0f;
            My -= (_yMin + _yMax) / 2.0f;
            Mz -= (_zMin + _zMax) / 2.0f;
        }

        float Ax = accelEvent.acceleration.x;
        float Ay = accelEvent.acceleration.y;
        float Az = accelEvent.acceleration.z;

        mag.x = Mx;
        mag.y = My;
        mag.z = Mz;

        accel.x = Ax;
        accel.y = Ay;
        accel.z = Az;

        return GetHeading(mag, accel);
    }

    void PrintRawValues()
    {
        sensors_event_t magEvent;
        sensors_event_t accelEvent;

        _CompassMagnetometer.getEvent(&magEvent);
        _CompassAccelerometer.getEvent(&accelEvent);

        float Mx = magEvent.magnetic.x;
        float My = magEvent.magnetic.y;
        float Mz = magEvent.magnetic.z;

        float Ax = accelEvent.acceleration.x;
        float Ay = accelEvent.acceleration.y;
        float Az = accelEvent.acceleration.z;

        ESP_LOGD(TAG_COMPASS_LSM, "Mx: %.2f My: %.2f Mz: %.2f Ax: %.2f Ay: %.2f Az: %.2f", Mx, My, Mz, Ax, Ay, Az);
    }

    void GetRawValues(JsonDocument &doc) override
    {
        sensors_event_t magEvent;
        sensors_event_t accelEvent;

        _CompassMagnetometer.getEvent(&magEvent);
        _CompassAccelerometer.getEvent(&accelEvent);

        float Mx = magEvent.magnetic.x;
        float My = magEvent.magnetic.y;
        float Mz = magEvent.magnetic.z;

        if (_IsCalibrated)
        {
            Mx -= (_xMin + _xMax) / 2.0f;
            My -= (_yMin + _yMax) / 2.0f;
            Mz -= (_zMin + _zMax) / 2.0f;
        }

        doc["magMoniker"] = "lis2mdl";
        doc["accelMoniker"] = "lsm303";

        auto magArr = doc["mag"].to<JsonArray>();
        magArr.add(Mx);
        magArr.add(My);
        magArr.add(Mz);

        auto accelArr = doc["accel"].to<JsonArray>();
        accelArr.add(accelEvent.acceleration.x);
        accelArr.add(accelEvent.acceleration.y);
        accelArr.add(accelEvent.acceleration.z);
    }

    // Calibration methods
    void BeginCalibration()
    {
        _IsCalibrated = false;

        _xMin = 1000000000;
        _xMax = -1000000000;

        _yMin = 1000000000;
        _yMax = -1000000000;

        _zMin = 1000000000;
        _zMax = -1000000000;
    }

    void IterateCalibration()
    {
        sensors_event_t magEvent;
        _CompassMagnetometer.getEvent(&magEvent);

        _xMin = min(_xMin, magEvent.magnetic.x);
        _xMax = max(_xMax, magEvent.magnetic.x);

        _yMin = min(_yMin, magEvent.magnetic.y);
        _yMax = max(_yMax, magEvent.magnetic.y);

        _zMin = min(_zMin, magEvent.magnetic.z);
        _zMax = max(_zMax, magEvent.magnetic.z);
    }

    void EndCalibration()
    {
        _IsCalibrated = true;
    }

    void GetCalibrationData(JsonDocument &doc)
    {
        doc["xMin"] = _xMin;
        doc["xMax"] = _xMax;

        doc["yMin"] = _yMin;
        doc["yMax"] = _yMax;

        doc["zMin"] = _zMin;
        doc["zMax"] = _zMax;
    }

    void SetCalibrationData(JsonDocument &doc)
    {
        if (!doc["xMin"].is<float>() || !doc["xMax"].is<float>() ||
            !doc["yMin"].is<float>() || !doc["yMax"].is<float>() ||
            !doc["zMin"].is<float>() || !doc["zMax"].is<float>())
        {
            ESP_LOGW(TAG_COMPASS_LSM, "SetCalibrationData: missing keys, ignoring");
            return;
        }

        float xMin = doc["xMin"].as<float>();
        float xMax = doc["xMax"].as<float>();
        float yMin = doc["yMin"].as<float>();
        float yMax = doc["yMax"].as<float>();
        float zMin = doc["zMin"].as<float>();
        float zMax = doc["zMax"].as<float>();

        if (xMin == 0 && xMax == 0 && yMin == 0 && yMax == 0 && zMin == 0 && zMax == 0)
        {
            ESP_LOGW(TAG_COMPASS_LSM, "SetCalibrationData: all values are zero, ignoring");
            return;
        }

        _xMin = xMin;
        _xMax = xMax;
        _yMin = yMin;
        _yMax = yMax;
        _zMin = zMin;
        _zMax = zMax;

        ESP_LOGI(TAG_COMPASS_LSM, "Calibration loaded: X[%.2f,%.2f] Y[%.2f,%.2f] Z[%.2f,%.2f]",
            _xMin, _xMax, _yMin, _yMax, _zMin, _zMax);

        _IsCalibrated = true;
    }

protected:

    Adafruit_LIS2MDL _CompassMagnetometer = Adafruit_LIS2MDL(12345);
    Adafruit_LSM303_Accel_Unified _CompassAccelerometer = Adafruit_LSM303_Accel_Unified(54321);

    // Callibration data
    float _xMin = 0;
    float _xMax = 0;

    float _yMin = 0;
    float _yMax = 0;

    float _zMin = 0;
    float _zMax = 0;

    bool _IsCalibrated = false;

    float getTiltCompensatedAzimuth(float Mx, float My, float Mz, float Ax, float Ay, float Az) 
    {
        // Normalize accelerometer values (optional but improves stability)
        float accel_magnitude = sqrt(Ax * Ax + Ay * Ay + Az * Az);
        Ax /= accel_magnitude;
        Ay /= accel_magnitude;
        Az /= accel_magnitude;

        // Calculate pitch and roll
        float pitch = asin(-Ax);  // Pitch around X-axis
        float roll = atan2(Ay, Az);  // Roll around Y-axis

        // Tilt compensation for magnetometer
        float Mx_tc = Mx * cos(pitch) + Mz * sin(pitch);
        float My_tc = Mx * sin(roll) * sin(pitch) + My * cos(roll) - Mz * sin(roll) * cos(pitch);

        // Calculate azimuth
        float azimuth = atan2(My_tc, Mx_tc) * (180.0 / M_PI);  // Convert to degrees
        if (azimuth < 0) 
        {
            azimuth += 360.0;  // Normalize to [0, 360]
        }

        return azimuth;
    }

    float GetHeading(Vector &mag, Vector &accel)
    {
        Vector from = {1, 0, 0};
        Vector east;
        Vector north;

        VectorCross(mag, accel, east);
        VectorNormalize(east);

        VectorCross(accel, east, north);
        VectorNormalize(north);

        float heading = atan2(VectorDot(east, from), VectorDot(north, from)) * (180.0 / M_PI);
        heading += AZIMUTH_OFFSET;
        if (heading < 0)
        {
            heading += 360.0;
        }

        // Something about these calculations isn't correct.
        // Seems to work fine if we just mirror whatever calculation comes out
        return 360.0f - heading;                                                  
    }

    void VectorCross(const Vector &a, const Vector &b, Vector &result)
    {
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
    }

    float VectorDot(const Vector &a, const Vector &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    void VectorNormalize(Vector &v)
    {
        float length = sqrt(VectorDot(v, v));
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }
};