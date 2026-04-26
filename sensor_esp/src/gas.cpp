#include "gas.h"
#include "logging.h"
#include <ArduinoJson.h>
String gasToJson() {
    JsonDocument doc;
    
    int rawSum = 0;
    for (int i = 0; i < 10; i++) {
        rawSum += analogRead(4);
        delay(10);
    }
    int raw = rawSum / 10;
    
    // offen = kein Sensor angeschlossen
    if (raw > 2000) {
        doc["valid"] = false;
        doc["raw"] = raw;
    } else {
        int percent = constrain((raw * 100) / 495, 0, 100);
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
