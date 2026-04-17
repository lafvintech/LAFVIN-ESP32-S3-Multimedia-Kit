#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

// MAX98357A amplifier pins
#define I2S_DOUT      41  // DIN
#define I2S_BCLK      42  // BCLK
#define I2S_LRC       14  // LRCLK

Audio audio;

// --- Enter your WiFi credentials here ---
const char* ssid     = "Your SSID";
const char* password = "Your Password";

void setup() {
    Serial.begin(115200);

    // 1. Connect to WiFi
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

    // 2. Configure the audio pins (I2S)
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // 3. Set the volume (0-21)
    audio.setVolume(10); // It is recommended to start with a lower volume to avoid sudden loud output

    // 4. Start playing the online stream
    audio.connecttohost("http://icecast.omroep.nl/radio4-bb-mp3"); // English radio station

    // Alternatively, you can use Python to share the songs on your local network, then access them using the following format. The command is "python -m http.server 8765" (run this in the directory where the songs are located).
    //audio.connecttohost("http://192.168.3.130:8765/test1.mp3");
}

void loop() {
    // Must be called continuously in loop() to process audio stream data.
    audio.loop();

    // Simple serial control
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

// --- Optional debug callback functions (can show track information) ---
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("Stream Title: "); Serial.println(info);
}