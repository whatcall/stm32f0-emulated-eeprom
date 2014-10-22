/**
  ******************************************************************************
  * @file    STM32F0xx_EEPROM_Emulation/src/eeprom.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    29-May-2012
  * @brief   This file provides all the EEPROM emulation firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/** @addtogroup STM32F0xx_EEPROM_Emulation
  * @{
  */ 

/* Includes ------------------------------------------------------------------*/
#include "eeprom.h"
#include "stdbool.h"
#include "stm32f0xx_conf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Global variable used to store variable value in read sequence */
ee_data_t DataVar = 0;

/* Virtual address defined by the user: 0xFFFF value is prohibited */
extern ee_data_t VirtAddVarTab[NB_OF_VAR];

/* multi-allocations definition */
#ifdef EE_MULT_ENABLE
extern ee_alloc_t EmulatedChips[EE_NUM];     
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static FLASH_Status EE_Format(uint16_t initial_page);
static uint16_t EE_VerifyPageFullWriteVariable(ee_data_t VirtAddress, ee_data_t Data);
static uint16_t EE_PageTransfer(ee_data_t VirtAddress, ee_data_t Data);
static uint16_t EE_FindValidPage(uint8_t Operation);


/**
  * @brief  Restore the pages to a known good state in case of page's status
  *   corruption after a power loss.
  * @param  None.
  * @retval - Flash error code: on write Flash error
  *         - FLASH_COMPLETE: on success
  */
