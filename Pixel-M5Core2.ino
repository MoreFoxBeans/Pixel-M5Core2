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

// Change these to change canvas size
#define CANVAS_WIDTH    64
#define CANVAS_HEIGHT   32

M5Canvas screen(&M5.Display); // Display buffer to reduce flickering
M5Canvas hud(&screen); // HUD on top
M5Canvas toolbar(&screen); // Toolbar on bottom
M5Canvas canvas(&screen); // Drawing canvas

// Previous touch info
uint8_t ptouched;
int16_t ptx;
int16_t pty;

// Store touch info when started touching canvas for panning and shapes
int16_t otx;
int16_t oty;
int16_t oviewX;
int16_t oviewY;

// Canvas pan and zoom
int16_t viewX = 0;
int16_t viewY = 0;
uint8_t viewZ = 4;

bool grid = true; // Show canvas grid

bool panning = false; // Is currently panning?

// Tool menu selected
enum Tool { drawing, shapes, fills, pan };
Tool tool = pan;

// Current drawing color
uint16_t drawColor = M5.Display.color24to16(0x000000);

// Menu sub-item selected
enum DrawingTools { pencil, eraser, eyedropper, dither };
enum ShapeTools { rectangle, line, circle };
enum FillTools { fillNormal, fillDiagonal, fillAll };
DrawingTools drawingTool = pencil;
ShapeTools shapeTool = rectangle;
FillTools fillTool = fillNormal;

uint32_t frames = 0; // Frame count

// Prototypes (match functions!)
void drawHUD(void);
void drawToolbar();
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

  // Create screen buffer
  screen.createSprite(320, 240);
  screen.fillSprite(BACKGROUND_COLOR);
  screen.setTextSize(1);
  screen.setFont(&Pixel128pt7b);
  screen.setTextWrap(false, false);

  // Create HUD text buffer
  hud.createSprite(320 - 16, 16);
  hud.fillSprite(TFT_TRANSPARENT);
  hud.setTextSize(1);
  hud.setFont(&Pixel128pt7b);
  hud.setTextWrap(false, false);

  // Create bottom toolbar buffer
  toolbar.createSprite(320, 38);
  toolbar.fillSprite(GUI_COLOR);

  // Create drawing toolbar buffer
  canvas.createSprite(CANVAS_WIDTH, CANVAS_HEIGHT);
  canvas.fillSprite(TFT_WHITE);

  // Draw everything!
  drawToolbar();
  drawHUD();
  drawScreen(true, true, true);
}

