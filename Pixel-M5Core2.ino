#include <SD.h> // Have to include SD.h to load files from the TF Card
#include <M5Unified.h>

/*
 * Converted using https://rop.nl/truetype2gfx/
 * Had to use C header instead of .VLW because
 * drawing text with the .VLW loaded created
 * extreme lag for some reason.
 */
#include "Pixel128pt7b.h"

/*
 * The display accepts 16-bit colors, use
 * http://www.rinkydinkelectronics.com/calc_rgb565.php
 * to find these codes.
 */
#define BACKGROUND_COLOR    0x9CD3
#define GUI_COLOR           0xFFFF
#define BORDER_COLOR        0x632C
#define SEPERATOR_COLOR     0xCE79

#define CANVAS_WIDTH    64
#define CANVAS_HEIGHT   32

M5Canvas screen(&M5.Display); // Display buffer to reduce flickering
M5Canvas hud(&screen); // HUD on top
M5Canvas toolbar(&screen); // Toolbar on bottom
M5Canvas canvas(&screen); // Drawing canvas

// Store touch info when started touching canvas for panning and shapes
uint8_t ptouched;
int16_t otx;
int16_t oty;
int16_t oviewX;
int16_t oviewY;

// Canvas pan and zoom
int16_t viewX = 0;
int16_t viewY = 0;
uint8_t viewZ = 4;

uint16_t drawColor = M5.Display.color24to16(0x000000);

uint32_t frames = 0; // Frame count

void drawHUD(void);
void drawToolbar(int8_t drawIcons);
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
  cfg.led_brightness = 0; // Power LED

  M5.begin(cfg);

  // Keep trying to initialize SD
  while (SD.begin(GPIO_NUM_4, SPI, 25000000) == false) {
    delay(500);
  }
  
  M5.Display.setBrightness(32);
  M5.Display.begin();

  screen.createSprite(320, 240);
  screen.fillSprite(BACKGROUND_COLOR);
  screen.setTextSize(1);
  screen.setFont(&Pixel128pt7b);
  screen.setTextWrap(false, false);

  hud.createSprite(320 - 16, 16);
  hud.fillSprite(TFT_TRANSPARENT);
  hud.setTextSize(1);
  hud.setFont(&Pixel128pt7b);
  hud.setTextWrap(false, false);

  toolbar.createSprite(320, 38);
  toolbar.fillSprite(GUI_COLOR);
  toolbar.writeFillRect(0, 0, 320, 2, BORDER_COLOR);
  toolbar.writeFillRect(0, 2, 320, 36, GUI_COLOR);
  toolbar.writeFastVLine(41, 10, 20, SEPERATOR_COLOR);
  toolbar.writeFastVLine(196, 10, 20, SEPERATOR_COLOR);
  toolbar.writeFastVLine(279, 10, 20, SEPERATOR_COLOR);

  canvas.createSprite(CANVAS_WIDTH, CANVAS_HEIGHT);
  canvas.fillSprite(TFT_WHITE);

  drawToolbar(-1);
  drawHUD();
  drawScreen(true, true, true);
}

void loop(void) {
  M5.update();

  bool canvasMoved = false;

  lgfx::touch_point_t tp[1];

  uint8_t touched = M5.Display.getTouch(tp, 1);

  if (!ptouched && touched) {
    otx = tp[0].x;
    oty = tp[0].y;
    oviewX = viewX;
    oviewY = viewY;
  }

  if (ptouched && touched) {
    viewX = oviewX + (tp[0].x - otx);
    viewY = oviewY + (tp[0].y - oty);
    canvasMoved = true;
  }

  ptouched = touched;

  if ((frames % 100) == 0) { // Time might have changed
    drawHUD();
    drawScreen(canvasMoved, true, canvasMoved);
  } else {
    drawScreen(canvasMoved, false, canvasMoved);
  }

  M5.Display.waitDisplay();
  screen.pushSprite(0, 0); // Draw screen to screen

  ++frames;
}

