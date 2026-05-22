#ifndef __BSP_FLASH_H_
#define __BSP_FLASH_H_

#include "typedefs.h"

// ** MEMORY MAP L496 (1024kB)**
// Functions declared in this header should be used to operate on the user region only.
//+----------------------------+  0x08100000  (End of Flash)
//|                            |
//| 64kB - CONFIGB2            |
//|                            |
//+----------------------------+
//|                            |
//| 64kB - CONFIGB1            |
//|                            |
//+----------------------------+
//|                            |
//| 896kB - Main flash         |
//|  (User Code / Data)        |
//|                            |
//+----------------------------+  0x08000000  (Start of Flash)

#define FLASH_BASE_ADDR               ((uint32_t)0x08000000UL)
#define FLASH_MAIN_SIZE               ((uint32_t)0x000E0000UL)
#define FLASH_CONFIGB1_SIZE           ((uint32_t)0x00010000UL)
#define FLASH_CONFIGB2_SIZE           ((uint32_t)0x00010000UL)

#define FLASH_MAIN_START_ADDR         (FLASH_BASE_ADDR)
#define FLASH_MAIN_END_ADDR           (FLASH_BASE_ADDR + FLASH_MAIN_SIZE - 1U)
#define FLASH_CONFIGB1_START_ADDR     (FLASH_BASE_ADDR + FLASH_MAIN_SIZE)
#define FLASH_CONFIGB1_END_ADDR       (FLASH_CONFIGB1_START_ADDR + FLASH_CONFIGB1_SIZE - 1U)
#define FLASH_CONFIGB2_START_ADDR     (FLASH_CONFIGB1_END_ADDR + 1U)
#define FLASH_CONFIGB2_END_ADDR       (FLASH_CONFIGB2_START_ADDR + FLASH_CONFIGB2_SIZE - 1U)

// STM32L496 flash is erased in 2 kB pages.
#define FLASH_SECTOR_SIZE           ((uint32_t)0x00000800U)
#define FLASH_USER_START_ADDR         (FLASH_CONFIGB1_START_ADDR)   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR           (FLASH_CONFIGB2_END_ADDR)     /* End @ of user Flash area */
#define FLASH_USER_SIZE               (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR + 1U)
#define FLASH_USER_NUMBER_OF_SECTORS  (FLASH_USER_SIZE / FLASH_SECTOR_SIZE)


int32_t BSP_FLASH_TestProc(void);
int32_t BSP_FLASH_Read(uint32_t sector, uint32_t offset, uint8_t* buffer, size_t size);
int32_t BSP_FLASH_Prog(uint32_t sector, uint32_t offset, uint8_t* buffer, size_t size);
int32_t BSP_FLASH_Erease(uint32_t sector);
int32_t BSP_FLASH_Sync(void);
uint32_t BSP_FLASH_GetSector(uint32_t addr);

#endif // __BSP_FLASH_H_