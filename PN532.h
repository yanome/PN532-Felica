#ifndef PN532_H
#define PN532_H

#include <Wire.h>

// #define PN532_DEBUG

#define PN532_RDY_TIMEOUT 10 // ms
#define PN532_BUFFER_SIZE 64
#define PN532_I2C_ADDRESS 0x48 >> 1
#define PN532_CMD_SAMCONFIGURATION 0x14
#define PN532_CMD_INLISTPASSIVETARGET 0x4A
#define PN532_CMD_INDATAEXCHANGE 0x40
#define PN532_CMD_INDESELECT 0x44
#define PN532_PREAMBLE 0x00
#define PN532_STARTCODE1 0x00
#define PN532_STARTCODE2 0xFF
#define PN532_TFI_SEND 0xD4
#define PN532_TFI_READ 0xD5
#define PN532_POSTAMBLE 0x00
#define PN532_ACK_LEN 6
#define PN532_FELICA_212 0x01

class PN532 {
public:
  PN532(TwoWire *wire);
  bool begin();
  bool cmd_InListPassiveTarget(uint8_t type, uint8_t *data, uint8_t *len);
  bool cmd_InDataExchange(uint8_t *data, uint8_t *len);
  bool cmd_InDeselect();

private:
  TwoWire *_wire;
  uint8_t _buffer[PN532_BUFFER_SIZE];
  const uint8_t ack[PN532_ACK_LEN] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

  bool send(uint8_t len);
  bool read(uint8_t cmd, uint8_t *len);
  bool read_ack();
  bool wait_rdy(uint8_t len);
  bool cmd_SAMConfiguration();
};

#endif