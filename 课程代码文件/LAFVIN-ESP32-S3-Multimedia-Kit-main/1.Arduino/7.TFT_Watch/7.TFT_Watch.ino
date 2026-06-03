#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// --- Classic LCD Colors (Positive Mode) ---
// Background: Light Grey/Greenish (Typical LCD backing)
#define LCD_BACK_LIGHT  0xC618 // A silver-ish grey
#define LCD_TEXT_BLACK  TFT_BLACK
#define WATCH_FRAME     0x39C4 // Darker grey for the bezel

uint32_t targetTime = 0;       // for next 1 second timeout

// Variables to hold time
static uint8_t conv2d(const char* p); // Forward declaration
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time

void setup(void) {
  tft.init();
  tft.setRotation(0); // Portrait
  
  tft.invertDisplay(true); 


  // 1. Fill the entire screen with WHITE (The "Outside" area)
  tft.fillScreen(TFT_WHITE);
  
  // --- Watch Face / Bezel Design ---
  // 2. Draw the LCD Screen Area (The "Inside" area)
  // First, the frame/bezel
  tft.fillRect(5, 40, 230, 220, WATCH_FRAME); 
  
  // Then the actual LCD active area (slightly smaller than frame)
  tft.fillRect(15, 50, 210, 200, LCD_BACK_LIGHT);

  // Decorative Lines inside the LCD
  tft.drawFastHLine(20, 80, 200, TFT_BLUE);
  tft.drawFastHLine(20, 220, 200, TFT_BLUE);
  
  // Small Labels inside the LCD
  tft.setTextColor(LCD_TEXT_BLACK, LCD_BACK_LIGHT);
  tft.setTextDatum(TL_DATUM); 
  tft.drawString("ALARM CHRONO", 20, 55, 2);
  
  tft.setTextDatum(TR_DATUM); 
  tft.drawString("LIGHT", 220, 55, 2);

  // Branding (Outside the LCD, on the White part)
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE); // Grey text on White background
  tft.drawString("LAFVIN", 120, 280, 4); 
}

void loop() {
  if (targetTime < millis()) {
    targetTime += 1000; // Update every second

    // --- Time Logic ---
    ss++;              
    if (ss == 60) {
      ss = 0;
      mm++;            
      if (mm > 59) {
        mm = 0;
        hh++;          
        if (hh > 23) {
          hh = 0;
        }
      }
    }

    // --- Drawing Logic ---
    
    String hourStr = "";
    if (hh < 10) hourStr += "0";
    hourStr += hh;
    
    String minStr = "";
    if (mm < 10) minStr += "0";
    minStr += mm;

    String secStr = "";
    if (ss < 10) secStr += "0";
    secStr += ss;

    // We use Font 7 (7-Segment) for EVERYTHING now.
    tft.setTextColor(LCD_TEXT_BLACK, LCD_BACK_LIGHT);
    
    // 1. Draw Hours:Minutes (Top Line)
    String mainTime = hourStr + ":" + minStr;
    tft.setTextDatum(MC_DATUM); 
    tft.drawString(mainTime, 120, 130, 7); 

    // 2. Draw Seconds (Bottom Line)
    tft.setTextSize(1); // Reset size just in case
    tft.drawString(secStr, 120, 190, 7);

  }
}

// Helper to extract numbers from Compile Time string
static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

