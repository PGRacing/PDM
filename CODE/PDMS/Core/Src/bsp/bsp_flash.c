#include "bsp_flash.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_flash.h"
#include "FreeRTOS.h"
#include <string.h>

static FLASH_EraseInitTypeDef eraseInitStruct;

// @brief  Gets the sector of a given address
// @param  addr: Address of the FLASH Memory
// @retval The sector of a given address
uint32_t BSP_FLASH_GetSector(uint32_t addr)
{
  uint32_t sector = 0;

  if((addr >= FLASH_BASE) && (addr < FLASH_BASE + FLASH_BANK_SIZE))
  {
    sector = (addr & ~FLASH_BASE) / FLASH_SECTOR_SIZE;
  }
  else if ((addr >= FLASH_BASE + FLASH_BANK_SIZE) && (addr < FLASH_BASE + FLASH_SIZE))
  {
    sector = ((addr & ~FLASH_BASE) - FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
  }
  else
  {
    sector = 0xFFFFFFFF; // Address out of range
  }

  return sector;
}


// @brief  Gets the bank of a given address
// @param  addr: Address of the FLASH Memory
// @retval The bank of a given address
static uint32_t BSP_FLASH_GetBank(uint32_t addr)
{
  uint32_t bank = 0;

  /* STM32L496 does not expose OPTSR_CUR; assume no bank swap and
     determine bank by address range. */
  if (addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    bank = FLASH_BANK_1;
  }
  else
  {
    bank = FLASH_BANK_2;
  }

  return bank;
}


// @brief  Check program operation.
// param StartAddress Area start address
// param EndAddress Area end address
// param Data Expected data
// @retval FailCounter
static uint32_t BSP_FLASH_CheckProgram(uint32_t startAddress, uint32_t endAddress, const uint32_t *data)
{
  uint32_t Address;
  uint32_t index, FailCounter = 0;
  uint32_t data32;

  // check the user Flash area word by word
  Address = startAddress;

  while(Address < endAddress)
  {
    for(index = 0; index<4; index++)
    {
      data32 = *(uint32_t*)Address;
      if(data32 != data[index])
      {
        FailCounter++;
        return FailCounter;
      }
      Address +=4;
    }
  }
  return FailCounter;
}


// @brief  Readout data form user available section
// @param  sector: Selected sector from user available section (relative to user area - not absolute sector number in memory)
// @param  offset: Readout offset in sector in bytes
// @param  buffer: Destination buffer
// @param  size:   Readout data size (bytes)
// @retval 0 if OK, -1 if readout out of range
int32_t BSP_FLASH_Read(uint32_t sector, uint32_t offset, uint8_t* buffer, size_t size)
{
    uint32_t addr = FLASH_USER_START_ADDR + (sector * FLASH_SECTOR_SIZE) + offset;
    if((addr + size) > FLASH_USER_END_ADDR )
    {
        return -1;
    }
    memcpy(buffer, (const void*) addr, size);
    return 0;
}

// @brief  Program flash data in user available section
// @param  sector: Selected sector from user available section (relative to user area - not absolute sector number in memory)
// @param  offset: Readout offset in sector in bytes
// @param  buffer: Source buffer
// @param  size:   Write data size (bytes)
// @retval 0 if OK, -1 if readout out of range, -2 if flash program error
int32_t BSP_FLASH_Prog(uint32_t sector, uint32_t offset, uint8_t* buffer, size_t size)
{
    vPortEnterCritical();
    HAL_FLASH_Unlock();
    
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                       FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR);

    uint32_t addr = FLASH_USER_START_ADDR + (sector * FLASH_SECTOR_SIZE) + offset;

    if((addr + size) > FLASH_USER_END_ADDR )
    {
        HAL_FLASH_Lock();
        vPortExitCritical();
        return -1;
    }

    for (size_t i = 0; i < size; i += 8)
    {
      uint64_t dword = 0;
      size_t chunk = (size - i >= sizeof(dword)) ? sizeof(dword) : (size - i);

      memcpy(&dword, &buffer[i], chunk);

      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, dword) != HAL_OK)
      {
        HAL_FLASH_Lock();
        vPortExitCritical();
        return -2;
      }
    }

    HAL_FLASH_Lock();
    vPortExitCritical();
    return 0;
}

