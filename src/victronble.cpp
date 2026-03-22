#include "victronble.h"
#include "appconfig.h"
#include "sensorpoll.h"
#include "logging.h"
#include <VictronBLE.h>

static VictronBLE victron;
static bool s_initialized = false;

static void onVictronData(const VictronDevice *dev)
{
    if (dev->deviceType != DEVICE_TYPE_BATTERY_MONITOR) return;
    if (!dev->dataValid) return;

    const VictronBatteryData &b = dev->battery;

    sensorData.vedirect_valid   = true;
    sensorData.voltage          = b.voltage;
    sensorData.current          = b.current;
    sensorData.power            = b.voltage * b.current;
    sensorData.soc              = b.soc;
    sensorData.ttg              = b.remainingMinutes;
    sensorData.voltage_starter  = b.auxVoltage;

    logPrintf("BMV712: %.2fV %.2fA %.1f%% TTG=%dmin\n",
        b.voltage, b.current, b.soc, b.remainingMinutes);
}

void victronBleSetup()
{
    // Nur starten wenn MAC und Key konfiguriert
    if (strlen(appConfig.bmv_mac) == 0 || strlen(appConfig.bmv_bindkey) == 0)
    {
        logPrintln("VictronBLE: MAC oder Key nicht konfiguriert");
        return;
    }

    if (!victron.begin(5))
    {
        logPrintln("VictronBLE: BLE init FEHLER");
        return;
    }

    victron.setCallback(onVictronData);
    victron.setMinInterval(2000);

    if (!victron.addDevice("BMV712",
                           appConfig.bmv_mac,
                           appConfig.bmv_bindkey,
                           DEVICE_TYPE_BATTERY_MONITOR))
    {
        logPrintln("VictronBLE: addDevice FEHLER");
        return;
    }

    s_initialized = true;
    logPrintf("VictronBLE: gestartet, MAC=%s\n", appConfig.bmv_mac);
}

void victronBleLoop()
{
    if (!s_initialized) return;
    victron.loop();
}
