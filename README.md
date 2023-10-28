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

1. Add fuse for external sensors supply (100mA)
2. Change CPC1117N to CPC1017N
3. Add clamping diodes on any signals
4. Change input filtration for low profile SMD capacitor instead of tht
5. Add SWO pin
6. Current sensors output should be collected by one ADC !!! - for eg. ADC3 (change pinout)
7. Add clamping diodes on current sensor to allow lower range current detecion
8. Selection and redirection od IO is not imporant
9. Fix WS2812B footprint
10. Use smaller footprint for voltage mux and can transceiver
11. Change FET's to BTS50010-1LUA
