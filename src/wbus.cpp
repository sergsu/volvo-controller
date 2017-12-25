/* wbus.cpp
 *
 * Original Author: Manuel Jander  mjander@users.sourceforge.net
 *
 * Modifications by Stuart Pittaway for Arduino
 */

#include "wbus.h"

unsigned char cmdbuf[5];
int len;

//uint16_t CommunicationsErrorCount=0;

void wbus_init() {
  WBUSPORT.end();
  delay(200);
  //Break set

#if defined(__AVR_ATmega328P__)
  //This code is for Arduino ATMEGA328 with single hardware WBUSPORT port (pins 0 and 1)
  //PORTD=digital pins 0 to 7
  DDRD = DDRD | B00000010;  //Pin1 = output
  PORTD = PORTD | B00000010; // digital 1 HIGH
  delay(25);
  PORTD = PORTD & B11111101; // digital 1 LOW
  delay(25);
#endif

  // initialize WBUSPORT communication at 2400 bits per second, 8 bit data, even parity and 1 stop bit
  WBUSPORT.begin(2400, SERIAL_8E1);

  //250ms for timeouts
  WBUSPORT.setTimeout(250);

  // Empty all queues. BRK toggling may cause a false received byte (or more than one who knows).
  while (WBUSPORT.available()) WBUSPORT.read();
}


/*
 * \param buf pointer to ACK message part
 * \param len length of data
 * \param chk initial value of the checksum. Useful for concatenating.
 */
unsigned char checksum(unsigned char *buf, unsigned char len, unsigned char chk)
{
  for (; len != 0; len--) {
    chk ^= *buf++;
  }
  return chk;
}


/**
 * Send request to heater and one or two consecutive buffers.
 * \param cmd wbus command to be sent
 * \param data pointer for first buffer.
 * \param len length of first data buffer.
 * \param data pointer to an additional buffer.
 * \param len length the second data buffer.
 * \return 0 if success, 1 on error.
 */
int wbus_msg_send( uint8_t addr,
                   uint8_t cmd,
                   uint8_t *data,
                   int len,
                   uint8_t *data2,
                   int len2)
{

  uint8_t bytes, chksum;
  uint8_t buf[3];

  /* Assemble packet header */
  buf[0] = addr;
  buf[1] = len + len2 + 2;
  buf[2] = cmd;

  chksum = checksum(buf, 3, 0);
  chksum = checksum(data, len, chksum);
  chksum = checksum(data2, len2, chksum);

  /* Send message */
  //rs232_flush(wbus->rs232);
  WBUSPORT.write(buf, 3);
  WBUSPORT.write(data, len);
  WBUSPORT.write(data2, len2);
  WBUSPORT.write(&chksum, 1);

  /* Read and check echoed header */
  bytes = WBUSPORT.readBytes((char*)buf, 3);
  if (bytes != 3) {
    return -1;
  }

  if (buf[0] != addr || buf[1] != (len + len2 + 2)  || buf[2] != cmd) {
    //PRINTF("wbus_msg_send() K-Line error %x %x %x\n", buf[0], buf[1], buf[2]);
    //CommunicationsErrorCount++;
    return -1;
  }

  /* Read and check echoed data */
  int i;

  for (i = 0; i < len; i++) {
    bytes = WBUSPORT.readBytes((char*)buf, 1);
    if (bytes != 1 || buf[0] != data[i]) {
      //PRINTF("wbus_msg_send() K-Line error. %d < 1 (data2)\n", bytes);
      //CommunicationsErrorCount++;
      return -1;
    }
  }

  for (i = 0; i < len2; i++) {
    bytes = WBUSPORT.readBytes((char*)buf, 1);
    if (bytes != 1 || buf[0] != data2[i]) {
      //PRINTF("wbus_msg_send() K-Line error. %d < 1 (data2)\n", bytes);
      //CommunicationsErrorCount++;
      return -1;
    }
  }

  /* Check echoed checksum */
  bytes = WBUSPORT.readBytes((char*)buf, 1);
  if (bytes != 1 || buf[0] != chksum) {
    //PRINTF("wbus_msg_send() K-Line error. %d < 1 (data2)\n", bytes);
    //CommunicationsErrorCount++;
    return -1;
  }

  return 0;
}


