#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <stdint.h>
#include <string.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

File uploadFile;

HardwareSerial LoRaSerial(2);

#define LORA_RX_PIN 16
#define LORA_TX_PIN 17

#define LORA_M0_PIN 25
#define LORA_M1_PIN 26
#define LORA_AUX_PIN 27

#define DEBUG_LED_PIN 2

uint8_t setTCmd[6] = {
    0xC0,
    0x00,
    0x10,
    0x1A,
    0x07,
    0x44
};

uint8_t imageSizeACK;
uint8_t chunkACK;


const char* uploadPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>STM32 OTA</title>
</head>

<body>

<h2>STM32 OTA Firmware Upload</h2>

<form method="POST"
      action="/upload"
      enctype="multipart/form-data">

    <input type="file" name="firmware">

    <br><br>

    <input type="submit" value="Upload">

</form>

</body>
</html>
)rawliteral";


void handleRoot()
{
    server.send(200, "text/html", uploadPage);
}


void handleUpload()
{
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.println("Upload was started.");

        Serial.print("File: ");
        Serial.println(upload.filename);

        // Firstly, delete the previous firmware
        if (LittleFS.exists("/ota_image.bin"))
        {
            LittleFS.remove("/ota_image.bin");
        }

        uploadFile = LittleFS.open(
            "/ota_image.bin",
            FILE_WRITE
        );

        if (!uploadFile)
        {
            Serial.println("File could not open!");
        }
    }

    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (uploadFile)
        {
            uploadFile.write(
                upload.buf,
                upload.currentSize
            );
        }

        Serial.print("Received Bytes: ");
        Serial.println(upload.currentSize);
    }

    else if (upload.status == UPLOAD_FILE_END)
    {
        if (uploadFile)
        {
            uploadFile.close();
        }

        Serial.println("Upload completed.");

        Serial.print("Total Size: ");
        Serial.println(upload.totalSize);
    }
}


void handleUploadFinished()
{
    server.send(
        200,
        "text/plain",
        "Firmware was uploaded successfully."
    );
}

bool LoRaReady()
{
    return digitalRead(LORA_AUX_PIN) == HIGH;
}

void SleepTMs()
{
    
    digitalWrite(LORA_M0_PIN, HIGH);
    digitalWrite(LORA_M1_PIN, HIGH);

    delay(50);
}


void WakeUpTMs()
{
    digitalWrite(LORA_M0_PIN, LOW);
    digitalWrite(LORA_M1_PIN, LOW);

    delay(50);
}

void SetTModuleParameters()
{
    while (LoRaReady())
    {
        LoRaSerial.write(setTCmd, 6);

        // finalize UART Transmission
        LoRaSerial.flush();
    }
}

void SendImageSize(uint32_t size)
{
    while (!LoRaReady())
    {
        Serial.println("LoRa AUX is not ready!");
    }


    LoRaSerial.write(
        (uint8_t *)&size,
        sizeof(size)
    );

    LoRaSerial.flush();

}

bool GetImageSizeACK()
{
    uint32_t timeout = millis() + 1000;

    while (LoRaSerial.available() == 0)
    {
        if (millis() > timeout)
        {
            Serial.println("ImageSize ACK timeout!");
            return false;
        }
    }

    imageSizeACK = LoRaSerial.read();
    return imageSizeACK == 0x01;
}

bool GetChunkACK()
{
    uint32_t timeout = millis() + 1000; 

    while (LoRaSerial.available() == 0)
    {
        if (millis() > timeout)
        {
            Serial.println("Chunk ACK timeout!");
            return false;
        }
    }

    chunkACK = LoRaSerial.read();
    return chunkACK == 0x01;
}

void SendImage(uint8_t *data, uint32_t size)
{
    while (!LoRaReady())
    {
        Serial.println("LoRa AUX is not ready!");
    }

    
    LoRaSerial.write(data, size);

    LoRaSerial.flush();

}

bool GetOtaRequest()
{
    while (!LoRaReady());
    
    if (LoRaSerial.available() == 0)
    {
        return false;
    }

    uint8_t enableOTA = LoRaSerial.read();
    
    return enableOTA == 0x01;
}

void SendOTA()
{
    File file = LittleFS.open("/ota_image.bin", FILE_READ);

    if (!file)
    {
        Serial.println("Could not open ota_image.bin !");
        return;
    }

    uint32_t imageSize = file.size();

    Serial.print("Image size: ");
    Serial.println(imageSize);

    uint8_t txData[32];

    // Firstly send image size
    
    SendImageSize(imageSize);

    Serial.println("Waiting for ImageSize ACK...");

    if (!GetImageSizeACK())
    {
        Serial.println("Could not received ImageSize ACK");
        file.close();
        return;
    }

    Serial.println("ImageSize ACK was successful.");
    

    uint32_t remaining = imageSize;
    uint32_t numChunk = 0;

    while (remaining > 0)
    {
        uint32_t len =
            (remaining > sizeof(txData))
            ? sizeof(txData)
            : remaining;

        size_t readLen = file.read(txData, len);

        if (readLen != len)
        {
            Serial.println("Could not read file!");
            file.close();
            return;
        }

        Serial.print("Chunk ");
        Serial.print(numChunk);
        Serial.print(" is sending. Size: ");
        Serial.println(len);

        SendImage(txData, len);

        // ACK bekle
        if (!GetChunkACK())
        {
            Serial.print("Could not receive Chunk ");
            Serial.print(numChunk);
            Serial.println(" ACK.");

            file.close();
            return;
        }

        Serial.print("Chunk ");
        Serial.print(numChunk);
        Serial.println(" ACK is successful.");

        remaining -= len;
        numChunk++;

        digitalWrite(
            DEBUG_LED_PIN,
            !digitalRead(DEBUG_LED_PIN)
        );

        Serial.print("Transmitted: ");
        Serial.print(imageSize - remaining);
        Serial.print(" / ");
        Serial.println(imageSize);

    }
 
    file.close();

    Serial.println("OTA image was sent.");
}



void setup()
{
    Serial.begin(9600);

    // LittleFS 
    if (!LittleFS.begin(true))
    {
        Serial.println("Could not begin LittleFS!");
        while (1);
    }

    Serial.println("LittleFS OK");


    // Wi-Fi
    WiFi.begin(ssid, password);

    Serial.print("Connecting to Wi-Fi...");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Connected to Wi-Fi");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());


    // HTTP
    server.on("/", HTTP_GET, handleRoot);

    server.on(
        "/upload",
        HTTP_POST,
        handleUploadFinished,
        handleUpload
    );

    server.begin();

    Serial.println("HTTP server up");

    // E32 UART

    pinMode(LORA_M1_PIN, OUTPUT);
    pinMode(LORA_M0_PIN, OUTPUT);
    pinMode(LORA_AUX_PIN, INPUT);
    pinMode(DEBUG_LED_PIN, OUTPUT);

    LoRaSerial.begin(
        9600,
        SERIAL_8N1,
        LORA_RX_PIN,
        LORA_TX_PIN
    );


    // E32 configuration mode
    SleepTMs();

    SetTModuleParameters();

    // Normal transmission mode
    WakeUpTMs();

    Serial.println("LoRa Ready..");
}


void loop()
{
 
    server.handleClient();

    if (GetOtaRequest()) // wait for OTA request.
    {
        Serial.println("OTA Request was received!");
        SendOTA();
    }

}
