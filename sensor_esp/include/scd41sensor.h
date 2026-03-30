#pragma once
#include <Arduino.h>
#include "config.h"
#include "logging.h"

bool scd41Setup();
void scd41Loop();
bool scd41IsValid();
int  scd41GetCO2();
float scd41GetTemperature();
float scd41GetHumidity();
String scd41ToJson();
String scd41ToJsonFull();