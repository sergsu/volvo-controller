/*
 * W-Bus constant definitions
 *
 * Original Author: Manuel Jander  mjander@users.sourceforge.net
 *
 * Modifications by Stuart Pittaway for Arduino
 */

#include <HardwareSerial.h>
#include <Arduino.h>
#include "wbus_const.h"
#include "utility.h"


typedef struct
{
  unsigned char wbus_ver;
  unsigned char wbus_code[7];
  unsigned char data_set_id[6];
  unsigned char sw_id[6];
  unsigned char hw_ver[2]; // week / year
  unsigned char sw_ver[5]; // day of week / week / year // ver/ver
  unsigned char sw_ver_eeprom[5];
} wb_version_info_t;

typedef wb_version_info_t *HANDLE_VERSION_WBINFO;


typedef struct
{
  char dev_name[9];
  unsigned char dev_id[5];
  unsigned char dom_cu[3]; // day / week / year
  unsigned char dom_ht[3];
  unsigned char customer_id[20]; // Vehicle manufacturer part number
  unsigned char serial[5];
} wb_basic_info_t;

typedef wb_basic_info_t *HANDLE_BASIC_WBINFO;


typedef struct
{
  unsigned char length;
  unsigned char idx;
  unsigned char value[32];
} wb_sensor_t;

typedef wb_sensor_t *HANDLE_WBSENSOR;

typedef struct {
  unsigned char code;
  unsigned char flags;
  unsigned char counter;
  unsigned char op_state[2];
  unsigned char temp;
  unsigned char volt[2];
  unsigned char hour[2];
  unsigned char minute;
} err_info_t;

typedef err_info_t *HANDLE_ERRINFO;

class WbusInterface
{
  HardwareSerial &serial;
  void init();
  int sensor_read(HANDLE_WBSENSOR sensor, int idx);
  //int wbus_get_version_wbinfo(HANDLE_VERSION_WBINFO i);
  int get_basic_info(HANDLE_BASIC_WBINFO i);
  int ident(uint8_t identCommand, uint8_t *in);

  int get_fault_count(unsigned char ErrorList[32]);
  int get_fault(unsigned char ErrorNumber, HANDLE_ERRINFO errorInfo);
  int clear_faults();

  int turnOn(unsigned char mode, unsigned char time);
  int turnOff();
  /* Check or keep alive heating process. mode is the argument that was passed as to wbus_turnOn() */
  int check(unsigned char mode);
  int fuelPrime( unsigned char time);
  unsigned char checksum(unsigned char *buf, unsigned char len, unsigned char chk);
  int recvAns(const uint8_t *addr,  const uint8_t *cmd,  uint8_t *data,  int *dlen,  int skip);
public:
  WbusInterface(HardwareSerial &refSer);//only hardware ports allowed (1-3)
  int listen(const uint8_t *addr, uint8_t *cmd,  uint8_t *data,  int *dlen);
  int send(uint8_t addr, uint8_t cmd, uint8_t *data, int len, uint8_t *data2, int len2);
  int io(uint8_t* cmd, uint8_t *out, uint8_t *out2, int len2, uint8_t *in, int *dlen, int skip);

};
