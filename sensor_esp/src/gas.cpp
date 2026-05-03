#include "gas.h"
#include "logging.h"
#include <ArduinoJson.h>
#include "sensorconfig.h"

String gasToJson() {
    JsonDocument doc;
    
    int rawSum = 0;
    for (int i = 0; i < 10; i++) {
        rawSum += analogRead(4);
        delay(10);
    }
    int raw = rawSum / 10;
    
    // kein sensor enabled
    if (!sensorConfig.gas_enabled) {
        doc["valid"] = false;
        String out;
        serializeJson(doc, out);
        return out;
    }

    //> 2000 kann eigentlich nicht sein, lasse ich mal drin
    if (raw > 2000) {
        doc["valid"] = false;
        doc["raw"] = raw;
    } else {
        int range = sensorConfig.gas_raw_max - sensorConfig.gas_raw_min;
        int percent = constrain((raw - sensorConfig.gas_raw_min) * 100 / range, 0, 100);
        doc["valid"]   = true;
        doc["raw"]     = raw;
        doc["percent"] = percent;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

void gasLoop() {
    static uint32_t lastMs = 0;
    if (millis() - lastMs < 2000) return;
    lastMs = millis();
    logPrintf("Gas: %s\n", gasToJson().c_str());
}
