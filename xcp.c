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


extern uint8_t rxBuffer[];
extern uint8_t txBuffer[];


bool ValidateAddress(uint32_t addr) {
    /* Check, address is within valid memory regions */
    if ((addr >= FLASH_START && addr < (FLASH_START + FLASH_SIZE)) ||
        (addr >= RAM_START && addr < (RAM_START + RAM_SIZE))) {
        return true;
    }
    return false;
}

/* Command Handlers */
void XCP_Connect(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
    res[1] = 0x01;  // Resource: Calibration
    res[2] = XCP_MAX_CTO;
    res[3] = 0x00;
    res[4] = XCP_MAX_DTO;
    res[5] = 0x01;  // XCP v1.0
    res[6] = 0x01;  // Transport v1.0
    res[7] = 0x00;
}

void XCP_Disconnect(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
}

void XCP_GetStatus(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
    memset(&res[1], 0, 7);
}

void XCP_Synch(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
}

void XCP_SetMTA(uint8_t *cmd, uint8_t *res) {
    /* Use only 32-bit addressing */
    mta_address = (uint32_t)cmd[4] |
                 ((uint32_t)cmd[5] << 8) |
                 ((uint32_t)cmd[6] << 16) |
                 ((uint32_t)cmd[7] << 24);
    res[0] = XCP_PID_RES;
}

void XCP_Upload(uint8_t *cmd, uint8_t *res) {
    uint8_t length = cmd[1];

    if(!ValidateAddress(mta_address) || (length > (XCP_MAX_DTO - 1))) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }
    __disable_irq();
    res[0] = XCP_PID_RES;
    memcpy(&res[1], (uint8_t*)mta_address, length);
    mta_address += length;
    __enable_irq();
}

void XCP_Download(uint8_t *cmd, uint8_t *res) {
    uint8_t length = cmd[1];

    if(!ValidateAddress(mta_address) || (length > (XCP_MAX_CTO - 2))) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }

    __disable_irq();
    memcpy((uint8_t*)mta_address, &cmd[2], length);
    mta_address += length;
    res[0] = XCP_PID_RES;
    __enable_irq();
}

void XCP_CommandHandler(uint8_t *cmd, uint8_t *res) {
    uint8_t pid = cmd[0];
    memset(res, 0, XCP_MAX_DTO);

    switch(pid) {
        case XCP_CONNECT:    XCP_Connect(cmd, res);    break;
        case XCP_DISCONNECT: XCP_Disconnect(cmd, res); break;
        case XCP_GET_STATUS: XCP_GetStatus(cmd, res);  break;
        case XCP_SYNCH:      XCP_Synch(cmd, res);      break;
        case XCP_SET_MTA:    XCP_SetMTA(cmd, res);     break;
        case XCP_UPLOAD:     XCP_Upload(cmd, res);     break;
        case XCP_DOWNLOAD:   XCP_Download(cmd, res);   break;
        default:
            res[0] = XCP_PID_ERR;
            res[1] = XCP_ERR_CMD_UNKNOWN;
            break;
    }
}

void XCP_Task(void) {
    if(!spi_busy && (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY)) {
        spi_busy = true;
        HAL_SPI_TransmitReceive_IT(&hspi1, txBuffer, rxBuffer, XCP_MAX_CTO);
    }
}