void loop(void) {
  M5.update();

  lgfx::touch_point_t tp[1];

  uint8_t touched = M5.Display.getTouch(tp, 1);

  // Touch pressed
  if (!ptouched && touched) {
    if (tp[0].y < 202) { // Client area tapped
      otx = tp[0].x;
      oty = tp[0].y;
      
      if (tool == pan) {
        panning = true;
        oviewX = viewX;
        oviewY = viewY;
      }
    } else { // Toolbar tapped
      if (tp[0].x < 36) { // Main menu
        
      } else if (tp[0].x < 84) { // Draw menu
        tool = drawing;
      } else if (tp[0].x < 120) { // Shapes menu
        tool = shapes;
      } else if (tp[0].x < 156) { // Fills menu
        tool = fills;
      } else if (tp[0].x < 192) { // Pan
        tool = pan;
      } else if (tp[0].x < 240) { // View menu

      } else if (tp[0].x < 276) { // Grid
        grid = !grid;
      } else { // Color picker

      }

      drawToolbar();
      drawScreen(false, false, false);
    }
  }

  // Touched at all
  if (touched) {
    if (tp[0].y < 202) {
      int16_t canvasPointX = (tp[0].x - viewX) / viewZ;
      int16_t canvasPointY = (tp[0].y - viewY) / viewZ;

      if (tool == drawing) {
        if ((canvasPointX >= 0) && (canvasPointX < CANVAS_WIDTH) && (canvasPointY >= 0) && (canvasPointY < CANVAS_HEIGHT)) {
          if (drawingTool == pencil || drawingTool == eraser) {
            canvas.setColor(drawColor);
            
            if (drawingTool == eraser) {
              canvas.setColor(TFT_WHITE);
            }
            
            if (ptouched) {
              int16_t pcanvasPointX = (ptx - viewX) / viewZ;
              int16_t pcanvasPointY = (pty - viewY) / viewZ;

              canvas.drawLine(pcanvasPointX, pcanvasPointY, canvasPointX, canvasPointY);
            } else {
              canvas.writePixel(canvasPointX, canvasPointY);
            }

            drawScreen(false, false, true);
          }
        }
      } else if (tool == shapes) {

      } else if (tool == fills) {

      }
    }
  }

  // Touch held
  if (ptouched && touched) {
    if ((tool == pan) && panning) {
      viewX = oviewX + (tp[0].x - otx);
      viewY = oviewY + (tp[0].y - oty);
      drawScreen(true, true, true);
    }
  }

  // Touch released
  if (ptouched && !touched) {
    panning = false;
  }

  ptouched = touched;
  ptx = tp[0].x;
  pty = tp[0].y;

  if ((frames % 100) == 0) { // Time has most likely changed
    drawHUD();
    drawScreen(false, true, false);
  }

  M5.Display.waitDisplay(); // IDK what this does but it seems safe :D
  screen.pushSprite(0, 0); // Update the screen
  
  ++frames;
}

void drawHUD(void) {
  hud.fillSprite(TFT_TRANSPARENT);

  char rbuf[32];

  // Make strings for HUD text
  m5::rtc_datetime_t dt;

  if (M5.Rtc.getDateTime(&dt)) {
    // Convert to 12-hour time
    bool pm = false;

    if (dt.time.hours >= 12) {
      pm = true;

      dt.time.hours -= 12;

      if (dt.time.hours == 0) dt.time.hours = 12;
    } else {
      if (dt.time.hours == 0) dt.time.hours = 12;
    }

    snprintf(rbuf, sizeof rbuf, pm ? "%d:%02d PM / %d%%" : "%d:%02d AM / %d%%", dt.time.hours, dt.time.minutes, M5.Power.getBatteryLevel());
  } else { // RTC broken :(
    snprintf(rbuf, sizeof rbuf, "ERROR / %d%%", M5.Power.getBatteryLevel());
  }

  // Draw HUD text
  hud.setTextColor(BORDER_COLOR);
  hud.setTextDatum(top_right);
  hud.drawString(rbuf, 320 - 16, 0);
}

