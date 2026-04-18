#include "FS.h"
#include "SD_MMC.h"

// Specific pin definitions for ESP32-S3
// These pins might differ for other ESP32 boards; check your board's schematic.
int clk = 39;
int cmd = 38;
int d0 = 40; 

void setup() {
  Serial.begin(115200);
  
  // 1. Configure SD card pins
  // This must be called before SD_MMC.begin() to set the correct GPIOs.
  if(! SD_MMC.setPins(clk, cmd, d0)){
      Serial.println("Pin configuration failed!");
      return;
  }

  // 2. Initialize the SD card
  // "/sdcard": Mount point for the SD card.
  // true: Enables 1-bit mode (uses only one data line, D0). Use 'false' for 4-bit mode.
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD card mount failed!");
    return;
  }

  // 3. Check card type
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached.");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // 4. Run file operation demo (functions defined in sd_helpers.ino)
  // These functions demonstrate basic operations like creating directories, reading/writing files.
  Serial.println("--- Starting file operation test ---");
  listDir(SD_MMC, "/", 0);
  createDir(SD_MMC, "/mydir");
  listDir(SD_MMC, "/", 0);
  removeDir(SD_MMC, "/mydir");
  listDir(SD_MMC, "/", 2);
  writeFile(SD_MMC, "/hello.txt", "Hello ");
  appendFile(SD_MMC, "/hello.txt", "World!\n");
  readFile(SD_MMC, "/hello.txt");
  deleteFile(SD_MMC, "/foo.txt");
  renameFile(SD_MMC, "/hello.txt", "/foo.txt");
  readFile(SD_MMC, "/foo.txt");
  testFileIO(SD_MMC, "/test.txt");
  
  Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
}

void loop() {
  delay(10);
}