#pragma once
#include <Arduino.h>
#include "config.h"
#include "logging.h"

struct WledInstance {
    char    ip[32];
    int     port;
    bool    lastState;  // on/off
    bool    online;     // erreichbar
    uint8_t bri;
    uint8_t r, g, b;
};

struct WledConfig {
    WledInstance innen;
    WledInstance aussen;
};

extern WledConfig wledConfig;

// Netzwerk
void wledSetup();
bool wledSetState(WledInstance &inst, bool on);
bool wledSendColor(WledInstance &inst, uint8_t r, uint8_t g, uint8_t b);
bool wledSendBri(WledInstance &inst, uint8_t bri);
void wledLoop();

#define WLED_POLL_INTERVAL_MS 5000