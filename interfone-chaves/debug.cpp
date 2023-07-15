#include "debug.h"
#include "settings.h"

void debugMsg(const char* format, ...) {
#ifdef DEBUG
  char str[200];

  va_list args;
  va_start(args, format);

  vsnprintf(str, sizeof(str), format, args);
  Serial.print(str);

  va_end(args);
#endif
}
