#include "logging.h"
#include <stdarg.h>

char    logBuffer[LOG_BUFFER_SIZE][LOG_LINE_LENGTH];
uint8_t logIndex = 0;
uint8_t logCount = 0;

// ----------------------------------------------------------------
// Interne Hilfsfunktion: eine Zeile in den Ringpuffer schreiben
// ----------------------------------------------------------------
static void bufferAdd(const char *line)
{
    strncpy(logBuffer[logIndex], line, LOG_LINE_LENGTH - 1);
    logBuffer[logIndex][LOG_LINE_LENGTH - 1] = '\0';
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE)
        logCount++;
}

// ----------------------------------------------------------------
// Öffentliche API
// ----------------------------------------------------------------
void logPrintln(const char *text)
{
    Serial.println(text);
    bufferAdd(text);
}

void logPrintln(const String &text)
{
    logPrintln(text.c_str());
}

void logPrintf(const char *format, ...)
{
    char buf[LOG_LINE_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    Serial.print(buf);

    // Zeilenweise in den Puffer schreiben
    // (ein logPrintf kann mehrere \n enthalten)
    String s(buf);
    int start = 0, idx;
    while ((idx = s.indexOf('\n', start)) >= 0)
    {
        bufferAdd(s.substring(start, idx).c_str());
        start = idx + 1;
    }
    // Rest ohne \n am Ende
    if (start < (int)s.length())
        bufferAdd(s.substring(start).c_str());
}

// ----------------------------------------------------------------
// Alle gepufferten Zeilen auf Serial ausgeben
// ----------------------------------------------------------------
void logDump()
{
    Serial.println("=== Log-Puffer ===");
    for (uint8_t i = 0; i < logCount; i++)
    {
        uint8_t idx = (logIndex + LOG_BUFFER_SIZE - logCount + i) % LOG_BUFFER_SIZE;
        Serial.println(logBuffer[idx]);
    }
    Serial.println("==================");
}