// @brief  Gets the bank of a given address
// @param  sector: Selected sector from user available section (relative to user area - not absolute sector number in memory)
// @retval 0 if OK, -1 if erase error
int32_t BSP_FLASH_Erease(uint32_t sector)
{
    vPortEnterCritical();
    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                       FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR);
    // Get the 1st sector to erase
    uint32_t dst =  BSP_FLASH_GetSector(FLASH_USER_START_ADDR + (sector * FLASH_SECTOR_SIZE));

    // Get the bank
    uint32_t bankNo = BSP_FLASH_GetBank(FLASH_USER_START_ADDR + (sector * FLASH_SECTOR_SIZE));

    // Fill EraseInit structure
    eraseInitStruct.TypeErase     = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.Banks         = bankNo;
    eraseInitStruct.Page          = dst;
    eraseInitStruct.NbPages       = 1;

    uint32_t sectorError = 0;
    if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        vPortExitCritical();
        return -1;
    }

    HAL_FLASH_Lock();
    vPortExitCritical();
    return 0;
}



// @brief  Sync function, assure that eveything is flashed to memory
// @retval 0 if OK, -1 if sync error
int32_t BSP_FLASH_Sync(void)
{
    return 0;
}

// This function flashes and verifies whole user flash area
int32_t BSP_FLASH_TestProc(void)
{
      // Disable instruction cache prior to internal cacheable memory update
      // if (HAL_ICACHE_Disable() != HAL_OK)
      // {
      //   Error_Handler();
      // }

      // Unlock the Flash to enable the flash control register access
      HAL_FLASH_Unlock();

      // Erase the user Flash area
      // (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR)

      // Get the 1st sector to erase

      const uint32_t doubleWord[2] = {0x12345678, 0x87654321};

      uint32_t firstSector = BSP_FLASH_GetSector(FLASH_USER_START_ADDR);

      // Get the number of sector to erase from 1st sector
      uint32_t nbOfSectors = BSP_FLASH_GetSector(FLASH_USER_END_ADDR) - firstSector + 1;

      // Get the bank
      uint32_t bankNumber = BSP_FLASH_GetBank(FLASH_USER_START_ADDR);

      // Fill EraseInit structure
      eraseInitStruct.TypeErase     = FLASH_TYPEERASE_PAGES;
      eraseInitStruct.Banks         = bankNumber;
      eraseInitStruct.Page          = firstSector;
      eraseInitStruct.NbPages       = nbOfSectors;

      uint32_t sectorError = 0;

      if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != HAL_OK)
      {
        // Error occurred while sector erase.
        // User can add here some code to deal with this error.
        // SectorError will contain the faulty sector and then to know the code error on this sector,
        // user can call function 'HAL_FLASH_GetError()'
        // Infinite loop
        while (1)
        {
          __NOP();
        }
      }

      // Program the user Flash area word by word
      // (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR)

      uint32_t Address = FLASH_USER_START_ADDR;

      while (Address < FLASH_USER_END_ADDR)
      {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, ((uint32_t)doubleWord)) == HAL_OK)
        {
          Address = Address + 8; // increment to next double word
        }
       else
        {
          // Error occurred while writing data in Flash memory.
          // User can add here some code to deal with this error
          while (1)
          {
            __NOP();
          }
        }
      }

      // Lock the Flash to disable the flash control register access (recommended
      // to protect the FLASH memory against possible unwanted operation)
      HAL_FLASH_Lock();

      // Re-enable instruction cache
      // if (HAL_ICACHE_Enable() != HAL_OK)
      // {
      //   Error_Handler();
      // }

      // Check if the programmed data is OK
      // MemoryProgramStatus = 0: data programmed correctly
      // MemoryProgramStatus != 0: number of words not programmed correctly
      return BSP_FLASH_CheckProgram(FLASH_USER_START_ADDR, FLASH_USER_END_ADDR, doubleWord);
}
