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

WbusInterface::WbusInterface(HardwareSerial &refSer)
:serial(refSer)
{
  init();
}

void WbusInterface::init() {
  serial.end();
  delay(200);
  //Break set

#if defined(__AVR_ATmega328P__)
  //This code is for Arduino ATMEGA328 with single hardware serial port (pins 0 and 1)
  //PORTD=digital pins 0 to 7
  DDRD = DDRD | B00000010;  //Pin1 = output
  PORTD = PORTD | B00000010; // digital 1 HIGH
  delay(25);
  PORTD = PORTD & B11111101; // digital 1 LOW
  delay(25);
#endif

  // initialize serial communication at 2400 bits per second, 8 bit data, even parity and 1 stop bit
  serial.begin(2400, SERIAL_8E1);

  //250ms for timeouts
  serial.setTimeout(1000);
  // Empty all queues. BRK toggling may cause a false received byte (or more than one who knows).
  while (serial.available()) serial.read();
}


/*
 * \param buf pointer to ACK message part
 * \param len length of data
 * \param chk initial value of the checksum. Useful for concatenating.
 */
unsigned char WbusInterface::checksum(unsigned char *buf, unsigned char len, unsigned char chk)
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
int WbusInterface::send( uint8_t addr,
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
  serial.write(buf, 3);
  serial.write(data, len);
  serial.write(data2, len2);
  serial.write(&chksum, 1);

  /* Read and check echoed header */
  bytes = serial.readBytes((char*)buf, 3);
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
    bytes = serial.readBytes((char*)buf, 1);
    if (bytes != 1 || buf[0] != data[i]) {
      //PRINTF("wbus_msg_send() K-Line error. %d < 1 (data2)\n", bytes);
      //CommunicationsErrorCount++;
      return -1;
    }
  }

  for (i = 0; i < len2; i++) {
    bytes = serial.readBytes((char*)buf, 1);
    if (bytes != 1 || buf[0] != data2[i]) {
      //PRINTF("wbus_msg_send() K-Line error. %d < 1 (data2)\n", bytes);
      //CommunicationsErrorCount++;
      return -1;
    }
  }

  /* Check echoed checksum */
  bytes = serial.readBytes((char*)buf, 1);
  if (bytes != 1 || buf[0] != chksum) {
    //PRINTF("wbus_msg_send() K-Line error. %d < 1 (data2)\n", bytes);
    //CommunicationsErrorCount++;
    return -1;
  }

  return 0;
}

