#pragma once

#include "HelperClasses/Compass/CompassV1.hpp"
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
        CompassInstance().SetInvertX(true);

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

    static QMC5883L &CompassInstance()
    {
        static QMC5883L compass;
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
            // Coordinates of San Francisco
            37.7749,   // Latitude
            -122.4194  // Longitude
        );
        return staticLocationSource;
    }
};