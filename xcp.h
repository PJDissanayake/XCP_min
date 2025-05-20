/**
  ******************************************************************************
  XCP.h
  Author:   Pathum 
  Created:  May 20, 2025
  ******************************************************************************
**/

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