# PDM

Schematic mockup:
![obraz](https://github.com/PGRacing/PDM/assets/78111197/ae750abc-0e28-4523-a7b9-82eb0c457f79)

TODO on next PCB:

Rev 4.3
| ID | NAME | CAUSE |DIFFICULTY | PRIORITY | STATUS |
| --- | --- | --- | --- | --- | --- |
| 1. | Create test board for BTS50025-1TAE | Switch with higher Rds(on) should perform better in terms of diagnostics on most used loads | 1 | 3 | | 
| 2. | Create schematic and brd for imporoved power delivery section optimized for autmotive use based on MP4322 TLS850F2TAV50, TVS diodes + filtering | Current solution with MP2332 has maximum rating of 18V, which is really for load dumps | 3 | 3 || 
| 3. | Divide task to "safety critical real-time" hardware based (timer) ones and other normal operation tasks | Current solution can cause safety issus | 3 | 3 || 
| 4. | Prepare I^2t wiring harness protection | This should allow better WH protection| 3 | 2 || 
| 5. | Prepare OCP using curve, not only single value | This should allow better WH protection | 2 | 3 || 
| 6. | Use buffers for MCU protection | Protect MCU I/O pins | 1 | 2 || 
| 7. | Change zenner protection diodes to extended linearity region of Is switch output || 1 | 3 || 
| 8. | Add better filtering (change RC filter values) on Is output | Is output on inductive signals or fast switched signals HFreq PWM used causes invalid readings | 1 | 3 || 
| 9. | Detect battery under-voltage and UV shutdown for battery protection || 1 | 2 || 
| 10. | Configuration over USB | For sake of user-friendly solution | 3 | 2 || 
| 11. | CAN inputs support || 2 | 2 || 
| 12. | Thermal sensing | Safety critical | 2 | 2 || 
| 13. | Better 3D printed caseing (take care of screws outside of caseing envelope) + add honeycomb reinforcment | Safety critical for splash proof | 2 | 1 || 
| 14. | Add logic-level shifter for LED's | Some LED's turn red on random order | 1 | 3 || 
| 15. | Add separate voltage source for ARGB LED's and CAN bus | working can bus is critical for system integrity | 1 | 3 || 
| 16. | Add external voltage sources diagnosis (at least voltage on output) || 2 | 3 || 
| 17. | Add battery insulation and reverse-polarity + huge TVS and maybe Gas Discharge Tubes | Load dumps + cold crank etc. | 3 | 2 || 
| 18. | Add better connector for BAT+ | 1 | 3 || 
| 19. | Add limp home mode when any peripheral is malfunctioning (detect those malfunctions) | 2 | 3 || 
| 20. | Add TVS or zenner on all inputs (in limit of absolute maximium rating) near connector | 1 | 3 || 
| 21. | Add external or on-board probes for temperature sensing | 2 | 2 || 
| 22. | Add more state/debug led's | 1 | 1 || 

Rev. 4.1
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

