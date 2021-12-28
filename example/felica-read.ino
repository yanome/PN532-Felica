#include <Felica.h>

#define BLOCKS 64

PN532 pn532(&Wire);
Felica felica(&pn532);

void setup() {
  while (!felica.begin()) {
    delay(1000);
  }
}

void loop() {
  uint8_t idx, system, service, blocks, block;
  uint8_t data[BLOCKS][FELICA_BLOCK_SIZE];
  bool ok = true;

  // polling
  if (felica.cmd_Polling()) {
    Serial.printf("Found card ");
    for (idx = 0; idx < FELICA_ID_LENGTH; ++idx) {
      Serial.printf("%02X ", felica.IDm[idx]);
    }
    Serial.println();
  } else {
    Serial.println("No card found");
    ok = false;
  }

  // get systems
  if (ok) {
    if (felica.cmd_RequestSystemCode()) {
      Serial.printf("Found %u systems\r\n", felica.system_count);
    } else {
      Serial.println("Error getting systems");
      ok = false;
    }
  }

  // get services
  for (system = 0; system < felica.system_count; ++system) {
    if (ok) {
      Serial.printf("System %u has code %04X\r\n", system,
                    felica.systems[system]);
      if (felica.cmd_SearchServiceCode(system)) {
        Serial.printf("Found %u services\r\n", felica.service_count);

        // read services
        for (service = 0; service < felica.service_count; ++service) {
          Serial.printf("Service %u has code %04X\r\n", service,
                        felica.services[service]);
          if (felica.can_read_service(service)) {
            blocks = BLOCKS;
            if (felica.read_service(service, &blocks, data)) {
              for (block = 0; block < blocks; ++block) {
                Serial.printf("%02X | ", block);
                for (idx = 0; idx < FELICA_BLOCK_SIZE; ++idx) {
                  Serial.printf("%02X ", data[block][idx]);
                }
                Serial.println();
              }
            } else {
              Serial.println("Error reading service");
              ok = false;
              break;
            }
          }
        }

      } else {
        Serial.println("Error getting services");
        ok = false;
      }
    }
  }

  delay(5000);
}