/*
 * Read answer from wbus
 * addr: source/destination address pair to be expected or returned in case of host I/O
 * cmd: if pointed location is zero, received client message, if not send client message with command *cmd.
 * data: buffer to either receive client data, or sent its content if client (*cmd!=0).  If *data is !=0, then *data amount of data bytes will  be skipped.
 * dlen: out: length of data.
 */
int wbus_msg_recv(uint8_t *addr,  uint8_t *cmd,  uint8_t *data,  int *dlen,  int skip)
{
  uint8_t buf[3];
  uint8_t chksum;
  int len;

  /* Read address header */
  do {
    if (WBUSPORT.readBytes((char*)buf, 1) != 1) {
      //if (*cmd != 0) {
      //PRINTF("wbus_msg_recv(): timeout\n");
      //}
      //CommunicationsErrorCount++;
      return -1;
    }

    buf[1] = buf[0];

    /* Check addresses */
  }
  while ( buf[1] != *addr );

  //rs232_blocking(wbus->rs232, 0);

  /* Read length and command */
  if (WBUSPORT.readBytes((char*)buf + 1, 2) != 2) {
    //    if (*cmd != 0) {
    //     //wbus_msg_recv(): No addr/len error
    //    }
    //CommunicationsErrorCount++;

    return -1;
  }

  {
    /* client case: check ACK */
    if (buf[2] != (*cmd | 0x80)) {
      /* Message reject happens. Do not be too picky about that. */
      *dlen = 0;
      return 0;
    }
  }

  chksum = checksum(buf, 3, 0);

  /* Read possible data */
  len = buf[1] - 2 - skip;

  if (len > 0 || skip > 0)
  {
    for (; skip > 0; skip--) {
      if (WBUSPORT.readBytes((char*)buf, 1) != 1) {
        return -1;
      }
      chksum = checksum(buf, 1, chksum);
    }

    if (len > 0) {
      if (WBUSPORT.readBytes((char*)data, len) != len) {
        return -1;
      }
      chksum = checksum(data, len, chksum);
    }
    /* Store data length */
    *dlen = len;
  }
  else {
    *dlen = 0;
  }

  /* Read and verify checksum */
  //rs232_read(wbus->rs232, buf, 1);
  WBUSPORT.readBytes((char*)buf, 1);

  if (*buf != chksum) {
    //   PRINTF("wbus_msg_recv() Checksum error\n");
    //CommunicationsErrorCount++;
    return -1;
  }

  return 0;
}

/*
 * Send a client W-Bus request and read answer from Heater.
 */
int wbus_io( uint8_t cmd, uint8_t *out, uint8_t *out2, int len2, uint8_t *in, int *dlen, int skip)
{
  int err, tries;
  int len;
  unsigned char addr;

  len = *dlen;

  //rs232_blocking(wbus->rs232, 0);

  tries = 0;
  do {
    if (tries != 0) {
      //CommunicationsErrorCount++;
      //PRINTF("wbus_io() retry: %d\n", tries);
      delay(50);
      wbus_init();
    }

    tries++;
    /* Send Message */
    addr = (WBUS_CADDR << 4) | WBUS_HADDR;
    err = wbus_msg_send( addr, cmd, out, len, out2, len2);
    if (err != 0) {
      continue;
    }

    /* Receive reply */
    addr = (WBUS_HADDR << 4) | WBUS_CADDR;
    err = wbus_msg_recv( &addr, &cmd, in, dlen, skip);
    if (err != 0) {
      continue;
    }
  }
  while (tries < 4 && err != 0);

  //Appears that there is a small 70ms delay on the real Thermotest software between requests, mimick this here which helps data reliability...
  delay(30);

  return err;
}

int wbus_ident(uint8_t identCommand, uint8_t *in) {
  uint8_t tmp;
  uint8_t tmp2[2];
  tmp = identCommand;
  len = 1;
  return wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, in, &len, 1);
}

