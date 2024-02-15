# PDM

Schematic mockup:
![obraz](https://github.com/PGRacing/PDM/assets/78111197/ae750abc-0e28-4523-a7b9-82eb0c457f79)

TODO on next PCB:
| NAME | DIFFICULTY | PRIORITY | STATUS |
| --- | --- | --- | --- |
| 1. Add fuse for external sensors supply (100mA). Can also be solved with second 5V regulator for sensors + fuse. | 1 | 3 | DONE - Added external LDO's to 3.3V and 5V |
| 2. Change CPC1117N to CPC1017N (or diffrent PhotoMos) | 1 | 2 | DONE - Added diffrent (smaller) Opto-MOS |
| 3. Add clamping diodes on any signals | 2 | 3 | Added zenner + shottky clippers |
| 4. Change input filtration for low profile SMD capacitor instead of tht | 1 | 1 | DEPRECEATED  |
| 5. Add SWO pin | 2 | 2 | DONE |
| 6. Current sensors output should be collected by one ADC !!! - for eg. ADC3 (change pinout) | 3 | 2 | DONE |
| 7. Add clamping diodes on current sensor to allow lower range current detecion | 2 | 3 | DONE |
| 8. Selection and redirection of IO is not imporant | 1 | 1 | DONE - (only four channels) |
| 9. Fix WS2812B footprint | 1 | 3 | DONE |
| 10. Use smaller footprint for voltage mux and can transceiver | 2 | 1 | DONE - changed voltage mux to 16 channel, CAN transceiver same |
| 11. Change FET's to BTS50010-1LUA | 3 | 2 |  DONE |
| 12. Add RX/TX information leds to CAN bus (software driven) | 1 | 1 | DEPERECATED |
| 13. Add MUX for mutlichannel switch | 2 | 2 | DONE - added 16 channel mux |
| 14. Add CAN software switching | 2 | 2 | DONE |
| 14. Remove additional header for stlink | 2 | 2 | DONE |
