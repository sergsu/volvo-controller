#include "utility.h"

#include <Arduino.h>
#include <avr/power.h>

#include "config.h"

char *PrintHexByte(char *str, unsigned int d) {
  if (d <= 0xF) *str++ = '0';
  utoa(d, str, 16);
  str += strlen(str);
  return str;
}

char *hexdump(char *str, unsigned char *d, int l, bool appendNewLine) {
  for (l--; l != -1; l--) {
    str = PrintHexByte(str, *d++);
  }

  if (appendNewLine) {
    *str++ = '\n';
  }

  return str;
}

char *i2str(int i, char *buf) {
  /* integer to string convert */
  itoa(i, buf, 10);
  buf += strlen(buf);
  return buf;
}

char *i2str_zeropad(int i, char *buf) {
  if (i < 10) *buf++ = '0';
  return i2str(i, buf);
}

inline void WORDSWAP(unsigned char *out, unsigned char *in) {
  /* No pointer tricks, to avoid alignment problems */
  out[1] = in[0];
  out[0] = in[1];
}

short twobyte2word(unsigned char *in) {
  short result;
  WORDSWAP((unsigned char *)&result, in);
  return result;
}

float currentVoltage() {
  power_adc_enable();
  constexpr float voltageDivider =
      (VoltageDividerR1 + VoltageDividerR2) /
      VoltageDividerR2;  // voltage divisor resistors
  int sensorValue = analogRead(VoltageProbe);
  power_adc_disable();

  return sensorValue * (voltageDivider * VoltageCorrection * 5.0 / 1023.0);
}
