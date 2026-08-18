# STM32F446 Bootloader OTA

Custom bootloader project for **STM32F446ZET6** implementing firmware updates over **LoRa**.

The project is designed to update the application firmware stored in the STM32 internal Flash without requiring an external programmer during the OTA update process.

---

## Project Status

**Progress: ~60%**

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

## Current Implementation

### STM32 Bootloader

- [x] Bootloader project
- [x] Application memory separation
- [x] Application start address
- [x] Bootloader → Application jump
- [x] Application header
- [x] OTA flag mechanism
- [x] Flash unlock
- [x] Flash erase
- [x] Flash write infrastructure
- [x] Flash lock
- [x] Application validation infrastructure
- [x] CRC32 infrastructure
- [x] UART / LoRa interface
- [x] LoRa module configuration
- [x] OTA request detection
- [x] Firmware size reception
- [x] Firmware size validation
- [x] Firmware chunk reception
- [x] RAM firmware buffer
- [x] Image reconstruction infrastructure
- [x] Image size ACK
- [x] Chunk ACK
- [ ] Reliable LoRa communication
- [ ] Reliable chunk retransmission
- [ ] Complete RAM image validation
- [ ] Complete Flash programming flow
- [ ] Final CRC verification
- [ ] Complete end-to-end OTA cycle

### STM32 Application

- [x] Normal application execution
- [x] OTA request mechanism
- [x] OTA flag update
- [x] MCU reset
- [x] Firmware image generation
- [x] Firmware size generation
- [x] Firmware image transmission infrastructure
- [x] Chunk-based firmware transmission

### ESP32 OTA Gateway

- [x] ESP32 firmware
- [x] PC → ESP32 firmware upload
- [x] Binary firmware storage
- [x] LittleFS integration
- [x] Firmware size detection
- [x] Chunk-based firmware transmission infrastructure
- [x] ACK-based transfer infrastructure
- [ ] Reliable LoRa communication
- [ ] Complete STM32 ↔ ESP32 OTA handshake
- [ ] Retry / retransmission mechanism
- [ ] Complete end-to-end firmware transfer

---

## Current Development Stage

The current development stage focuses on establishing a reliable communication channel between the ESP32 OTA gateway and the STM32 bootloader through the E32-900T20D LoRa modules.

The current target is:

    PC
     │
     ▼
    ota_image.bin
     │
     ▼
    ESP32
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
    CRC Validation
     │
     ▼
    Application

The firmware transfer protocol currently uses:

- Firmware size
- 32-byte chunks
- ACK responses
- RAM-based image reconstruction

Reliable LoRa communication and retransmission are still under development.

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
- ESP32
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

## Project Status

**~60% Complete**

The fundamental bootloader architecture and OTA infrastructure are implemented.

The current focus is establishing reliable LoRa communication between the ESP32 OTA gateway and the STM32 bootloader, followed by reliable firmware reconstruction, Flash programming, CRC validation and completion of the end-to-end OTA update process.