ee_status_t EE_Init(void)
{
  uint16_t VarIdx = 0;
  uint16_t EepromStatus = 0, ReadStatus = 0;
  int16_t x = -1;
  uint16_t  FlashStatus;
 
  uint16_t page_idx;  
  ee_page_status_t page_status[PAGE_NUM];
  uint16_t current_page = NO_VALID_PAGE;
  uint16_t next_page = NO_VALID_PAGE; 
  bool is_pages_invalid = false;


  /* Set initial status value */
  for(page_idx = 0; page_idx < PAGE_NUM; page_idx++)
  {
    page_status[page_idx] = PAGE_UNKNOWN;
  }
  
  /* Read all pages' status */
  for(page_idx = 0; page_idx < PAGE_NUM; page_idx++)
  {
    page_status[page_idx] = (*(__IO uint16_t*)PAGE_BASE_ADDRESS(page_idx));
  }

  /* check the most possible valid page if existed, it is impossible more than 1 valid pages existed. */
  /* try to find a valid page */
  for(page_idx = 0; page_idx < PAGE_NUM; page_idx++)
  {
    if(page_status[page_idx] == VALID_PAGE)
    {
      if(current_page == NO_VALID_PAGE)
      {
        current_page = page_idx;
      }
      else
      {
        is_pages_invalid = true;                //more than 1 valid pages detected
        break;
      }
    }
  }
  
  /* if no valid page found, try to find a page with RECEIVE_DATA */
  if(!is_pages_invalid && current_page == NO_VALID_PAGE)
  {
    for(page_idx = 0; page_idx < PAGE_NUM; page_idx++)
    {
      if(page_status[page_idx] == RECEIVE_DATA)
      {
        if(current_page == NO_VALID_PAGE)
        {
          current_page = page_idx;
        }
        else
        {
          is_pages_invalid = true;                //more than 1 RECEIVE_DATA pages detected
          break;
        }
      }
    }
  }


  /* if page statuses are invalid (unexpected number of VALID_PAGE or RECEIVE_DATA detected) or 
     all pages are erased(or other unknown status value) */
  if(is_pages_invalid || current_page == NO_VALID_PAGE)
  {
    // select initial valid page
    if(is_pages_invalid)
    {
      current_page = page_idx;                    // last data operation position
    }
    else
    {
      current_page = PAGE0;                       // default:set PAGE0 as VALID_PAGE
    }
    
    // erase all pages and set current_page status to VALID_PAGE
    FlashStatus = EE_Format(current_page);                                
    if (FlashStatus != FLASH_COMPLETE)
    {
      return FlashStatus;
    }
  }
  else
  {
    uint16_t current_page_status = (uint16_t)page_status[current_page];
    next_page = (current_page + 1) % PAGE_NUM;
    
    switch(current_page_status){
      case ERASED:        
        // not possible, this situation should be processed as all pages erased before
        break;
      case RECEIVE_DATA:  
        /* means only 1 page indicates RECEIVE_DATA, all others are ERASED. */
        
        /* Mark current page as valid */
        FlashStatus = FLASH_ProgramHalfWord(PAGE_BASE_ADDRESS(current_page), VALID_PAGE);
        if (FlashStatus != FLASH_COMPLETE)
        {
          return FlashStatus;
        }
        
        /* Erase next page */
        FlashStatus = FLASH_ErasePage(PAGE_BASE_ADDRESS(next_page));
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
          return FlashStatus;
        }
        break;
        
      case VALID_PAGE:    
        /* means only 1 valid page found, it may has a next page with RECEIVE_DATA or ERASED status */
        
        if(page_status[next_page] == RECEIVE_DATA)
        {
          // use current page as VALID_PAGE, transfer the last updated vars from current page to 
          // next page, and then mark next page as VALID_PAGE and erase current page
          
          /* Transfer data from current valid page to next page */
          for (VarIdx = 0; VarIdx < NB_OF_VAR; VarIdx++)
          {
            if (( *(__IO uint16_t*)(PAGE_BASE_ADDRESS(next_page) + 6)) == VirtAddVarTab[VarIdx])
            {
              x = VarIdx;
            }
            if (VarIdx != x)
            {
              /* Read the last variables' updates */  
              ReadStatus = EE_ReadVariable(VirtAddVarTab[VarIdx], &DataVar);
              /* In case variable corresponding to the virtual address was found */
              if (ReadStatus != 0x1)
              {
                /* Transfer the variable to the next page */
                EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddVarTab[VarIdx], DataVar);
                if (EepromStatus != FLASH_COMPLETE)
                {
                  return EepromStatus;
                }
              }
            }
          }
          /* transfer finished */
          
          
          /* Mark before erase may leave 2 valid pages if power down happened here, so we change the order*/
          /* Erase current page */
          FlashStatus = FLASH_ErasePage(PAGE_BASE_ADDRESS(current_page));
          /* If erase operation was failed, a Flash error code is returned */
          if (FlashStatus != FLASH_COMPLETE)
          {
            return FlashStatus;
          }
          
          /* Mark next page as valid */
          FlashStatus = FLASH_ProgramHalfWord(PAGE_BASE_ADDRESS(next_page), VALID_PAGE);
          /* If program operation was failed, a Flash error code is returned */
          if (FlashStatus != FLASH_COMPLETE)
          {
            return FlashStatus;
          }
        }
        else if(page_status[next_page] == ERASED)
        {
          // erase next page and use current page as VALID_PAGE
          // FIXME: why erase an ERASED page? Can we read the memory first to verify if it has bee full erased before erasing?
          FlashStatus = FLASH_ErasePage(PAGE_BASE_ADDRESS(next_page));
          /* If erase operation was failed, a Flash error code is returned */
          if (FlashStatus != FLASH_COMPLETE)
          {
            return FlashStatus;
          }
        }
        break;
      default:
        break;
    }
  }
    
  return (ee_status_t) FLASH_COMPLETE;
}

/**
  * @brief  Returns the last stored variable data, if found, which correspond to
  *   the passed virtual address
  * @param  VirtAddress: Variable virtual address
  * @param  Data: Global variable contains the read variable value
  * @retval Success or error status:
  *           - 0: if variable was found
  *           - 1: if the variable was not found
  *           - NO_VALID_PAGE: if no valid page was found.
  */
