# XCP Embedded Memory Access Library

## Overview

This library provides a lightweight implementation of the XCP (Universal Measurement and Calibration Protocol) for embedded systems, specifically designed for STM32 microcontrollers. It supports basic memory access operations over SPI, including connect, disconnect, status queries, memory transfer address (MTA) setting, and data upload/download. The library is tailored for applications requiring calibration and data acquisition in embedded environments.

## Features

- **SPI-based Communication**: Implements XCP as an SPI slave for reliable data transfer.
- **Supported Commands**:
  - CONNECT (0xFF)
  - DISCONNECT (0xFE)
  - GET_STATUS (0xFD)
  - SYNCH (0xFC)
  - SET_MTA (0xF6)
  - UPLOAD (0xF5)
  - DOWNLOAD (0xF0)
- **Memory Validation**: Ensures safe access to STM32F407xx Flash (1MB) and RAM (128KB).
- **DMA Support**: Utilizes DMA for efficient SPI data transfers.
- **Error Handling**: Includes error codes for unknown commands, syntax errors, and out-of-range addresses.

## Prerequisites

- **Hardware**: STM32F407xx microcontroller (or compatible).
- **Software**:
  - STM32Cube HAL library for SPI and DMA operations.
  - A compatible IDE (e.g., STM32CubeIDE).
- **Dependencies**: Ensure `main.h` and STM32 HAL drivers are included in your project.

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/<your-username>/<your-repo-name>.git
   ```
2. Copy the `spi.c`, `spi.h`, `xcp.c`, and `xcp.h` files into your STM32 project.
3. Include the necessary HAL libraries in your project configuration.
4. Configure your STM32 project to use SPI1 and DMA for SPI communication.

## Usage

1. **Initialize SPI**: Call `SPI_Init()` to configure the SPI peripheral as a slave with 8-bit data size, low polarity, and first-edge phase.
2. **Start SPI Communication**: Call `SPI_Start()` to initialize buffers and begin DMA-based SPI transmit/receive operations.
3. **Handle XCP Commands**: The `XCP_CommandHandler` function processes incoming XCP commands and generates appropriate responses. It is triggered in the `HAL_SPI_TxRxCpltCallback` after each SPI transaction.
4. **Memory Access**:
   - Use `XCP_SetMTA` to set the memory transfer address.
   - Use `XCP_Upload` to read data from the specified address.
   - Use `XCP_Download` to write data to the specified address.
5. **Error Handling**: The library validates memory addresses and returns error codes (`XCP_ERR_OUT_OF_RANGE`, `XCP_ERR_CMD_UNKNOWN`, etc.) for invalid operations.

## Example

```c
#include "spi.h"
#include "xcp.h"

int main(void) {
    // Initialize system and peripherals
    HAL_Init();
    SystemClock_Config(); // Configure your system clock
    SPI_Init();           // Initialize SPI
    SPI_Start();          // Start SPI communication

    while (1) {
        // Main loop (SPI handled via DMA interrupts)
    }
}
```

## Callback Integration

The library uses the HAL SPI callback to process XCP commands:

```c
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &hspi1) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15); // Optional: Toggle LED for debugging
        XCP_CommandHandler(rxBuffer, txBuffer);
        memset(rxBuffer, 0, SPI_BUFFER_SIZE);
        HAL_SPI_TransmitReceive_DMA(&hspi1, txBuffer, rxBuffer, SPI_BUFFER_SIZE);
    }
}
```

## Limitations

- Supports only a subset of XCP commands for basic memory access.
- Configured specifically for STM32F407xx memory regions (Flash: 0x08000000–0x080FFFFF, RAM: 0x20000000–0x2001FFFF).
- Maximum CTO/DTO size is 8 bytes.
- No support for advanced XCP features like calibration page switching beyond the defined resource byte.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Please submit a pull request or open an issue on the GitHub repository for bug reports or feature requests.

## Author

- **Pathum Dissanayake**

# Created: May 20, 2025
