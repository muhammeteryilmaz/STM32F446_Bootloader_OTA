# STM32F446 Bootloader OTA

Custom bootloader project for **STM32F446ZET6** implementing firmware updates over **LoRa**.

The project is designed to update the application firmware stored in the STM32 internal Flash without requiring an external programmer during the OTA update process.

---

## Project Status

**Progress: ~60%**

The core bootloader architecture, application separation, OTA request mechanism, firmware image generation, Flash programming infrastructure and firmware validation infrastructure have been implemented.

The current development focus is the communication layer between the STM32 bootloader and an external ESP32 gateway, and the reliable transfer of the firmware image over LoRa.

---

## Project Architecture

The system consists of three main components:

- **STM32 Bootloader** — Receives the firmware image, reconstructs it in RAM, validates it, programs the application Flash region and starts the application.
- **STM32 Application** — Runs the main firmware and can request an OTA update.
- **ESP32 OTA Gateway** — Receives the firmware image from a PC and transfers it to the STM32 bootloader through LoRa.

### System Architecture

    PC
     │
     │ Firmware Image
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

### 1. OTA Request

The STM32 application sets the OTA request flag and resets the MCU.

After reset:

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

---

### 2. Firmware Size

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

---

### 3. Firmware Transfer

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

## Firmware Image Format

The firmware image is generated from the application binary.

The current image generation process is:

    application.bin
          │
          ▼
      Add Header
          │
          ▼
      ota_image.bin
          │
          ▼
        bin2c
          │
          ▼
      ota_image.h

The firmware header currently contains:

```c
typedef struct
{
    uint32_t magic;
    uint32_t size;
    uint32_t crc;
    uint32_t version;
} app_header_t;