ee_status_t EE_ReadVariable(ee_data_t VirtAddress, ee_data_t* Data)
{
  uint16_t ValidPage = NO_VALID_PAGE;
  uint16_t AddressValue = 0x5555;
  uint16_t ReadStatus = 1;
  uint32_t PageStartAddress = PAGE0_BASE_ADDRESS;
  uint32_t Address = PAGE0_END_ADDRESS - 1;
  
  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  PageStartAddress = PAGE_BASE_ADDRESS(ValidPage);
  /* Get the valid Page end Address */
  Address = PAGE_END_ADDRESS(ValidPage) - 1;
  
  /* Check each active page address starting from end */
  while (Address > (PageStartAddress + 2))
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(__IO uint16_t*)Address);

    /* Compare the read address with the virtual address */
    if (AddressValue == VirtAddress)
    {
      /* Get content of Address-2 which is variable value */
      *Data = (*(__IO uint16_t*)(Address - 2));

      /* In case variable value is read, reset ReadStatus flag */
      ReadStatus = 0;

      break;
    }
    else
    {
      /* Next address location */
      Address = Address - 4;
    }
  }

  /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
  return ReadStatus;
}

/**
  * @brief  Writes/upadtes variable data in EEPROM.
  * @param  VirtAddress: Variable virtual address
  * @param  Data: 16 bit data to be written
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
ee_status_t EE_WriteVariable(ee_data_t VirtAddress, ee_data_t Data)
{
  uint16_t Status = 0;

  /* Write the variable virtual address and value in the EEPROM */
  Status = EE_VerifyPageFullWriteVariable(VirtAddress, Data);

  /* In case the EEPROM active page is full */
  if (Status == PAGE_FULL)
  {
    /* Perform Page transfer */
    Status = EE_PageTransfer(VirtAddress, Data);
  }

  /* Return last operation status */
  return Status;
}

/**
  * @brief  Erases PAGE0 and PAGE1 and writes VALID_PAGE header to PAGE0
  * @param  None
  * @retval Status of the last operation (Flash write or erase) done during
  *         EEPROM formating
  */
static FLASH_Status EE_Format(uint16_t initial_page)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint16_t page_idx;
  
  //assert_param((IS_VALID_PAGE_INDEX(initial_page));
  
  for(page_idx = 0; page_idx < PAGE_NUM; page_idx++)
  {
    FlashStatus = FLASH_ErasePage(PAGE_BASE_ADDRESS(page_idx));
    if (FlashStatus != FLASH_COMPLETE)
    {
      return FlashStatus;
    }
    
    /* Set Page0 as valid page: Write VALID_PAGE at initial_page base address */
    if(page_idx == initial_page)
    {
      FlashStatus = FLASH_ProgramHalfWord(PAGE_BASE_ADDRESS(page_idx), VALID_PAGE);
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
  }
  
      
  return FlashStatus;
}

/**
  * @brief  Find valid Page for write or read operation
  * @param  Operation: operation to achieve on the valid page.
  *   This parameter can be one of the following values:
  *     @arg READ_FROM_VALID_PAGE: read operation from valid page
  *     @arg WRITE_IN_VALID_PAGE: write operation from valid page
  * @retval Valid page number (PAGE0 or PAGE1) or NO_VALID_PAGE in case
  *   of no valid page was found
  */
static uint16_t EE_FindValidPage(uint8_t Operation)
{
  ee_page_status_t page_status[PAGE_NUM];
  uint16_t page_idx;
  uint16_t valid_page;
  
  /* Read all pages' status */
  for(page_idx = 0; page_idx < PAGE_NUM; page_idx++)
  {
    page_status[page_idx] = (*(__IO uint16_t*)PAGE_BASE_ADDRESS(page_idx));
  }
  
  /* Scan for a valid page */
  switch (Operation){
    case WRITE_IN_VALID_PAGE:       // if only VALID, return valid, if a RECEIVE after VALID, return RECEIVE
      for(page_idx  = 0; page_idx < PAGE_NUM; page_idx++)
      {
        if(page_status[page_idx] == VALID_PAGE)
        {
          uint16_t next_page = PAGE_NEXT(page_idx);
          if(page_status[next_page] == RECEIVE_DATA)
          {
            return next_page;
          }
          else
          {
            return page_idx;
          }
        }
      }
      
      return NO_VALID_PAGE;   /* No valid Page */

    case READ_FROM_VALID_PAGE:
      for(page_idx  = 0; page_idx < PAGE_NUM; page_idx++)
      {
        if(page_status[page_idx] == VALID_PAGE)
        {
          return page_idx;
        }
      }
      
      return NO_VALID_PAGE;   /* No valid Page */
      
    default:
      return PAGE0;
      break;
  }
}


