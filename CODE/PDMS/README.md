# PDMS 

### DESCRIPTION
This repository contains code that is targeted for PDM - Power distribution module. PDMS is custom PDM mainly created for motorcycles and single-seater vehicles (formula student). Making changes in code and configurations can have critical impact on vehicle and driver safety. After changing configuration ALWAYS verify it in secure test enviroment. 

### NOTE
- This is code that should be used with PDMS v4.1 or PDMS v4.2 board
- Before usage verify MCU I/O configuration and mapping

### INSTALL

0. Install choco package manager: https://chocolatey.org/install

1. Install Visual Studio code as it is used as IDE for this project:
` > choco install vscode`

2. Install following extensions for VS Code: 
    - [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
    - [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
    - [Makefile Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.makefile-tools)



3. Check if cygwin is installed if not type: 
` > choco install cygwin` 

4. Check if you have ARM embedded toolchain installed, version at least 10.3.1 required. In project directory type: 
` > arm-none-eabi-gcc --version `

5. If no toolchain present install it by:
` > choco install gcc-arm-embedded `
or by installing binary from: [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

6. Install GNU make: 
` > choco install make `

7. Verify that all tools installed are added to `$PATH` and available in projects directory. 
    
    > arm-none-eabi-gcc --version
    > make --version
    > cygwin

### BUILD

Project is based on makefile system based on one root makefile [Makefile](https://github.com/PGRacing/PDM/blob/main/CODE/PDMS/Makefile)

To start build type: 
` >  make -j 16`
or start task:
` Build STM32 - L496` by clicking `Ctrl + Shift + B`

To clean buid type: 
` > make clean -j 16`
or start task:
` Clean STM32 - L496` by clicking `Ctrl + Shift + B`

Build results are present in `./build` directory as:
    - `PDMS.elf`
    - `PDMS.hex`
    - `PDMS.bin`
    - `PDMS.map`

### DEBUG

Debug is performed via SWD interface on ST-Link probe via GDB Debugger. Start cortex-debug session from `Run and Debug (Ctrl + Shift + D)` tab by selecting one of following options: 
    - `Debug STM32 (PDMS)`
    - `Attach STM32 (PDMS)`
    - `Debug STM32 with Live Expressions`
Last option gives you access to Cortex Live Watch. Refresh frequency can be adjusted in `.vscode/launch.json` file in `samplesPerSecond` parameter. 

###### SWV
When SWO pin in ST-Link probe is connected, additional logging (SWV - Serial Wire Viewer) functionality can be used. On debug startup new terminal tab will pop-up named "ITM port 0 output". All printf function output is redirected over there. To interface with this port, wrapper macros from `util\logger.h` should be used. SWV is MCU and probe clock dependent, if any of those are changed it needs to be adjusted in `.vscode/launch.json`