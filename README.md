# STM32F446 Bootloader OTA

Custom bootloader project for **STM32F446ZET6** implementing firmware update over **LoRa**.

## Project Status

**Progress: 50%**

The bootloader structure, application separation, OTA request mechanism, LoRa communication and firmware transfer infrastructure have been implemented.

The remaining work is mainly focused on reliably reconstructing the received firmware image, writing it to Flash, validating it and starting the new application.

---

## Project Architecture

The project consists of two separate firmware applications:

- **Bootloader** — Handles OTA requests, firmware reception, Flash operations, application validation and application startup.
- **Application** — Runs the main application and can request a firmware update through LoRa.

### Flash Memory Layout

    STM32F446 Flash

    0x08000000
        │
        ├── Bootloader
        │
    0x0800C000
        │
        ├── Application
        │
        └── ...

The current application start address is:

    0x0800C000

---

## OTA Update Flow

    Application
         │
         │ OTA Request
         ▼
        Reset
         │
         ▼
    Bootloader
         │
         │ Start LoRa
         ▼
    Receive Firmware
         │
         ▼
    Reconstruct Image
         │
         ▼
    Erase Application Flash
         │
         ▼
    Write New Firmware
         │
         ▼
    Verify Firmware
         │
         ▼
    Reset / Jump
         │
         ▼
    New Application

---

## LoRa Communication

The current communication architecture is:

    Application STM32
           │
          UART
           │
           ▼
        LoRa TX
           │
           │ LoRa
           ▼
        LoRa RX
           │
          UART
           │
           ▼
    Bootloader STM32

The application sends:

1. Firmware image size
2. Firmware image data in chunks
3. OTA request

The bootloader receives the firmware and reconstructs the image before programming the application Flash.

---

## Application Header

The bootloader uses an application header to store information about the firmware:

    typedef struct
    {
        uint32_t ota_flag;
        uint32_t magic;
        uint32_t size;
        uint32_t crc;
        uint32_t version;
    } app_header_t;

The header is used by the bootloader to determine the state and validity of the application.

---

## Flash Programming

The bootloader contains functionality for:

- Flash unlock
- Flash erase
- Flash programming
- Application header programming
- Flash lock
- Application validation

The intended update sequence is:

    Receive Firmware
           ↓
    Erase Application Region
           ↓
    Write Firmware
           ↓
    Calculate / Verify CRC
           ↓
    Write Application Header
           ↓
    Reset MCU
           ↓
    Bootloader Validates Application
           ↓
    Jump to Application

---

## Current Implementation

### Bootloader

- [x] Bootloader project
- [x] Application memory separation
- [x] Application start address
- [x] Bootloader → Application jump
- [x] Application header
- [x] OTA flag mechanism
- [x] Flash unlock / erase / write
- [x] Application validation infrastructure
- [x] CRC infrastructure
- [x] LoRa UART configuration
- [x] LoRa module configuration
- [x] Firmware size reception
- [x] Firmware image reception
- [ ] Reliable firmware reconstruction
- [ ] Write received image to application Flash
- [ ] Final CRC verification
- [ ] Complete OTA update cycle

### Application

- [x] Normal application execution
- [x] OTA request mechanism
- [x] Firmware image generation
- [x] Firmware size transmission
- [x] Firmware transmission over LoRa
- [x] OTA flag update
- [x] MCU reset

---

## Current Development Stage

The current development focus is the firmware image received through LoRa.

The firmware can be transmitted and received in chunks. The remaining work is to reliably reconstruct the complete firmware image and program it into the application Flash region without data corruption.

The immediate target is:

    LoRa Image
        ↓
    Receive
        ↓
    Reconstruct
        ↓
    Flash Write
        ↓
    CRC Check
        ↓
    Application Header
        ↓
    Reset
        ↓
    New Application

---

## Final Goal

The final goal is a fully functional OTA bootloader for STM32F446 capable of receiving and installing a new firmware image over LoRa without requiring an external programmer.

    New Firmware
         │
         ▼
       LoRa TX
         │
         ▼
       LoRa RX
         │
         ▼
     Bootloader
         │
         ▼
       Flash
         │
         ▼
     Validation
         │
         ▼
    New Application

---

## Hardware

- STM32F446ZET6
- LoRa modules
- UART communication

## Development Tools

- STM32CubeIDE
- STM32CubeProgrammer
- ARM GCC Toolchain

## Technologies

- C
- STM32F446
- STM32 HAL
- UART
- LoRa
- Flash Programming
- CRC32
- Bootloader
- OTA Firmware Update
- ARM Cortex-M4

---

## Project Status

**50% Complete**

Bootloader and OTA communication infrastructure are implemented.

The remaining work is focused on reliable firmware reconstruction, Flash programming, firmware validation and completing the end-to-end OTA update process.
