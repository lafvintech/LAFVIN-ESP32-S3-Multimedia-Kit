#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

// MAX98357A 功放引脚
#define I2S_DOUT      41  // DIN
#define I2S_BCLK      42  // BCLK
#define I2S_LRC       14  // LRCLK

Audio audio;

// --- 在这里填入你的 WiFi 信息 ---
const char* ssid     = "2A811";
const char* password = "la1234567890";

void setup() {
    Serial.begin(115200);
    
    // 1. 连接 WiFi
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.println("\nConnecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // 2. 配置音频引脚 (I2S)
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // 3. 设置音量 (0-21)
    audio.setVolume(10); // 建议先设小一点，以免吓一跳

    // 4. 开始播放网络流
    // 注意：原来的那个 kuwo 链接可能已经失效或有防盗链，建议用下面这个非常稳定的流测试
    
    // 这里的流是一首循环播放的测试音乐，连接速度快
    //audio.connecttohost("http://icecast.omroep.nl/radio4-bb-mp3"); // 英文电台
    
    //audio.connecttohost("http://192.168.3.130:8765/test1.mp3");
}

void loop() {
    // 必须在 loop 中不断调用，处理音频数据流
    audio.loop();
    
    // 简单的串口控制功能
    if(Serial.available()){
        String r = Serial.readString(); 
        r.trim();
        if(r.length() > 5) {
            audio.stopSong();
            audio.connecttohost(r.c_str());
            Serial.print("Switching to: "); Serial.println(r);
        }
    }
}

// --- 可选的调试回调函数 (可以看到歌曲信息) ---
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("Stream Title: "); Serial.println(info);
}