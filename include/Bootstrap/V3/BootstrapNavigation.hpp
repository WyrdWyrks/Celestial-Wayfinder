#pragma once

#include "BootstrapMicrocontroller.hpp"

#include "HelperClasses/Compass/CompassV3.hpp"
#include "TinyGPS++.h"

#include "NavigationManager.h"
#include "GpsSource.hpp"
#include "StaticLocation.hpp"
#include "WiFiGeolocator.hpp"
#include "EzTimeSource.hpp"
#include "NavigationUtils.h"
#include "FilesystemUtils.h"

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

        NavigationModule::Utilities::RegisterLocationSource(&GpsLocatorAndClock(), true);
        NavigationModule::Utilities::RegisterLocationSource(&WiFiGeolocatorSource(), true);
        NavigationModule::Utilities::RegisterLocationSource(&StaticLocationSource(), false);

        // Settings are loaded before navigation boots (see Bootstrap() in
        // main.cpp), so seed the source now and follow any later edits.
        ApplyStaticLocationSettings();
        FilesystemModule::Utilities::SettingsUpdated() += [](JsonDocument &_) { ApplyStaticLocationSettings(); };

        NavigationManagerInstance().StartLocationPolling(); // 15s interval, 60s max-age
    }

    static void ApplyStaticLocationSettings()
    {
        // Falls back to whatever the source already holds if the settings are
        // missing, rather than dropping the device off Null Island.
        auto lat = FilesystemModule::Utilities::FetchFloatSetting("Static Lat", StaticLocationSource().GetLatitude());
        auto lon = FilesystemModule::Utilities::FetchFloatSetting("Static Lon", StaticLocationSource().GetLongitude());

        ESP_LOGI("NavBoostrap", "Static location set to %.4f, %.4f", lat, lon);
        StaticLocationSource().SetLocation(lat, lon);
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

    static NavigationModule::WiFiGeolocator &WiFiGeolocatorSource()
    {
        static NavigationModule::WiFiGeolocator wifiGeolocatorSource;
        return wifiGeolocatorSource;
    }

    static NavigationModule::StaticLocation &StaticLocationSource()
    {
        static NavigationModule::StaticLocation staticLocationSource(
            // Coordinates of Atlanta, matching the "Static Lat"/"Static Lon"
            // setting defaults that overwrite these at boot.
            33.7490,   // Latitude
            -84.3880   // Longitude
            );
        return staticLocationSource;
    }
};