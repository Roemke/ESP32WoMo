#pragma once
#include <Arduino.h>
#include "config.h"

// ----------------------------------------------------------------
// Ringpuffer für Log-Nachrichten
// Wird bei Setup/Fehlerdiagnose über Serial ausgegeben.
// Kein WebSocket – einfach und robust.
// ----------------------------------------------------------------

extern char    logBuffer[LOG_BUFFER_SIZE][LOG_LINE_LENGTH];
extern uint8_t logIndex; // nächste Schreibposition
extern uint8_t logCount; // aktuell befüllte Zeilen

void logPrintln(const char   *text);
void logPrintln(const String &text);
void logPrintf (const char   *format, ...);

// Alle gepufferten Zeilen auf Serial ausgeben (z.B. nach Reconnect)
void logDump();
