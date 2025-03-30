# PDM_NOTES

Idea of on init loading and control access

UV detection

1. NVM_Load()
2. Load to struct
3. Make use of configuration on all levels
4. PLAT Init
5. Start tasks

---------------------------------
TASKS:
(NAT) IRQ --> Phy fast loop
PDM LOGIC --> Logic loop
    --> Get INPUTS
    --> Set Outputs

CAN BUS QUEUE HANDLER

TELEMETRY DATA

PDM SLOW LOOP

///// Inrush OC settings

// Parameters
enableInrush  - boolean if inrush function is used
Tallinr - allowed time for inrush to start after channel turnon
Tinr - allowed time of inrush current
In - allowed nominal current
Iinr - allowed max inrush current
Ith - allowed current threshold to not turn of channel

1. Channel set on
2. Check if inrush function is enabled (enableInrush)
    2.1. Set Ith = Iinr
    2.2. During Tallinr wait till I >= In - wait for first current peak during allowed time (Tallinr)
    2.3. Start timer counting down from Tinr to 0
    2.4  During this time check if I >= Iinr, if yes disable channel ASAP if no do nothing
3. Set Ith = In
4. If I >= Ith disable channel asap, otherwise allow normal work


// Sample PWM during high state
// Remeber that both voltage and current must be intact to make sure that all readings are correct
// Adding config and regs to pdm file
// Configurations

OUT
- outsCfg
- outsReg

LOGIC 
- logicCfg
- logicReg

TELEMETRY
- telemetryCfg

BSP
- no configuration will be used

CAN HANDLER
- cansCfg


VMUX????

Trzeba zweryfikować czy inputy działają
Dodać filtr antyalisingowy na dobre wartości