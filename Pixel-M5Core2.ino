#include <SD.h>
#include <M5Unified.h>

#define CANVAS_WIDTH    64
#define CANVAS_HEIGHT   32

#define BACKGROUND_COLOR    0x9CD3
#define GUI_COLOR           0xFFFF
#define BORDER_COLOR        0x632C
#define SEPERATOR_COLOR     0xCE79

M5Canvas gui(&M5.Display);
M5Canvas canvas(&M5.Display);

uint16_t viewX = 0;
uint16_t viewY = 0;
uint16_t viewZ = 4;

uint32_t frames = 0;

void drawScreen(bool whole=false, bool clock=false);

void setup(void) {
  auto cfg = M5.config();
  cfg.clear_display  = true;
  cfg.output_power   = false;
  cfg.internal_imu   = true;
  cfg.internal_rtc   = true;
  cfg.internal_spk   = false;
  cfg.internal_mic   = false;
  cfg.external_imu   = false;
  cfg.external_rtc   = false;
  cfg.external_spk   = false;
  cfg.led_brightness = 0;

  M5.begin(cfg);
  
  M5.Display.setBrightness(32);
  M5.Display.begin();
  M5.Display.loadFont("font", SD);

  gui.createSprite(320, 240);
  gui.fillSprite(TFT_TRANSPARENT);
  gui.fillRect(0, 202, 320, 2, BORDER_COLOR);
  gui.fillRect(0, 204, 320, 36, GUI_COLOR);
  gui.drawFastVLine(41, 212, 20, SEPERATOR_COLOR);
  gui.drawFastVLine(196, 212, 20, SEPERATOR_COLOR);
  gui.drawFastVLine(279, 212, 20, SEPERATOR_COLOR);

  canvas.createSprite(CANVAS_WIDTH, CANVAS_HEIGHT);
  canvas.fillSprite(TFT_WHITE);

  M5.Display.fillScreen(BACKGROUND_COLOR);
}

void loop(void) {
  M5.update();

  M5.Display.waitDisplay();

  if ((frames % 30) == 0) {
    drawScreen(false, true);
  } else {
    drawScreen(false, false);
  }

  ++frames;

  lgfx::touch_point_t tp[1];

  uint8_t num = M5.Display.getTouch(tp, 1);

  if (num) {
    M5.Display.startWrite();
    M5.Display.fillCircle(tp[0].x, tp[0].y, 5, TFT_RED);
    M5.Display.endWrite();
  }
}

void drawScreen(bool whole, bool clock) {
  if (whole) {
    clock = false;
  }
  
  M5.Display.startWrite();

  if (whole) {
    M5.Display.fillScreen(BACKGROUND_COLOR);
  }

  if (clock) {
    M5.Display.fillRect(0, 0, 320, 22, BACKGROUND_COLOR);
  }

  canvas.pushRotateZoom(viewX, viewY, 0, viewZ, viewZ);

  if (clock || whole) {
    M5.Display.setTextDatum(top_right);
    M5.Display.setTextColor(BORDER_COLOR);
    M5.Display.setTextSize(1);

    String status = "3:04 PM / " + String(M5.Power.getBatteryLevel()) + "%";
    M5.Display.drawString(status, 320 - 8, 8);
  }

  gui.pushSprite(0, 0, TFT_TRANSPARENT);

  M5.Display.endWrite();
}
