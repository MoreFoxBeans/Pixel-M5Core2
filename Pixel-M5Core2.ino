#include <AXP192.h>
#include <Free_Fonts.h>
#include <M5Core2.h>
#include <M5Display.h>
#include <M5Touch.h>
#include <RTC.h>
#include <Speaker.h>
#include <SD.h>
#include <M5GFX.h>

#define CANVAS_WIDTH    64
#define CANVAS_HEIGHT   32

#define BACKGROUND_COLOR    0x9CD3
#define GUI_COLOR           0xFFFF
#define BORDER_COLOR        0x632C
#define SEPERATOR_COLOR     0xCE79

M5GFX tft;
M5Canvas gui(&tft);
M5Canvas canvas(&tft);

uint16_t viewX = 0;
uint16_t viewY = 0;
uint16_t viewZ = 4;

uint32_t frames = 0;

void drawScreen(bool whole=false, bool clock=false);

void setup(void) {
  M5.begin(true, true, false, false);
  M5.Axp.SetLcdVoltage(map(32, 0, 255, 2500, 3300));
  M5.Axp.SetSpkEnable(0);
  M5.Axp.SetLed(0);
  
  tft.begin();
  tft.fillScreen(BACKGROUND_COLOR);
  tft.loadFont(SD, "font");

  gui.createSprite(320, 240);
  gui.fillSprite(TFT_TRANSPARENT);
  gui.fillRect(0, 202, 320, 2, BORDER_COLOR);
  gui.fillRect(0, 204, 320, 36, GUI_COLOR);
  gui.drawFastVLine(41, 212, 20, SEPERATOR_COLOR);
  gui.drawFastVLine(196, 212, 20, SEPERATOR_COLOR);
  gui.drawFastVLine(279, 212, 20, SEPERATOR_COLOR);

  canvas.createSprite(CANVAS_WIDTH, CANVAS_HEIGHT);
  canvas.fillSprite(TFT_WHITE);
}

void loop(void) {
  M5.update();

  tft.waitDisplay();

  if ((frames % 30) == 0) {
    drawScreen(false, true);
  } else {
    drawScreen(false, false);
  }

  ++frames;

  lgfx::touch_point_t tp[2];

  uint8_t num = tft.getTouch(tp, 2);

  if (num) {
    tft.startWrite();

    for (uint8_t i = 0; i < 2; ++i) {
      tft.fillCircle(tp[i].x, tp[i].y, 5, TFT_RED);
    }

    tft.endWrite();
  }
}

void drawScreen(bool whole=false, bool clock=false) {
  if (whole) {
    clock = false;
  }
  
  tft.setTextDatum(top_right);
  tft.setTextColor(BORDER_COLOR);

  tft.startWrite();

  if (whole) {
    tft.fillScreen(BACKGROUND_COLOR);
  }

  if (clock) {
    tft.fillRect(0, 0, 320, 22, BACKGROUND_COLOR);
  }

  canvas.pushRotateZoom(viewX, viewY, 0, viewZ, viewZ);

  if (clock || whole) {
    String status = "3:04 PM / " + String(round(M5.Axp.GetBatteryLevel()), 0) + "%";
    tft.drawString(status, 320 - 8, 8);
  }

  gui.pushSprite(0, 0, TFT_TRANSPARENT);

  tft.endWrite();
}
