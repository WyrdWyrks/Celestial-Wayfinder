#pragma once

#include "BootstrapMicrocontroller.hpp"

#include "HelperClasses/Compass/CompassV3.hpp"
#include "TinyGPS++.h"

#include "NavigationManager.h"
#include "GpsSource.hpp"
#include "StaticLocation.hpp"
#include "EzTimeSource.hpp"
#include "NavigationUtils.h"

class BootstrapNavigation
{
public:

    static void Initialize()
    {
        ESP_LOGI("NavBoostrap", "Initializing Navigation Module");

        Serial2.setPins(5, 4);
        Serial2.begin(9600);
        NavigationManagerInstance().InitializeUtils(&CompassInstance());

        auto ezTime = new EzTimeSource();
        System_Utils::TimeSources().push_back(&GpsLocatorAndClock());
        System_Utils::TimeSources().push_back(ezTime);

        NavigationModule::Utilities::LocationSources().push_back(&GpsLocatorAndClock());
        NavigationModule::Utilities::LocationSources().push_back(&StaticLocationSource());

        NavigationManagerInstance().StartLocationPolling(); // 15s interval, 60s max-age
    }

    static NavigationManager &NavigationManagerInstance()
    {
        static NavigationManager navManager;
        return navManager;
    }

    // =================== Hardware =======================

    static CompassV3 &CompassInstance()
    {
        static CompassV3 compass(BootstrapMicrocontroller::ScannedDevices(), BootstrapMicrocontroller::I2cBus());
        return compass;
    }

    static NavigationModule::GpsSource &GpsLocatorAndClock()
    {
        static NavigationModule::GpsSource gpsLocatorAndClock(NavigationModule::Utilities::GetGPS(), Serial2);
        return gpsLocatorAndClock;
    }

    static NavigationModule::StaticLocation &StaticLocationSource()
    {
        static NavigationModule::StaticLocation staticLocationSource(
            // Coordinates of New York City
            40.7128,   // Latitude
            -74.0060   // Longitude
            );
        return staticLocationSource;
    }
};