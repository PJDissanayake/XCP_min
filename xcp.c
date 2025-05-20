/**
  ******************************************************************************
  XCP.c
  For STM32 microcontrollers
  Author:   Pathum J Dissanayake
  Created:  May 20, 2025
  ******************************************************************************
**/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>


/* XCP Command Packet Identifiers (PIDs) */
#define XCP_CONNECT            0xFF
#define XCP_DISCONNECT         0xFE
#define XCP_GET_STATUS         0xFD
#define XCP_SYNCH              0xFC
#define XCP_SET_MTA            0xF6
#define XCP_UPLOAD             0xF5
#define XCP_DOWNLOAD           0xF0

/* XCP Response Packet Identifiers */
#define XCP_PID_RES            0xFF  // Positive Response
#define XCP_PID_ERR            0xFE  // Error Response

/* XCP Error Codes (commonly used) */
#define XCP_ERR_CMD_UNKNOWN    0x20
#define XCP_ERR_CMD_SYNTAX     0x21
#define XCP_ERR_OUT_OF_RANGE   0x22

#define XCP_MAX_CTO 8  // Maximum CTO packet size
#define XCP_MAX_DTO 8  // Maximum DTO packet size



/* SPI handle (provided by STM32 HAL) */
extern SPI_HandleTypeDef hspi1;

/* XCP Buffers */
uint8_t rxBuffer[XCP_MAX_CTO];
uint8_t txBuffer[XCP_MAX_DTO];

/* Memory Transfer Address (MTA) */
uint32_t mta_address = 0;
uint8_t  mta_extension = 0;

/* xxxxx Validate memory address (for safety, adjust as per your MCU's memory map) xxxxx*/
#define FLASH_START 0x08000000
#define FLASH_SIZE  (128*1024)
#define RAM_START   0x20000000
#define RAM_SIZE    (64*1024)

bool ValidateAddress(uint32_t addr) {
    if ((addr >= FLASH_START && addr < (FLASH_START + FLASH_SIZE)) ||
        (addr >= RAM_START && addr < (RAM_START + RAM_SIZE)))
        return true;
    return false;
}

/* Command Handlers */
void XCP_Connect(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
    res[1] = 0x01; // Resource: Calibration supported
    res[2] = XCP_MAX_CTO;
    res[3] = 0x00; // MAX_DTO LSB
    res[4] = XCP_MAX_DTO; // MAX_DTO MSB
    res[5] = 0x01; // XCP v1.0
    res[6] = 0x01; // Transport v1.0
    res[7] = 0x00; // Reserved
}

void XCP_Disconnect(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
}

void XCP_GetStatus(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
    res[1] = 0x00; // Session status: connected
    res[2] = 0x00; // Protection status
    res[3] = 0x00; // Reserved
    res[4] = 0x00; // Reserved
    res[5] = 0x00; // Reserved
    res[6] = 0x00; // Reserved
    res[7] = 0x00; // Reserved
}

void XCP_Synch(uint8_t *cmd, uint8_t *res) {
    res[0] = XCP_PID_RES;
}

void XCP_SetMTA(uint8_t *cmd, uint8_t *res) {
    mta_extension = cmd[1];
    mta_address = (uint32_t)cmd[4] |
                  ((uint32_t)cmd[5] << 8) |
                  ((uint32_t)cmd[6] << 16) |
                  ((uint32_t)cmd[7] << 24);
    res[0] = XCP_PID_RES;
}

void XCP_Upload(uint8_t *cmd, uint8_t *res) {
    uint8_t length = cmd[1];
    if (!ValidateAddress(mta_address) || (length > (XCP_MAX_DTO - 1))) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }
    res[0] = XCP_PID_RES;
    memcpy(&res[1], (uint8_t*)mta_address, length);
    mta_address += length;
}

void XCP_Download(uint8_t *cmd, uint8_t *res) {
    uint8_t length = cmd[1];
    if (!ValidateAddress(mta_address) || (length > (XCP_MAX_CTO - 2))) {
        res[0] = XCP_PID_ERR;
        res[1] = XCP_ERR_OUT_OF_RANGE;
        return;
    }
    memcpy((uint8_t*)mta_address, &cmd[2], length);
    mta_address += length;
    res[0] = XCP_PID_RES;
}

/* Main Command Dispatcher */
void XCP_CommandHandler(uint8_t *cmd, uint8_t *res) {
    uint8_t pid = cmd[0];
    memset(res, 0, XCP_MAX_DTO); // Clear response buffer

    switch (pid) {
        case XCP_CONNECT:
            XCP_Connect(cmd, res);
            break;
        case XCP_DISCONNECT:
            XCP_Disconnect(cmd, res);
            break;
        case XCP_GET_STATUS:
            XCP_GetStatus(cmd, res);
            break;
        case XCP_SYNCH:
            XCP_Synch(cmd, res);
            break;
        case XCP_SET_MTA:
            XCP_SetMTA(cmd, res);
            break;
        case XCP_UPLOAD:
            XCP_Upload(cmd, res);
            break;
        case XCP_DOWNLOAD:
            XCP_Download(cmd, res);
            break;
        default:
            res[0] = XCP_PID_ERR;
            res[1] = XCP_ERR_CMD_UNKNOWN;
            break;
    }
}

/* SPI Transaction Handler */
void XCP_Task(void) {
    // Wait for SPI transaction initiated by master
    if (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY) {
        HAL_SPI_TransmitReceive_IT(&hspi1, txBuffer, rxBuffer, XCP_MAX_CTO);
    }
}

/* SPI Callback */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &hspi1) {
        XCP_CommandHandler(rxBuffer, txBuffer);
        // Next SPI transaction will be started by master
    }
}