void drawToolbar() {
  // Clear toolbar and draw seperators
  toolbar.fillSprite(GUI_COLOR);
  toolbar.writeFillRect(0, 0, 320, 2, BORDER_COLOR);
  toolbar.writeFastVLine(41, 10, 20, SEPERATOR_COLOR);
  toolbar.writeFastVLine(196, 10, 20, SEPERATOR_COLOR);
  toolbar.writeFastVLine(279, 10, 20, SEPERATOR_COLOR);

  String toolIcon;

  toolbar.drawBmpFile(SD, "/icons/menu.bmp", 0, 2); // Menu icon

  // Drawing menu icon
  switch (drawingTool) {
    case pencil: toolIcon = "/icons/menu-drawing/pencil.bmp"; break;
    case eraser: toolIcon = "/icons/menu-drawing/eraser.bmp"; break;
    case eyedropper: toolIcon = "/icons/menu-drawing/eyedropper.bmp"; break;
    case dither: toolIcon = "/icons/menu-drawing/dither.bmp"; break;
  }

  toolbar.drawBmpFile(SD, toolIcon, 48, 2);

  // Shapes menu icon
  switch (shapeTool) {
    case rectangle: toolIcon = "/icons/menu-shapes/rectangle.bmp"; break;
    case line: toolIcon = "/icons/menu-shapes/line.bmp"; break;
    case circle: toolIcon = "/icons/menu-shapes/circle.bmp"; break;
  }

  toolbar.drawBmpFile(SD, toolIcon, 84, 2);

  // Fills menu icon
  switch (fillTool) {
    case fillNormal: toolIcon = "/icons/menu-fill/fill.bmp"; break;
    case fillDiagonal: toolIcon = "/icons/menu-fill/fill-diagonal.bmp"; break;
    case fillAll: toolIcon = "/icons/menu-fill/fill-all.bmp"; break;
  }

  toolbar.drawBmpFile(SD, toolIcon, 120, 2);

  toolbar.drawBmpFile(SD, "/icons/pan.bmp", 156, 2); // Pan icon
  toolbar.drawBmpFile(SD, "/icons/menu-view.bmp", 204, 2); // View menu icon
  toolbar.drawBmpFile(SD, "/icons/grid.bmp", 240, 2); // Grid icon

  uint16_t outlineX = 320; // If outline is invalid, draw it outside screen.

  switch (tool) {
    case drawing: outlineX = 48; break;
    case shapes: outlineX = 84; break;
    case fills: outlineX = 120; break;
    case pan: outlineX = 156; break;
  }

  toolbar.drawRoundRect(outlineX + 3, 2 + 3, 30, 30, 6, SEPERATOR_COLOR);
  toolbar.drawRoundRect(outlineX + 4, 2 + 4, 28, 28, 5, SEPERATOR_COLOR);
  
  if (grid) {
    toolbar.drawRoundRect(240 + 3, 2 + 3, 30, 30, 6, SEPERATOR_COLOR);
    toolbar.drawRoundRect(240 + 4, 2 + 4, 28, 28, 5, SEPERATOR_COLOR);
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

    if (drawClock || whole) {
      screen.drawRect(viewX - 1, viewY - 1, CANVAS_WIDTH * viewZ + 2, CANVAS_HEIGHT * viewZ + 2, BORDER_COLOR);
      screen.drawRoundRect(viewX - 2, viewY - 2, CANVAS_WIDTH * viewZ + 4, CANVAS_HEIGHT * viewZ + 4, 2, BORDER_COLOR);
    }
  }

  // Draw HUD on top
  if (drawClock || drawCanvas || whole) {
    hud.pushSprite(&screen, 8, 8, TFT_TRANSPARENT);
  }

  // Draw toolbar on bottom
  toolbar.pushSprite(&screen, 0, 202);

  // Draw round corners
  screen.setColor(TFT_BLACK);

  screen.writeFastHLine(0, 0, 5); // Top-left
  screen.writeFastHLine(0, 1, 3);
  screen.writeFastHLine(0, 2, 2);
  screen.writeFastHLine(0, 3, 1);
  screen.writeFastHLine(0, 4, 1);

  screen.writeFastHLine(320 - 5, 0, 5); // Top-right
  screen.writeFastHLine(320 - 3, 1, 3);
  screen.writeFastHLine(320 - 2, 2, 2);
  screen.writeFastHLine(320 - 1, 3, 1);
  screen.writeFastHLine(320 - 1, 4, 1);

  screen.writeFastHLine(0, 240 - 1, 5); // Bottom-left
  screen.writeFastHLine(0, 240 - 2, 3);
  screen.writeFastHLine(0, 240 - 3, 2);
  screen.writeFastHLine(0, 240 - 4, 1);
  screen.writeFastHLine(0, 240 - 5, 1);

  screen.writeFastHLine(320 - 5, 240 - 1, 5); // Bottom-right
  screen.writeFastHLine(320 - 3, 240 - 2, 3);
  screen.writeFastHLine(320 - 2, 240 - 3, 2);
  screen.writeFastHLine(320 - 1, 240 - 4, 1);
  screen.writeFastHLine(320 - 1, 240 - 5, 1);
}