/**
  * @brief  Verify if active page is full and Writes variable in EEPROM.
  * @param  VirtAddress: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
static uint16_t EE_VerifyPageFullWriteVariable(ee_data_t VirtAddress, ee_data_t Data)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint16_t ValidPage = PAGE0;
  uint32_t Address = PAGE0_BASE_ADDRESS;        //0x08010000;     FIXME
  uint32_t PageEndAddress = PAGE0_END_ADDRESS;  // 0x080107FF;    // FIXME
   
  /* Get valid Page for write operation */
  ValidPage = EE_FindValidPage(WRITE_IN_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = PAGE_BASE_ADDRESS(ValidPage);
  /* Get the valid Page end Address */
  PageEndAddress = PAGE_END_ADDRESS(ValidPage) - 1;  //FIXME: need some check
  
  /* Check each active page address starting from begining */
  while (Address < PageEndAddress)
  {
    /* Verify if Address and Address+2 contents are 0xFFFFFFFF */
    if ((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
    {
      /* Set variable data */
      FlashStatus = FLASH_ProgramHalfWord(Address, Data);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Set variable virtual address */
      FlashStatus = FLASH_ProgramHalfWord(Address + 2, VirtAddress);
      /* Return program operation status */
      return FlashStatus;
    }
    else
    {
      /* Next address location */
      Address = Address + 4;
    }
  }

  /* Return PAGE_FULL in case the valid page is full */
  return PAGE_FULL;
}

/**
  * @brief  Transfers last updated variables data from the full Page to
  *   an empty one.
  * @param  VirtAddress: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
static uint16_t EE_PageTransfer(ee_data_t VirtAddress, ee_data_t Data)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint32_t NewPageAddress = EEPROM_START_ADDRESS;   // FIXME: Not proper
  uint32_t OldPageAddress = EEPROM_START_ADDRESS;   // FIXME: Not proper
  uint16_t ValidPage = PAGE0, VarIdx = 0;
  uint16_t EepromStatus = 0, ReadStatus = 0;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Set New and Old page address */
  if (ValidPage != NO_VALID_PAGE)
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE_BASE_ADDRESS(PAGE_NEXT(ValidPage)); 

    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE_BASE_ADDRESS(ValidPage);
  }
  else
  {
    return NO_VALID_PAGE;       /* No valid Page */
  }

  /* Set the new Page status to RECEIVE_DATA status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, RECEIVE_DATA);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Write the variable passed as parameter in the new active page */
  EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddress, Data);
  /* If program operation was failed, a Flash error code is returned */
  if (EepromStatus != FLASH_COMPLETE)
  {
    return EepromStatus;
  }

  /* Transfer process: transfer variables from old to the new active page */
  for (VarIdx = 0; VarIdx < NB_OF_VAR; VarIdx++)
  {
    if (VirtAddVarTab[VarIdx] != VirtAddress)  /* Check each variable except the one passed as parameter */
    {
      /* Read the other last variable updates */
      ReadStatus = EE_ReadVariable(VirtAddVarTab[VarIdx], &DataVar);
      /* In case variable corresponding to the virtual address was found */
      if (ReadStatus != 0x1)
      {
        /* Transfer the variable to the new active page */
        EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddVarTab[VarIdx], DataVar);
        /* If program operation was failed, a Flash error code is returned */
        if (EepromStatus != FLASH_COMPLETE)
        {
          return EepromStatus;
        }
      }
    }
  }

  /* Erase the old Page: Set old Page status to ERASED status */
  FlashStatus = FLASH_ErasePage(OldPageAddress);
  /* If erase operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Set new Page status to VALID_PAGE status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, VALID_PAGE);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Return last operation flash status */
  return FlashStatus;
}



/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
