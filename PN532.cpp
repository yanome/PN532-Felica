#include <PN532.h>

PN532::PN532(TwoWire *wire) { _wire = wire; }

bool PN532::send(uint8_t len) {
  uint8_t checksum, idx;

  _wire->beginTransmission(PN532_I2C_ADDRESS);
  _wire->send(PN532_PREAMBLE);
  _wire->send(PN532_STARTCODE1);
  _wire->send(PN532_STARTCODE2);
  _wire->send(1 + len);    // TFI(1) + data
  _wire->send(0xFF - len); // LCS checksum
  _wire->send(PN532_TFI_SEND);
  checksum = PN532_TFI_SEND;
  for (idx = 0; idx < len; ++idx) {
    _wire->send(_buffer[idx]);
    checksum += _buffer[idx];
  }
  _wire->send(0x00 - checksum);
  _wire->send(PN532_POSTAMBLE);
  _wire->endTransmission();
  return read_ack();
}

bool PN532::read(uint8_t cmd, uint8_t *len) {
  uint8_t checksum, idx;

  if (!wait_rdy(PN532_BUFFER_SIZE)) {
    return false;
  }
  if (_wire->read() != PN532_PREAMBLE || _wire->read() != PN532_STARTCODE1 ||
      _wire->read() != PN532_STARTCODE2) {
#ifdef PN532_DEBUG
    Serial.println("Invalid frame start");
#endif
    return false;
  }
  *len = _wire->read();
  if (0xFF & (*len + _wire->read())) {
#ifdef PN532_DEBUG
    Serial.println("Invalid checksum");
#endif
    return false;
  }
  if (_wire->read() != PN532_TFI_READ || _wire->read() != cmd) {
#ifdef PN532_DEBUG
    Serial.println("Invalid response");
#endif
    return false;
  }
  checksum = PN532_TFI_READ + cmd;
  *len -= 2; // length - TFI - PD0
  for (idx = 0; idx < *len; ++idx) {
    _buffer[idx] = _wire->read();
    checksum += _buffer[idx];
  }
  if (0xFF & (checksum + _wire->read())) {
#ifdef PN532_DEBUG
    Serial.println("Invalid checksum");
#endif
    return false;
  }
  if (_wire->read() != PN532_POSTAMBLE) {
#ifdef PN532_DEBUG
    Serial.println("Invalid frame end");
#endif
    return false;
  }
  return true;
}

bool PN532::read_ack() {
  uint8_t idx;

  if (!wait_rdy(PN532_ACK_LEN)) {
    return false;
  }
  for (idx = 0; idx < PN532_ACK_LEN; ++idx) {
    if (ack[idx] != _wire->read()) {
#ifdef PN532_DEBUG
      Serial.println("Invalid ACK");
#endif
      return false;
    }
  }
  return true;
}

bool PN532::wait_rdy(uint8_t len) {
  uint8_t timeout = PN532_RDY_TIMEOUT;

  while (timeout > 0) {
    delay(1);
    _wire->requestFrom(PN532_I2C_ADDRESS, 1 + len);
    if (0x01 & _wire->read()) {
      break;
    }
    --timeout;
  }
#ifdef PN532_DEBUG
  if (!timeout) {
    Serial.println("Timeout waiting for RDY status");
  }
#endif
  return timeout;
}

bool PN532::begin() {
  _wire->begin();
  return cmd_SAMConfiguration();
}

bool PN532::cmd_SAMConfiguration() {
  uint8_t len;

  _buffer[0] = PN532_CMD_SAMCONFIGURATION;
  _buffer[1] = 0x01; // normal mode
  _buffer[2] = 0x14; // 1s timeout (20 x 50ms)
  _buffer[3] = 0x00; // don't use IRQ pin
  return send(4) && read(1 + PN532_CMD_SAMCONFIGURATION, &len);
}

bool PN532::cmd_InListPassiveTarget(uint8_t type, uint8_t *data, uint8_t *len) {
  _buffer[0] = PN532_CMD_INLISTPASSIVETARGET;
  _buffer[1] = 0x01; // max targets
  _buffer[2] = type;
  memcpy(3 + _buffer, data, *len);
  if (send(3 + *len) && read(1 + PN532_CMD_INLISTPASSIVETARGET, len)) {
    if (0x01 == _buffer[0]       // number of targets
        && 0x01 == _buffer[1]) { // target 1
      *len -= 2;
      memcpy(data, 2 + _buffer, *len);
      return true;
    }
#ifdef PN532_DEBUG
    Serial.println("No target found");
#endif
  }
  return false;
}

bool PN532::cmd_InDataExchange(uint8_t *data, uint8_t *len) {
  _buffer[0] = PN532_CMD_INDATAEXCHANGE;
  _buffer[1] = 0x01; // target 1
  memcpy(2 + _buffer, data, *len);
  if (send(2 + *len) && read(1 + PN532_CMD_INDATAEXCHANGE, len)) {
    if (!_buffer[0]) { // status
      --*len;
      memcpy(data, 1 + _buffer, *len);
      return true;
    }
#ifdef PN532_DEBUG
    Serial.printf("Data exchange error %02X\r\n", _buffer[0]);
#endif
  }
  return false;
}

bool PN532::cmd_InDeselect() {
  uint8_t len;

  _buffer[0] = PN532_CMD_INDESELECT;
  _buffer[1] = 0x00; // all targets
  if (send(2) && read(1 + PN532_CMD_INDESELECT, &len)) {
    if (!_buffer[0]) { // status
      return true;
    }
#ifdef PN532_DEBUG
    Serial.printf("Deselect error %02X\r\n", _buffer[0]);
#endif
  }
  return false;
}