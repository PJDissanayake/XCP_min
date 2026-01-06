/**
******************************************************************************
XCP.c
For STM32 microcontrollers
Author: Pathum J Dissanayake
Created: May 20, 2025
******************************************************************************/

#include <string.h>
#include <stdbool.h>
#include "spi.h"
#include "xcp.h"
#include "stm32f4xx_hal.h"

extern uint8_t rxBuffer[];
extern uint8_t txBuffer[];

volatile uint32_t mta_address = 0;
volatile uint8_t mta_extension = 0;
volatile bool xcp_connected = false;

/*
bool ValidateAddress(uint32_t addr) {
    if ((addr >= RAM_START && addr < (RAM_START + RAM_SIZE))) {
        return true;
    }
    // Only allow application flash (protect bootloader region)
    if ((addr >= APP_FLASH_START && addr < FLASH_END)) {
        return true;
    }
    return false;
}
*/

bool ValidateAddress(uint32_t addr) {
    /* Check, address is within valid memory regions */
    if ((addr >= FLASH_START && addr < (FLASH_START + FLASH_SIZE)) ||
        (addr >= RAM_START && addr < (RAM_START + RAM_SIZE))) {
        return true;
    }
    return false;
}


void XCP_Connect(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    res[0] = XCP_PID_RES;
    res[1] = 0x10 | 0x20;        // PGM + CAL/PAG
    res[2] = XCP_MAX_CTO;
    res[3] = 0x00;
    res[4] = XCP_MAX_DTO;
    res[5] = XCP_PROTOCOL_VERSION;
    res[6] = XCP_TRANSPORT_VERSION;

    xcp_connected = true;
}

void XCP_Disconnect(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    res[0] = XCP_PID_RES;

    xcp_connected = false;
}

void XCP_GetStatus(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    res[0] = XCP_PID_RES;
}

void XCP_Synch(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    res[0] = XCP_PID_ERR;

}

void XCP_SetMTA(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    uint32_t addr = ((uint32_t)cmd[7] << 24) |
                    ((uint32_t)cmd[6] << 16) |
                    ((uint32_t)cmd[5] << 8)  |
                    (uint32_t)cmd[4];

    if (!ValidateAddress(addr)) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }

    mta_address = addr;
    res[0] = XCP_PID_RES;
}

void XCP_Upload(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    uint8_t length = cmd[1];

    if (!ValidateAddress(mta_address) || (length > (XCP_MAX_DTO - 1))) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }
    res[0] = XCP_PID_RES;
    __disable_irq();
    memcpy(&res[1], (uint8_t*)mta_address, length);
    __enable_irq();
    mta_address += length;
}

void XCP_Download(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    uint8_t length = cmd[1];

    if (!ValidateAddress(mta_address) || (length > (XCP_MAX_CTO - 2))) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }

    __disable_irq();
    memcpy((uint8_t*)mta_address, &cmd[2], length);
    __enable_irq();
    mta_address += length;
    res[0] = XCP_PID_RES;
}

void XCP_ProgramStart(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    if (HAL_FLASH_Unlock() != HAL_OK) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_CMD_BUSY;
        return;
    }
    res[0] = XCP_PID_RES;
}

void XCP_ProgramClear(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    uint32_t length = ((uint32_t)cmd[7] << 24) |
                      ((uint32_t)cmd[6] << 16) |
                      ((uint32_t)cmd[5] << 8)  |
                      (uint32_t)cmd[4];

    if (!ValidateAddress(mta_address) || length == 0 ||
        !ValidateAddress(mta_address + length - 1)) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }

    // Simple sector calculation for STM32F407
    uint32_t start_addr = mta_address;
    uint32_t end_addr   = mta_address + length - 1;

    uint32_t start_sector, end_sector;
    if (start_addr < 0x08004000U) start_sector = FLASH_SECTOR_0;
    else if (start_addr < 0x08008000U) start_sector = FLASH_SECTOR_1;
    else if (start_addr < 0x0800C000U) start_sector = FLASH_SECTOR_2;
    else if (start_addr < 0x08010000U) start_sector = FLASH_SECTOR_3;
    else if (start_addr < 0x08020000U) start_sector = FLASH_SECTOR_4;
    else start_sector = FLASH_SECTOR_5;  // and higher...

    if (end_addr < 0x08004000U) end_sector = FLASH_SECTOR_0;
    else if (end_addr < 0x08008000U) end_sector = FLASH_SECTOR_1;
    else if (end_addr < 0x0800C000U) end_sector = FLASH_SECTOR_2;
    else if (end_addr < 0x08010000U) end_sector = FLASH_SECTOR_3;
    else if (end_addr < 0x08020000U) end_sector = FLASH_SECTOR_4;
    else end_sector = FLASH_SECTOR_11;  // safe max

    uint32_t nb_sectors = end_sector - start_sector + 1;

    FLASH_EraseInitTypeDef eraseInit = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Banks        = FLASH_BANK_1,
        .Sector       = start_sector,
        .NbSectors    = nb_sectors,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3
    };

    uint32_t sector_error;

    __disable_irq();
    if (HAL_FLASHEx_Erase(&eraseInit, &sector_error) == HAL_OK) {
        res[0] = XCP_PID_RES;
    } else {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_CMD_BUSY;
    }
    __enable_irq();
}

void XCP_Program(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    uint8_t length = cmd[1];

    if (!ValidateAddress(mta_address) || length > (XCP_MAX_CTO - 2) || length == 0) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }

    __disable_irq();
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t i;
    for (i = 0; i < length; i += 4) {
        uint32_t data = 0xFFFFFFFF;  // default erased
        uint8_t bytes = (length - i >= 4) ? 4 : (length - i);
        memcpy(&data, &cmd[2 + i], bytes);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, mta_address + i, data) != HAL_OK) {
            status = HAL_ERROR;
            break;
        }
    }

    if (status == HAL_OK) {
        mta_address += length;
        res[0] = XCP_PID_RES;
    } else {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_CMD_BUSY;
    }
    __enable_irq();
}

void XCP_ProgramReset(uint8_t *cmd, uint8_t *res) {
    memset(res, 0, XCP_MAX_DTO);
    HAL_FLASH_Lock();
    res[0] = XCP_PID_RES;
    // Small delay to send response
    for (volatile uint32_t i = 0; i < 10000; i++) __NOP();

    NVIC_SystemReset();
}

void XCP_CommandHandler(uint8_t *cmd, uint8_t *res) {
    uint8_t pid = cmd[0];
    if (pid == 0xAA) {
        return;
    }

    switch (pid) {
        case XCP_CONNECT:        XCP_Connect(cmd, res);        break;
        case XCP_DISCONNECT:     XCP_Disconnect(cmd, res);     break;
        case XCP_GET_STATUS:     XCP_GetStatus(cmd, res);      break;
        case XCP_SYNCH:          XCP_Synch(cmd, res);          break;
        case XCP_SET_MTA:        XCP_SetMTA(cmd, res);         break;
        case XCP_UPLOAD:         XCP_Upload(cmd, res);         break;
        case XCP_DOWNLOAD:       XCP_Download(cmd, res);       break;
        case XCP_PROGRAM_START:  XCP_ProgramStart(cmd, res);   break;
        case XCP_PROGRAM_CLEAR:  XCP_ProgramClear(cmd, res);   break;
        case XCP_PROGRAM:        XCP_Program(cmd, res);        break;
        case XCP_PROGRAM_RESET:  XCP_ProgramReset(cmd, res);   break;
        default:
            res[0] = XCP_PID_ERR;
            res[1] = XCP_ERR_CMD_UNKNOWN;
            break;
    }
}

