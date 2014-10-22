/**
  ******************************************************************************
  * @file    STM32F0xx_EEPROM_Emulation/inc/eeprom_conf.h 
  * @author  $Author: jing.tian $
  * @version $Revision: 65 $
  * @date    $Date: 2014-09-26 15:55:17 +0800 (周五, 26 九月 2014) $
  * @brief   EEPROM Emulation configuration header file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 Playine Information Technology</center></h2>
  *
  ******************************************************************************
  */ 
#ifndef __EEPROM_CONF_H
#define __EEPROM_CONF_H

#include "stdint.h"

/* Define if we need multi-EEPROM support */
#define EE_MULT_ENABLE

/* Number of EEPROMs will be used */
#define EE_NUM                2

/* Number of pages will be used */
#define PAGE_NUM              6

/* Define the size of the sectors to be used */
#define PAGE_SIZE             ((uint32_t)0x0400)     /* Page size = 1KByte */

/* EEPROM start address in Flash */
#define EEPROM_START_ADDRESS  ((uint32_t)0x08002800) /* EEPROM emulation start address:
                                                        from sector2, after 10KByte of used 
                                                        Flash memory */

/* Variables' number */
#define NB_OF_VAR             ((uint8_t)22)


/* Emulated data and virtual address bits */
#define EE_DATA_16BIT         16
#define EE_DATA_32BIT         32
#define EE_DATA_WIDTH         EE_DATA_16BIT


/* EEPROM allocation type definition */
typedef struct{
  uint32_t      start_addr;         // EEPROM emulation start address
  uint16_t      page_num;           // number of pages allocated for this EEPROM
  uint16_t      var_num;            // number of variables support
  uint16_t*     var_addr_tab;       // point to variable virtual address table
}ee_alloc_t;

#endif
