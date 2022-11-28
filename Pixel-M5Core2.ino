#include <SD.h>
#include <M5Unified.h>

// https://rop.nl/truetype2gfx/
#include "Pixel128pt7b.h"

#define CANVAS_WIDTH    64
#define CANVAS_HEIGHT   32

#define BACKGROUND_COLOR    0x9CD3
#define GUI_COLOR           0xFFFF
#define BORDER_COLOR        0x632C
#define SEPERATOR_COLOR     0xCE79

M5Canvas screen(&M5.Display);
M5Canvas gui(&screen);
M5Canvas canvas(&screen);

uint8_t pnum;
uint8_t num;
int16_t otx;
int16_t oty;
int16_t oviewX;
int16_t oviewY;

int16_t viewX = 0;
int16_t viewY = 0;
uint8_t viewZ = 4;

uint32_t frames = 0;

void drawScreen(bool whole, bool drawClock, bool drawCanvas);

void setup(void) {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
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

  screen.createSprite(320, 240);
  screen.fillSprite(BACKGROUND_COLOR);
  screen.setTextSize(1);
  screen.setFont(&Pixel128pt7b);
  screen.setTextWrap(false, false);

  gui.createSprite(320, 240);
  gui.fillSprite(TFT_TRANSPARENT);
  gui.writeFillRect(0, 202, 320, 2, BORDER_COLOR);
  gui.writeFillRect(0, 204, 320, 36, GUI_COLOR);
  gui.writeFastVLine(41, 212, 20, SEPERATOR_COLOR);
  gui.writeFastVLine(196, 212, 20, SEPERATOR_COLOR);
  gui.writeFastVLine(279, 212, 20, SEPERATOR_COLOR);

  canvas.createSprite(CANVAS_WIDTH, CANVAS_HEIGHT);
  canvas.fillSprite(TFT_WHITE);

  drawScreen(true, true, true);
}

void loop(void) {
  M5.update();

  bool canvasMoved = false;

  lgfx::touch_point_t tp[1];

  num = M5.Display.getTouch(tp, 1);

  if (!pnum && num) {
    otx = tp[0].x;
    oty = tp[0].y;
    oviewX = viewX;
    oviewY = viewY;
  }

  if (pnum && num) {
    viewX = oviewX + (tp[0].x - otx);
    viewY = oviewY + (tp[0].y - oty);
    canvasMoved = true;
  }

  pnum = num;

  if ((frames % 100) == 0) {
    drawScreen(canvasMoved, true, canvasMoved);
  } else {
    drawScreen(canvasMoved, false, canvasMoved);
  }

  M5.Display.waitDisplay();
  screen.pushSprite(0, 0);

  ++frames;
}

void drawScreen(bool whole, bool drawClock, bool drawCanvas) {
  if (whole) {
    drawClock = false;
    drawCanvas = false;

    screen.fillSprite(BACKGROUND_COLOR);
  }
  
  char rbuf[32];
  char lbuf[32];

  if (drawClock || drawCanvas || whole) {
    m5::rtc_datetime_t dt;

    if (M5.Rtc.getDateTime(&dt)) {
      snprintf(rbuf, sizeof rbuf, "%d:%02d / %d%%", dt.time.hours, dt.time.minutes, M5.Power.getBatteryLevel());
    } else {
      snprintf(rbuf, sizeof rbuf, "ERROR / %d%%", M5.Power.getBatteryLevel());
    }

    snprintf(lbuf, sizeof lbuf, "(%d, %d)", viewX, viewY);
  }
  
  if (drawClock) {
    uint16_t rwidth = screen.textWidth(rbuf) + 20;
    screen.writeFillRect(320 - 8 - rwidth, 8, rwidth, 16, BACKGROUND_COLOR);

    uint16_t lwidth = screen.textWidth(lbuf) + 20;
    screen.writeFillRect(8, 8, lwidth, 16, BACKGROUND_COLOR);
  }

  if (drawCanvas || ((viewY <= 24) && drawClock) || whole) {
    canvas.pushRotateZoom(&screen, viewX + ((CANVAS_WIDTH * (uint16_t)viewZ + viewZ) >> 1), viewY + ((CANVAS_HEIGHT * (uint16_t)viewZ + viewZ) >> 1), 0, viewZ, viewZ);
    screen.drawRect(viewX - 1, viewY - 1, CANVAS_WIDTH * viewZ + 2, CANVAS_HEIGHT * viewZ + 2, BORDER_COLOR);
    screen.drawRoundRect(viewX - 2, viewY - 2, CANVAS_WIDTH * viewZ + 4, CANVAS_HEIGHT * viewZ + 4, 2, BORDER_COLOR);
  }

  if (drawClock || drawCanvas || whole) {
    screen.setTextColor(BORDER_COLOR);

    screen.setTextDatum(top_right);
    screen.drawString(rbuf, 320 - 8, 8);

    screen.setTextDatum(top_left);
    screen.drawString(lbuf, 8, 8);
  }

  gui.pushSprite(&screen, 0, 0, TFT_TRANSPARENT);
}