void drawHUD(void) {
  hud.fillSprite(TFT_TRANSPARENT);

  char rbuf[32];

  // Make strings for HUD text
  m5::rtc_datetime_t dt;

  if (M5.Rtc.getDateTime(&dt)) {
    if (dt.time.hours >= 12) {
      dt.time.hours -= 12;

      if (dt.time.hours == 0) dt.time.hours = 12;

      snprintf(rbuf, sizeof rbuf, "%d:%02d PM / %d%%", dt.time.hours, dt.time.minutes, M5.Power.getBatteryLevel());
    } else {
      if (dt.time.hours == 0) dt.time.hours = 12;

      snprintf(rbuf, sizeof rbuf, "%d:%02d AM / %d%%", dt.time.hours, dt.time.minutes, M5.Power.getBatteryLevel());
    }
  } else {
    snprintf(rbuf, sizeof rbuf, "ERROR / %d%%", M5.Power.getBatteryLevel());
  }

  // Draw HUD text
  hud.setTextColor(BORDER_COLOR);

  hud.setTextDatum(top_right);
  hud.drawString(rbuf, 320 - 16, 0);
}

void drawToolbar(int8_t drawIcons) {
  /*
   * -1 = all icons
   * 0 = no icons
   * 1 = first icon
   * ...
   */

  if (drawIcons != 0) {
    if ((drawIcons == -1) || (drawIcons == 1)) toolbar.drawBmpFile(SD, "/icons/menu.bmp", 0, 2);
    if ((drawIcons == -1) || (drawIcons == 2)) toolbar.drawBmpFile(SD, "/icons/menu-drawing/pencil.bmp", 48, 2);
    if ((drawIcons == -1) || (drawIcons == 3)) toolbar.drawBmpFile(SD, "/icons/menu-shapes/rectangle.bmp", 84, 2);
    if ((drawIcons == -1) || (drawIcons == 4)) toolbar.drawBmpFile(SD, "/icons/menu-fill/fill.bmp", 120, 2);
    if ((drawIcons == -1) || (drawIcons == 5)) toolbar.drawBmpFile(SD, "/icons/pan.bmp", 156, 2);
    if ((drawIcons == -1) || (drawIcons == 6)) toolbar.drawBmpFile(SD, "/icons/menu-view.bmp", 204, 2);
    if ((drawIcons == -1) || (drawIcons == 7)) toolbar.drawBmpFile(SD, "/icons/grid.bmp", 240, 2);
  }

  // Draw current color preview
  toolbar.fillRoundRect(293, 10, 20, 20, 4, drawColor);
}

void drawScreen(bool whole, bool drawClock, bool drawCanvas) {
  if (whole) {
    drawClock = false;
    drawCanvas = false;

    screen.fillSprite(BACKGROUND_COLOR);
  }
  
  if (drawClock) { // Clear screen behind text
    screen.writeFillRect(8, 8, 320 - 16, 16, BACKGROUND_COLOR);
  }

  // Draw main drawing canvas with outline
  if (drawCanvas || ((viewY <= 24) && drawClock) || whole) {
    canvas.pushRotateZoom(&screen, viewX + ((CANVAS_WIDTH * (uint16_t)viewZ + viewZ) >> 1), viewY + ((CANVAS_HEIGHT * (uint16_t)viewZ + viewZ) >> 1), 0, viewZ, viewZ);
    screen.drawRect(viewX - 1, viewY - 1, CANVAS_WIDTH * viewZ + 2, CANVAS_HEIGHT * viewZ + 2, BORDER_COLOR);
    screen.drawRoundRect(viewX - 2, viewY - 2, CANVAS_WIDTH * viewZ + 4, CANVAS_HEIGHT * viewZ + 4, 2, BORDER_COLOR);
  }

  // Draw HUD on top
  if (drawClock || drawCanvas || whole) {
    hud.pushSprite(&screen, 8, 8, TFT_TRANSPARENT);
  }

  // Draw toolbar on bottom
  toolbar.pushSprite(&screen, 0, 202);
}