int WbusInterface::listen(const uint8_t *addr,  uint8_t *cmd,  uint8_t *data,  int *dlen)
{
  uint8_t buf[3];
  uint8_t chksum = 0;
  int len = 0;

  do{
    if(serial.readBytes((char*)buf, 1) != 1) {
      DEBUGPORT.print("rcv timeout 0");
      return -1;
    }
  }
  while ( buf[0] != *addr );
  DEBUGPORT.print("wbus_msg_recv(): received cmd ");

  /* Read length and command */
  if (serial.readBytes((char*)buf + 1, 2) != 2) {
    DEBUGPORT.println("rcv timeout 1");
    return -1;
  }
  *cmd = buf[2];
  DEBUGPORT.println(*cmd,HEX);
  chksum = checksum(buf, 3, 0);

  /* Read possible data */
  len = buf[1] - 2;
  *dlen = 0;

  if (len > 0 )
  {
    if ((int)serial.readBytes((char*)data, len) != len) {
        DEBUGPORT.println("rcv timeout 2");
        return -1;
      }
      chksum = checksum(data, len, chksum);
    }
  /* Store data length */
  *dlen = len;

  /* Read and verify checksum */
  serial.readBytes((char*)buf, 1);

  if (*buf != chksum) {
    DEBUGPORT.println("rcv chksum err");
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
int WbusInterface::recvAns(const uint8_t *addr,  const uint8_t *cmd,  uint8_t *data,  int *dlen,  int skip)
{
  uint8_t buf[3];
  uint8_t chksum;
  int len;

  /* Read address header */
  do {
    if (serial.readBytes((char*)buf, 1) != 1) {
      //if (*cmd != 0) {
      //}
      //CommunicationsErrorCount++;
      return -3;
    }

    buf[1] = buf[0];

    /* Check addresses */
  }
  while ( buf[1] != *addr );

  /* Read length and command */
  if (serial.readBytes((char*)buf + 1, 2) != 2) {
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
      DEBUGPORT.print("+");
      return 0;
    }
  }
  chksum = checksum(buf, 3, 0);

  /* Read possible data */
  len = buf[1] - 2 - skip;

  if (len > 0 || skip > 0)
  {
    for (; skip > 0; skip--) {
      if (serial.readBytes((char*)buf, 1) != 1) {
        DEBUGPORT.print("*");
        return -1;
      }
      chksum = checksum(buf, 1, chksum);
    }

    if (len > 0) {
      if ((int)serial.readBytes((char*)data, len) != len) {
        DEBUGPORT.print(":");
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
  serial.readBytes((char*)buf, 1);

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
int WbusInterface::io( uint8_t cmd, uint8_t *out, uint8_t *out2, int len2, uint8_t *in, int *dlen, int skip)
{
  int err, tries;
  int len;
  unsigned char addr;

  len = *dlen;

  tries = 0;
  do {
    if (tries != 0) {
      //CommunicationsErrorCount++;
      DEBUGPORT.println("wbus_io() retry");
      delay(50);
      init();
    }

    tries++;
    /* Send Message */
    addr = (WBUS_CADDR << 4) | WBUS_HADDR;
    err = send( addr, cmd, out, len, out2, len2);
    if (err != 0) {
      continue;
    }

    /* Receive reply */
    addr = (WBUS_HADDR << 4) | WBUS_CADDR;
    err = recvAns( &addr, &cmd, in, dlen, skip);
    if (err != 0) {
      continue;
    }
  }
  while (tries < 4 && err != 0);

  //Appears that there is a small 70ms delay on the real Thermotest software between requests, mimick this here which helps data reliability...
  delay(30);

  return err;
}

int WbusInterface::ident(uint8_t identCommand, uint8_t *in) {
  uint8_t tmp;
  tmp = identCommand;
  len = 1;
  return io( WBUS_CMD_IDENT, &tmp, NULL, 0, in, &len, 1);
}

/* Overall info */
int WbusInterface::get_basic_info(HANDLE_BASIC_WBINFO i)
{
  int err;
//  uint8_t tmp;

//  tmp = IDENT_DEV_NAME; len = 1; err = wbus_io( WBUS_CMD_IDENT, &tmp, NULL, 0, (unsigned char*)i->dev_name, &len, 1);
//  if (err) goto bail;

  err=ident(IDENT_DEV_NAME,(unsigned char*)i->dev_name);
  if (err) goto bail;
  //Hack: Null terminate this string - len is set globally
  i->dev_name[len] = 0;

  err=ident(IDENT_DEV_ID,i->dev_id);
  if (err) goto bail;

  err=ident(IDENT_DOM_CU,i->dom_cu);
  if (err) goto bail;

  err=ident(IDENT_DOM_HT,i->dom_ht);
  if (err) goto bail;

  err=ident(IDENT_CUSTID,i->customer_id);
  if (err) goto bail;

  err=ident(IDENT_SERIAL,i->serial);
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
int WbusInterface::sensor_read(HANDLE_WBSENSOR sensor, int idx)
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
  err = io(WBUS_CMD_QUERY, &sen, NULL, 0, sensor->value, &len, 1);
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
int WbusInterface::turnOn(unsigned char cmd, unsigned char time)
{
  len = 1;
  cmdbuf[0] = time;
  return io(cmd, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

/* Check current command */
int WbusInterface::check(unsigned char mode)
{
  len = 2;
  //unsigned char tmp[3];
  cmdbuf[0] = mode;
  cmdbuf[1] = 0;
  return io(WBUS_CMD_CHK, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

/* Turn heater off */
int WbusInterface::turnOff()
{
  len = 0;
  return io(WBUS_CMD_OFF, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

int WbusInterface::fuelPrime( unsigned char time)
{
  cmdbuf[0] = 0x03;
  cmdbuf[1] = 0x00;
  cmdbuf[2] = time >> 1;
  len = 3;

  return io(WBUS_CMD_X, cmdbuf, NULL, 0, cmdbuf, &len, 0);
}

int WbusInterface::get_fault_count(unsigned char ErrorList[32]) {
  unsigned char tmp[2];

  tmp[0] = ERR_LIST;
  len = 1;
  return io(WBUS_CMD_ERR, tmp, NULL, 0, ErrorList, &len, 1);
}

int WbusInterface::clear_faults() {
  uint8_t tmp;
  tmp = ERR_DEL;
  len = 1;
  return io(WBUS_CMD_ERR, &tmp, NULL, 0, &tmp, &len, 0);
}

int WbusInterface::get_fault(unsigned char ErrorNumber, HANDLE_ERRINFO errorInfo) {
  len = 2;
  cmdbuf[0] = ERR_READ;
  cmdbuf[1] = ErrorNumber;
  return io(WBUS_CMD_ERR, cmdbuf, NULL, 0, (unsigned char*)errorInfo, &len, 1);
}