/* Overall info */
int wbus_get_basic_info(HANDLE_BASIC_WBINFO i)
{
  int err;
//  uint8_t tmp;

//  tmp = IDENT_DEV_NAME; len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, (unsigned char*)i->dev_name, &len, 1);
//  if (err) goto bail;

  err=wbus_ident(IDENT_DEV_NAME,(unsigned char*)i->dev_name);
  if (err) goto bail;
  //Hack: Null terminate this string - len is set globally
  i->dev_name[len] = 0;

  err=wbus_ident(IDENT_DEV_ID,i->dev_id);
  if (err) goto bail;

  err=wbus_ident(IDENT_DOM_CU,i->dom_cu);
  if (err) goto bail;

  err=wbus_ident(IDENT_DOM_HT,i->dom_ht);
  if (err) goto bail;

  err=wbus_ident(IDENT_CUSTID,i->customer_id);
  if (err) goto bail;

  err=wbus_ident(IDENT_SERIAL,i->serial);
  if (err) goto bail;

  //tmp = IDENT_DEV_ID;  len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, i->dev_id, &len, 1);
  //if (err) goto bail;
  //tmp = IDENT_DOM_CU;  len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, i->dom_cu, &len, 1);
  //if (err) goto bail;
  //tmp = IDENT_DOM_HT;  len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, i->dom_ht, &len, 1);
  //if (err) goto bail;
  //tmp = IDENT_CUSTID;  len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, i->customer_id, &len, 1);
  //if (err) goto bail;
  //tmp = IDENT_SERIAL;  len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, i->serial, &len, 1);
  //if (err) goto bail;

bail:
  return err;
}


/* Sensor access */
int wbus_sensor_read(HANDLE_WBSENSOR sensor, int idx)
{
  int err  = 0;
  uint8_t sen;

  /* Tweak: skip some addresses since reading them just causes long delays. */
  switch (idx) {
    default:
      break;
    case 0:
    case 1:
    case 8:
    case 9:
    case 13:
    case 14:
    case 16:
      sensor->length = 0;
      sensor->idx = 0xff;
      return -1;
      break;
  }

  sen = idx;
  len = 1;
  err = wbus_io(WBUS_CMD_QUERY, &sen, NULL, 0, sensor->value, &len, 1);
  if (err != 0)
  {
    //PRINTF("Reading sensor %d failed\n", );
    sensor->length = 0;
  } else {
    /* Store length of received value */
    sensor->length = len;
    sensor->idx = idx;
  }

  return err;
}

/* Turn heater on for time minutes */
int wbus_turnOn(unsigned char cmd, unsigned char time)
{
  len = 1;
  cmdbuf[0] = time;
  return wbus_io(cmd, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

/* Check current command */
int wbus_check(unsigned char mode)
{
  len = 2;
  //unsigned char tmp[3];
  cmdbuf[0] = mode;
  cmdbuf[1] = 0;
  return wbus_io(WBUS_CMD_CHK, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

/* Turn heater off */
int wbus_turnOff()
{
  len = 0;
  return wbus_io(WBUS_CMD_OFF, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

int wbus_fuelPrime( unsigned char time)
{
  cmdbuf[0] = 0x03;
  cmdbuf[1] = 0x00;
  cmdbuf[2] = time >> 1;
  len = 3;

  return wbus_io(WBUS_CMD_X, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

int wbus_get_fault_count(unsigned char ErrorList[32]) {
  unsigned char tmp[2];

  tmp[0] = ERR_LIST;
  len = 1;
  return wbus_io(WBUS_CMD_ERR, tmp, NULL, 0, ErrorList, &len, 1);
}

int wbus_clear_faults() {
  uint8_t tmp;
  tmp = ERR_DEL;
  len = 1;
  return wbus_io(WBUS_CMD_ERR, &tmp, NULL, 0, &tmp, &len, 0);
}

int wbus_get_fault(unsigned char ErrorNumber, HANDLE_ERRINFO errorInfo) {
  len = 2;
  cmdbuf[0] = ERR_READ;
  cmdbuf[1] = ErrorNumber;
  return wbus_io(WBUS_CMD_ERR, cmdbuf, NULL, 0, (unsigned char*)errorInfo, &len, 1);
}
