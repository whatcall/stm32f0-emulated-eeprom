/**
  ******************************************************************************
  * @file    STM32F0xx_EEPROM_Emulation/inc/eeprom.h 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    29-May-2012
  * @brief   This file contains all the functions prototypes for the EEPROM 
  *          emulation firmware library.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EEPROM_H
#define __EEPROM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "eeprom_conf.h"


/* Type definitions ---------------------------------------------------------*/

/* Data and virtual address bits */
#if (EE_DATA_16BIT == EE_DATA_WIDTH)
  typedef uint16_t  ee_data_t;     
#elif (EE_DATA_32BIT == EE_DATA_WIDTH)
  typedef uint32_t  ee_data_t; 
#else
  #error("Unsupported emulation data width!")
#endif

/* EEPROM Emulation operation status */
typedef enum{
  EE_SUCCESS = FLASH_COMPLETE
}ee_status_t;       

/* Exported constants --------------------------------------------------------*/

/* assume the EEPROM memory allocation is consecutive */
#define PAGE_BASE_ADDRESS(pg) ((uint32_t)(EEPROM_START_ADDRESS + (pg * PAGE_SIZE)))
#define PAGE_END_ADDRESS(pg)  ((uint32_t)(EEPROM_START_ADDRESS + ((pg + 1) * PAGE_SIZE - 1)))

/* Number of pages supported */
#define PAGE_NUM_MIN          2
#define PAGE_NUM_MAX          6

#ifndef PAGE_NUM
  #define PAGE_NUM            3
#endif

#if (PAGE_NUM < PAGE_NUM_MIN) || (PAGE_NUM > PAGE_NUM_MAX)
  #error ("Invalid Page Number configuration!")  
#endif

/* Pages 0 and 1 base and end addresses and used Flash pages for EEPROM emulation */
#define PAGE0_BASE_ADDRESS    PAGE_BASE_ADDRESS(0) //((uint32_t)(EEPROM_START_ADDRESS + 0x0000))
#define PAGE0_END_ADDRESS     PAGE_END_ADDRESS(0)  //((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))

#define PAGE0                 ((uint16_t)0x0000)

#define PAGE1_BASE_ADDRESS    PAGE_BASE_ADDRESS(1) //((uint32_t)(EEPROM_START_ADDRESS + 0x0400))
#define PAGE1_END_ADDRESS     PAGE_END_ADDRESS(1)  //((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))


#define PAGE1                 ((uint16_t)0x0001)

#if (PAGE_NUM > 2)
#define PAGE2_BASE_ADDRESS    PAGE_BASE_ADDRESS(2) 
#define PAGE2_END_ADDRESS     PAGE_END_ADDRESS(2)  
#define PAGE2                 ((uint16_t)0x0002)
#endif 

#if (PAGE_NUM > 3)
#define PAGE3_BASE_ADDRESS    PAGE_BASE_ADDRESS(3) 
#define PAGE3_END_ADDRESS     PAGE_END_ADDRESS(3)  
#define PAGE3                 ((uint16_t)0x0003)
#endif 

#if (PAGE_NUM > 4)
#define PAGE4_BASE_ADDRESS    PAGE_BASE_ADDRESS(4) 
#define PAGE4_END_ADDRESS     PAGE_END_ADDRESS(4)  
#define PAGE4                 ((uint16_t)0x0004)
#endif 

#if (PAGE_NUM > 5)
#define PAGE5_BASE_ADDRESS    PAGE_BASE_ADDRESS(5) 
#define PAGE5_END_ADDRESS     PAGE_END_ADDRESS(5)  
#define PAGE5                 ((uint16_t)0x0005)
#endif 


/* No valid page define */
#define NO_VALID_PAGE         ((uint16_t)0x00AB)

/* Get next page */
#define PAGE_NEXT(pg)         (((pg) + 1) % PAGE_NUM)

/* Page status definitions */
typedef enum{
  ERASED         =       ((uint16_t)0xFFFF),     /* Page is empty */
  RECEIVE_DATA   =       ((uint16_t)0xEEEE),     /* Page is marked to receive data */
  VALID_PAGE     =       ((uint16_t)0x0000),     /* Page containing valid data */
  PAGE_UNKNOWN   =       ((uint16_t)0x0006)      /* Page with unknown status */
}ee_page_status_t;

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE  ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE   ((uint8_t)0x01)

/* Page full define */
#define PAGE_FULL             ((uint8_t)0x80)

/* Check whether a page index is valid */
#define IS_VALID_PAGE_INDEX(page)   ((page) < PAGE_NUM)

/* Exported types ------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
ee_status_t EE_Init(void);
ee_status_t EE_ReadVariable(ee_data_t VirtAddress, ee_data_t* Data);
ee_status_t EE_WriteVariable(ee_data_t VirtAddress, ee_data_t Data);

#endif /* __EEPROM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
