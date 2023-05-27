#ifndef FELICA_H
#define FELICA_H

#include <PN532.h>

// #define FELICA_DEBUG

#define FELICA_BUFFER_SIZE 64
#define FELICA_MAX_SYSTEMS 4
#define FELICA_MAX_SERVICES 128
#define FELICA_CMD_POLLING 0x00
#define FELICA_CMD_REQUESTSYSTEMCODE 0x0C
#define FELICA_CMD_SEARCHSERVICECODE 0x0A
#define FELICA_CMD_READWITHOUTENCRYPTION 0x06
#define FELICA_CMD_WRITEWITHOUTENCRYPTION 0x08
#define FELICA_ID_LENGTH 8
#define FELICA_BLOCK_SIZE 16
#define FELICA_SYSTEM_CODE 0xFFFF    // node code used to indicate system
#define FELICA_ATTRIBUTE_MASK 0x3F   // 6 bits
#define FELICA_AREA_ATTRIBUTE_0 0x00 // area that can create sub-area
#define FELICA_AREA_ATTRIBUTE_1 0x01 // area that cannot create sub-area
#define FELICA_SERVICE_ATTRIBUTE_NO_ENCRYPTION                                 \
  0x01 // authentication not required
#define FELICA_SERVICE_END_FLAG 0xA8

class Felica {
public:
  uint8_t IDm[FELICA_ID_LENGTH];
  uint8_t system_count;
  uint16_t systems[FELICA_MAX_SYSTEMS];
  uint8_t service_count;
  uint16_t services[FELICA_MAX_SERVICES];

  Felica(PN532 *pn532);
  bool begin();
  bool cmd_Polling();
  bool cmd_RequestSystemCode();
  bool cmd_SearchServiceCode(uint8_t system);
  bool can_read_service(uint8_t service);
  bool read_service(uint8_t service, uint8_t *len,
                    uint8_t (*data)[FELICA_BLOCK_SIZE]);
  bool write_random_service(uint8_t service, uint8_t len,
                            uint8_t (*data)[FELICA_BLOCK_SIZE]);

private:
  PN532 *_pn532;
  uint8_t _buffer[FELICA_BUFFER_SIZE];

  bool pn532_send_read(uint8_t cmd, uint8_t *len);
  bool cmd_ReadWithoutEncryption(uint8_t service, uint8_t block, bool *end,
                                 uint8_t data[FELICA_BLOCK_SIZE]);
  bool cmd_WriteWithoutEncryption(uint8_t service, uint8_t block,
                                  uint8_t data[FELICA_BLOCK_SIZE]);
};

#endif