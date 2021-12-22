#include <Felica.h>

Felica::Felica(PN532 *pn532) { _pn532 = pn532; }

bool Felica::begin() { return _pn532->begin(); }

bool Felica::pn532_send_read(uint8_t cmd, uint8_t *len) {
  memmove(2 + FELICA_ID_LENGTH + _buffer, _buffer, *len);
  *len += 2 + FELICA_ID_LENGTH; // len + cmd + IDm
  _buffer[0] = *len;
  _buffer[1] = cmd;
  memcpy(2 + _buffer, IDm, FELICA_ID_LENGTH);
  if (_pn532->cmd_InDataExchange(_buffer, len)) {
    if (_buffer[0] == *len && _buffer[1] == 1 + cmd &&
        !memcmp(IDm, 2 + _buffer, FELICA_ID_LENGTH)) {
      *len -= 2 + FELICA_ID_LENGTH;
      memmove(_buffer, 2 + FELICA_ID_LENGTH + _buffer, *len);
      return true;
    }
#ifdef FELICA_DEBUG
    Serial.println("Unexpected command response");
#endif
  }
  return false;
}

bool Felica::cmd_Polling() {
  uint8_t len;

  _buffer[0] = FELICA_CMD_POLLING;
  _buffer[1] = 0xFF; // wildcard system code
  _buffer[2] = 0xFF; // wildcard system code
  _buffer[3] = 0x00; // request code (no request)
  _buffer[4] = 0x00; // time slots (1)
  len = 5;
  if (_pn532->cmd_InListPassiveTarget(PN532_FELICA_212, _buffer, &len)) {
    if (_buffer[0] == len &&
        2 + 2 * FELICA_ID_LENGTH == len // len + cmd + IDm + PMm
        && _buffer[1] == 1 + FELICA_CMD_POLLING) {
      memcpy(IDm, 2 + _buffer, FELICA_ID_LENGTH);
      // PMm is also returned but ignored
      return true;
    }
#ifdef FELICA_DEBUG
    Serial.println("Unexpected polling command response");
#endif
  }
  return false;
}

bool Felica::cmd_RequestSystemCode() {
  uint8_t len, idx;

  len = 0;
  if (pn532_send_read(FELICA_CMD_REQUESTSYSTEMCODE, &len)) {
    if (len == 1 + 2 * _buffer[0]) {
      if (_buffer[0] > FELICA_MAX_SYSTEMS) {
        system_count = FELICA_MAX_SYSTEMS;
#ifdef FELICA_DEBUG
        Serial.println("Cannot store all systems info");
#endif
      } else {
        system_count = _buffer[0];
      }
      for (idx = 0; idx < system_count; ++idx) {
        systems[idx] = (_buffer[1 + 2 * idx] << 8) + _buffer[2 + 2 * idx];
      }
      return true;
    }
#ifdef FELICA_DEBUG
    Serial.println("Invalid systems list");
#endif
  }
  return false;
}

bool Felica::cmd_SearchServiceCode(uint8_t system) {
  uint8_t len, attribute;
  uint16_t idx, service;
  bool is_area;

  IDm[0] = (0x0F & IDm[0]) | (system << 4); // upper 4 bits of IDm[0]
  service_count = 0;
  idx = 0;
  while (true) {
    _buffer[0] = idx;
    _buffer[1] = idx >> 8;
    len = 2;
    if (!pn532_send_read(FELICA_CMD_SEARCHSERVICECODE, &len)) {
      break;
    }
    service = (_buffer[1] << 8) + _buffer[0]; // little endian
    attribute = FELICA_ATTRIBUTE_MASK & service;
    is_area = FELICA_AREA_ATTRIBUTE_0 == attribute ||
              FELICA_AREA_ATTRIBUTE_1 == attribute;
    if (is_area && 4 == len) {
// area information is discarded
#ifdef FELICA_DEBUG
      Serial.printf("Found area with range 0x%02X%02X - 0x%02X%02X\r\n",
                    _buffer[1], _buffer[0], _buffer[3], _buffer[2]);
#endif
    } else if (!is_area && 2 == len) {
      if (FELICA_SYSTEM_CODE == service) { // end
        return true;
      }
      if (FELICA_MAX_SERVICES == service_count) {
#ifdef FELICA_DEBUG
        Serial.println("Cannot store all services info");
#endif
        return true;
      }
      services[service_count] = service;
      ++service_count;
    } else {
#ifdef FELICA_DEBUG
      Serial.println("Invalid response to search command");
#endif
      break;
    }
    ++idx;
  }
  return false;
}

bool Felica::cmd_ReadWithoutEncryption(uint8_t service, uint8_t block,
                                       bool *end, uint8_t *data) {
  uint8_t len;

  _buffer[0] = 0x01;              // number of services
  _buffer[1] = services[service]; // service list: service, little endian
  _buffer[2] = services[service] >> 8;
  _buffer[3] = 0x01;  // number of blocks
  _buffer[4] = 0x80;  // block list: length (2 bytes), access mode (read),
                      // service list element
  _buffer[5] = block; // block list: block number
  len = 6;
  if (pn532_send_read(FELICA_CMD_READWITHOUTENCRYPTION, &len)) {
    if (3 + FELICA_BLOCK_SIZE ==
            len        // flag1 + flag2 + number of blocks + block size
        && !_buffer[0] // flag1
        && !_buffer[1] // flag2
        && 0x01 == _buffer[2]) { // number of blocks
      memcpy(data, 3 + _buffer, FELICA_BLOCK_SIZE);
      *end = false;
      return true;
    } else if (2 == len && _buffer[0]                      // flag1
               && FELICA_SERVICE_END_FLAG == _buffer[1]) { // flag2
      *end = true;
      return true;
    }
#ifdef FELICA_DEBUG
    Serial.println("Invalid response to read command");
#endif
  }
  return false;
}

bool Felica::can_read_service(uint8_t service) {
  return FELICA_SERVICE_ATTRIBUTE_NO_ENCRYPTION & services[service];
}

bool Felica::read_service(uint8_t service, uint8_t *len,
                          uint8_t (*data)[FELICA_BLOCK_SIZE]) {
  uint8_t idx;
  bool end;

  for (idx = 0; idx < *len; ++idx) {
    if (!cmd_ReadWithoutEncryption(service, idx, &end, *(data + idx))) {
      return false;
    }
    if (end) {
      *len = idx;
      return true;
    }
  }
#ifdef FELICA_DEBUG
  Serial.println("Cannot read all blocks");
#endif
  return true;
}