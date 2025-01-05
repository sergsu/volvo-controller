#ifndef _UTILITY_H_
#define _UTILITY_H_
#include <HardwareSerial.h>

char *PrintHexByte(char *str, unsigned int d);
char *hexdump(char *str, unsigned char *d, int l, bool appendNewLine);
char *i2str_zeropad(int i, char *buf);
char *i2str(int i, char *buf);

inline void WORDSWAP(unsigned char *out, unsigned char *in);
short twobyte2word(unsigned char *in);
float currentVoltage();

#define BYTE2TEMP(x) ((unsigned char)((x) - 50))
#define WORD2HOUR(w) twobyte2word(w)

#endif  //_UTILITY_H_
