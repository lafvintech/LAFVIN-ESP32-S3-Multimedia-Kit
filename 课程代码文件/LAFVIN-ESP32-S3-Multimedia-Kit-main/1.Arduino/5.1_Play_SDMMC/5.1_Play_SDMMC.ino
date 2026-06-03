#include "Arduino.h"
#include "Audio.h"
#include "SD_MMC.h"

#define SD_MMC_CMD 38 
#define SD_MMC_CLK 39 
#define SD_MMC_D0  40
#define I2S_BCLK   42
#define I2S_DOUT   41
#define I2S_LRC    14

Audio audio;

void my_audio_info(Audio::msg_t m) {
    Serial.printf("%s: %s\n", m.s, m.msg);
}

void setup() {
    Audio::audio_info_callback = my_audio_info;
    Serial.begin(115200);
    pinMode(SD_MMC_D0, INPUT_PULLUP);
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    SD_MMC.begin("/sdcard", true);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(15); // default 0-21
    audio.connecttoFS(SD_MMC, "music/test1.mp3");
}

void loop() {
    audio.loop();
    vTaskDelay(1);
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}