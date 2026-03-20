#pragma once
#include <Arduino.h>

// Sensordaten die vom Sensor-ESP geholt werden
struct SensorData
{
    // BME280
    bool    bme_valid;
    float   temperature;
    float   humidity;
    float   pressure;
    // VE.Direct
    bool    vedirect_valid;
    float   voltage;
    float   current;
    float   power;
    float   soc;
    int     ttg;
    float   voltage_starter;
    // CO2
    bool    co2_valid;
    int     co2_ppm;
};

extern SensorData sensorData;

void sensorPollSetup();
void sensorPollLoop();
