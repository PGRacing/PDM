# PDM

Schematic mockup:
![mockup drawio(1)](https://user-images.githubusercontent.com/78111197/210089409-ce1f0308-e05c-4726-afdd-d9baeaa8d556.png)

TODO:

1. Find better temperature sensor - maybe just NTC ones
2. Add ADC over/undervoltage protection and voltage divider on each ADC input 5V->3.3V
3. Check for ADC performance in high current enviroment
4. Current limiting on VNF1048 using PWM contolled PMOS between charge-pump output and Power-MOS gate.
5. Add buck-converter for 3.3V automotive grade only!

TODO on next PCB:
| NAME | DIFFICULTY | PRIORITY | STATUS |
| --- | --- | --- | --- |
| 1. Add fuse for external sensors supply (100mA). Can also be solved with second 5V regulator for sensors + fuse. | 1 | 3 | |
| 2. Change CPC1117N to CPC1017N | 1 | 2 ||
| 3. Add clamping diodes on any signals | 2 | 3 | |
| 4. Change input filtration for low profile SMD capacitor instead of tht | 1 | 1 | |
| 5. Add SWO pin | 2 | 2 | |
| 6. Current sensors output should be collected by one ADC !!! - for eg. ADC3 (change pinout) | 3 | 2 | |
| 7. Add clamping diodes on current sensor to allow lower range current detecion | 2 | 3 | |
| 8. Selection and redirection of IO is not imporant | 1 | 1 ||
| 9. Fix WS2812B footprint | 1 | 3 | |
| 10. Use smaller footprint for voltage mux and can transceiver | 2 | 1 | |
| 11. Change FET's to BTS50010-1LUA | 3 | 2 ||
| 12. Add RX/TX information leds to CAN bus (software driven) | 1 | 1 ||
| 13. Add MUX for mutlichannel switch | 2 | 2 ||
