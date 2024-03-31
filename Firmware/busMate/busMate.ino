#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SDA_PIN 21         // Connects to the SDA pin of PN532 (an RFID/NFC sensor)
#define SCL_PIN 22         // Connects to the SCL pin of PN532
#define BAUDRATE 57600     // Baud rate of UART

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);  // Use PN532 library to interact with NFC sensor
SoftwareSerial HZ1050(4, 3);           // Use SoftwareSerial to communicate with another device (HZ1050 module)

String value_key_13MHz = "";   // String to store the value from the 13MHz module key
String value_key_125kHz = "";  // String to store the value from the 125kHz module key

void setup(void) {
  Serial.begin(9600);   // Initialize serial port for PC
  HZ1050.begin(9600);   // Start serial connection with 125kHz RFID reader
  nfc.begin();          // Start connection with 13.56MHz RFID reader

  //initialize for 13MHz RFID module
  initialize_RFID_13MHz();

  // Create tasks
  xTaskCreatePinnedToCore(taskRFID125kHzFunction, "RFID125kHz", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskRFID13MHzFunction, "RFID13MHz", 10000, NULL, 1, NULL, 1);
}

void loop(void) {
  // Empty loop as tasks are managed by FreeRTOS Scheduler
}

//Function to initialize 13.56MHz RFID module
void initialize_RFID_13MHz() {
  // Get firmware version information of RFID module
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("PN53x board not found");
  }
  // Configure operation mode for RFID module
  nfc.SAMConfig();
}

//Function to process information from 13.56MHz RFID tag
String process_RFID_13MHz(uint8_t uid[], uint8_t uidLength) {
  String value = "";
  // Iterate through bytes in RFID tag UID and append to result string
  for (uint8_t i = 0; i < uidLength; i++) {
    value += uid[i];
  }
  return value;
}

//Function to process information from 125kHz RFID tag
String process_RFID_125kHz() {
  long value = 0;
  // Check if there is new data from 125kHz RFID module
  if (HZ1050.available() > 0) {
    // Read 4 bytes of data from 125kHz RFID module
    for (int j = 0; j < 4; j++) {
      while (HZ1050.available() == 0) {};  // Wait for new data
      int i = HZ1050.read();
      value += ((long)i << (24 - (j * 8)));  // Construct value from 4 bytes of data
    }
    return String(value);
  }
}

//Print out the ID of 125kHz RFID tag
void taskRFID125kHzFunction(void *pvParameters) {
  for (;;) {
    // Check if there is data from 125kHz RFID module
    if (HZ1050.available() > 0) {
      // Read and process information from 125kHz RFID tag
      value_key_125kHz = process_RFID_125kHz();
      Serial.print("value 125KHz: ");
      Serial.println(value_key_125kHz);
      delay(1000);
    }
    // Delay between task iterations
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

//Print out the ID of 13.56MHz RFID tag
void taskRFID13MHzFunction(void *pvParameters) {
  for (;;) {
    // Declare variable to store UID of 13.56MHz RFID tag
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;

    // Check if there is a 13.56MHz RFID tag within read range
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      value_key_13MHz = process_RFID_13MHz(uid, uidLength);
      Serial.print("value 13.56MHz: ");
      Serial.println(value_key_13MHz);
      delay(1000);
    }

    // Delay between task iterations
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
