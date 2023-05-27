# PN532-Felica
Library to read Felica cards using boards with the PN532 NFC chip.

I have a [DFRobot NFC Module](https://www.dfrobot.com/product-1917.html), but this library should work with other systems using the PN532 chip.

Implemented PN532 commands:
- `SAMConfiguration`
- `InListPassiveTarget`
- `InDataExchange`
- `InDeselect`

Felica-specific commands:
- `Polling`
- `Request System Code`
- `Search Service Code`
- `Read Without Encryption`
- `Write Without Encryption`

The `Search Service Code` command does not have publicly available documentation. My implementation emulates the `felica_search_service` function from [libpafe](https://github.com/rfujita/libpafe).
