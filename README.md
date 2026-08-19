# STM32F446 Bootloader OTA

!!! WARNING: ota_image_bin buffer size is fixed at compile-time (OTA_MAX_SIZE). Increasing this to support larger images directly increases static RAM usage, which may not be feasible given limited SRAM. Consider streaming chunks directly to flash instead of buffering the whole image in RAM.

You can find the relevant Python code for generating ota_image.bin in the application/debug/app_final.py file.

You can also use the resources in Parts 1–2–3–4–5 of the **https://controllerstech.com/stm32-custom-bootloader-tutorial/** to better understand the project.

I used bootloader_comm.ino code for ESP32.

Custom bootloader project for **STM32F446ZET6** implementing firmware updates over **LoRa**.

The project is designed to update the application firmware stored in the STM32 internal Flash without requiring an external programmer during the OTA update process.


---

## Project Status

**Progress: ~99%**

The core bootloader architecture, application separation, OTA request mechanism, firmware image generation, Flash programming infrastructure and firmware validation infrastructure have been implemented.

The current development focus is establishing reliable communication between the STM32 bootloader and the ESP32 OTA gateway through LoRa.

---

## Project Architecture

The system consists of three main components:

- **STM32 Bootloader** — Handles OTA requests, firmware reception, Flash operations, application validation and application startup.
- **STM32 Application** — Runs the main application and can request a firmware update.
- **ESP32 OTA Gateway** — Receives the firmware image from a PC and transfers it to the STM32 bootloader through LoRa.

### System Architecture

    PC
     │
     │ ota_image.bin
     ▼
    ESP32
     │
     │ UART
     ▼
    LoRa Module
     │
     │ LoRa
     ▼
    LoRa Module
     │
     │ UART
     ▼
    STM32F446
     │
     ├── Bootloader
     │
     └── Application

---

## Flash Memory Layout

The STM32 Flash memory is divided into separate regions for the bootloader and application.

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

The bootloader occupies the Flash region before the application start address.

The application region is the only region that should be erased and reprogrammed during an OTA update.

---

## OTA Update Flow

The intended OTA process is:

    Running Application
            │
            │ OTA Request
            ▼
        Store OTA Flag
            │
            ▼
          Reset
            │
            ▼
        Bootloader
            │
            │ Check OTA Flag
            ▼
       Request Firmware
            │
            ▼
          ESP32
            │
            │ Firmware Image
            ▼
        LoRa Transfer
            │
            ▼
       STM32 Bootloader
            │
            ▼
      Receive Image Size
            │
            ▼
      Receive Image Chunks
            │
            ▼
       Reconstruct Image
            │
            ▼
       Validate Firmware
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

## OTA Communication Protocol

The current communication protocol uses a simple ACK-based transfer mechanism.

### OTA Request

The STM32 application sets the OTA request flag and resets the MCU.

    Application
        │
        │ OTA Flag
        ▼
    Bootloader
        │
        │ OTA Request
        ▼
      ESP32

The ESP32 waits for the OTA request before starting the firmware transfer.

### Firmware Size

The ESP32 first sends the firmware image size.

    ESP32
       │
       │ Image Size (uint32_t)
       ▼
    STM32 Bootloader
       │
       │ Validate Size
       ▼
    ACK / NACK

The STM32 checks that the received image size is within the allowed OTA image buffer size.

### Firmware Transfer

The firmware image is transferred in small chunks.

Current chunk size:

    32 bytes

Example:

    ESP32
       │
       │ Chunk 0
       ▼
    STM32
       │
       │ ACK
       ▼
    ESP32
       │
       │ Chunk 1
       ▼
    STM32
       │
       │ ACK
       ▼
      ...

The STM32 stores the received data in RAM before Flash programming.

---

## STM32 Bootloader

The bootloader is responsible for:

- Detecting OTA requests
- Communicating with the LoRa module
- Receiving firmware metadata
- Receiving firmware chunks
- Reconstructing the firmware image in RAM
- Erasing the application Flash region
- Programming the new firmware
- Calculating and validating CRC
- Validating the application
- Jumping to the application

### Bootloader → Application

After a successful update, the bootloader transfers execution to the application.

    Bootloader
        │
        │ Validate Application
        ▼
    Set VTOR
        │
        ▼
    Load MSP
        │
        ▼
    Reset Handler
        │
        ▼
    Application

The bootloader remains in its dedicated Flash region and does not overwrite itself during an OTA update.

---

## STM32 Application

The STM32 application is responsible for:

- Normal application execution
- Generating an OTA request
- Setting the OTA flag
- Resetting the MCU
- Providing the firmware image during development and testing

The OTA request mechanism allows the running application to request a firmware update.

The application sets the OTA flag and resets the MCU.

After reset, the bootloader checks the OTA flag and starts the OTA process if an update has been requested.

---

## ESP32 OTA Gateway

The ESP32 acts as the bridge between the PC and the STM32 bootloader.

The intended workflow is:

    PC
     │
     │ Upload ota_image.bin
     ▼
    ESP32
     │
     │ Store Firmware
     ▼
    LittleFS
     │
     │ Firmware Size
     ▼
    LoRa
     │
     ▼
    STM32 Bootloader

The ESP32 stores the binary firmware image in LittleFS and transfers it to the STM32 bootloader in chunks.

The ESP32 is responsible for:

- Receiving the firmware image from the PC
- Storing the firmware image
- Detecting the firmware image size
- Waiting for the OTA request
- Sending the firmware size
- Sending firmware chunks
- Waiting for ACK responses
- Monitoring transfer progress

---

## LoRa Communication

The LoRa modules communicate with the STM32 and ESP32 through UART.

    STM32F446
        │
       UART
        │
        ▼
    LoRa Module
        │
        │ RF
        ▼
    LoRa Module
        │
       UART
        │
        ▼
      ESP32

The current development hardware uses:

- STM32F446ZET6
- ESP32
- E32-900T20D LoRa modules

The LoRa modules are connected to the STM32 and ESP32 through UART interfaces.

The E32 modules are configured using their UART configuration interface.

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
    Reconstruct Image in RAM
           ↓
    Validate Image
           ↓
    Erase Application Region
           ↓
    Write Firmware to Flash
           ↓
    Calculate / Verify CRC
           ↓
    Write Application Header
           ↓
    Reset MCU
           ↓
    Validate Application
           ↓
    Jump to Application

---

## Final Goal

The final goal is a fully functional OTA bootloader for STM32F446 capable of receiving and installing a new firmware image over LoRa without requiring an external programmer.

The final system will allow:

    PC
     │
     │ New Firmware
     ▼
    ESP32 OTA Gateway
     │
     ▼
    LoRa
     │
     ▼
    STM32 Bootloader
     │
     ▼
    RAM
     │
     ▼
    Flash
     │
     ▼
    CRC / Validation
     │
     ▼
    New Application

The bootloader will remain protected from application updates while only the application region is erased and reprogrammed.

---

## Hardware

- STM32F446ZET6
- ESP32 DEVKIT V1
- E32-900T20D LoRa modules
- UART interfaces
- STM32 internal Flash

---

## Development Tools

- STM32CubeIDE
- STM32CubeProgrammer
- ARM GCC Toolchain
- Arduino IDE
- Python

---

## Technologies

- C
- Python
- STM32F446
- ARM Cortex-M4
- STM32 HAL
- UART
- LoRa
- E32-900T20D
- Flash Programming
- CRC32
- Bootloader
- OTA Firmware Update
- ESP32
- LittleFS

---
