#include "esphome/core/log.h"
#include "max3000.h"

namespace esphome {
namespace max3000 {


static const char *TAG = "max3000";

#define COLOR_ON Color(255, 255, 255)
#define PANEL_HEIGHT 16
#define PANEL_WIDTH 28

// Constructor
MAX3000::MAX3000(int displaysWide, int displaysHigh) : displaysWide_(displaysWide), displaysHigh_(displaysHigh) {
  dWidth = displaysWide * PANEL_WIDTH;
  dHeight = displaysHigh * PANEL_HEIGHT;

  // Amount of bytes in memory that the buffer needs to be
  int bufferSize = dWidth *((dHeight + 7) / 8);

  // Set up the memory, maybe it will work this time.
  before = new uint8_t[bufferSize];
  after = new uint8_t[bufferSize];
}

void MAX3000::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SPI MAX3000...");

  // Set up the pins
  clk_pin_->setup();
  mosi_pin_->setup();
  col_pin_->setup();
  row_pin_->setup();
  pulse_pin_->setup();
  latch_pin_->setup();
  reset_pin_->setup();

  // Initialize our copy of the display
  fDots = new MAX3000_Display(MAX3000_Config(dWidth, dHeight,
    mosi_pin_, clk_pin_, latch_pin_, reset_pin_,
    pulse_pin_, col_pin_, row_pin_));

  // Set up the memory for the buffers.  If we do it in the Begin function, it crashes.
  ESP_LOGCONFIG(TAG, "Allocating memory for buffer in display");

  // Initialize the display
  fDots->begin();
  fDots->setDissolveEnable(dissolveEnabled);

  // Clear it and get it ready
  fDots->clearDisplay();
  fDots->display();

  ESP_LOGCONFIG(TAG, "Display Ready");
}

void MAX3000::set_dissolve(bool dissolve) {
  dissolveEnabled = dissolve;
}

void MAX3000::dump_config(){
    ESP_LOGCONFIG(TAG, "MAX3000 SPI");
}

// no idea what this HOT does
void HOT MAX3000::draw_absolute_pixel_internal(int x, int y, Color color) {
    fDots->drawPixel(x, y, color.is_on());
}

void MAX3000::update() {

    // If transition is turned on...
    if (nextTransition > 0) {
      // Copy the current look into a buffer, this will be the 'before' of the transition.
      fDots->copyBuffer(before);
    }

    // Without this one do_update call, NONE of the ESPHome drawing functions work.
    // This seems to be what causes ESPHome to actually issue all the draw calls to the draw_absolute_pixel above.
    do_update_();

    if (nextTransition > 0) {
        // Copy the after into a buffer
        fDots->copyBuffer(after);

        doTransition(nextTransition);
        nextTransition = 0;
    }

    // Send the pixels to the display
    fDots->display();
}

void MAX3000::transitionOnNextUpdate(int transition) {
    nextTransition = transition;
}

void MAX3000::doTransition(int transition) {

    // Put the BEFORE back on the display.
    fDots->replaceBuffer(before);

    switch (transition) {
      case 1: // Transition 1: A simple horizontal wipe from left to right
        for (int x = 0; x < dWidth; x++) {

          // Draw the line top to bottom
          for (int y = 0; y < dHeight; y++) {
            // double thickness to make it more obvious
            fDots->drawPixel(x, y, MAX3000_LIGHT);
            fDots->drawPixel(x+1, y, MAX3000_LIGHT);

            // Draw the 'after' in the row behind the line
            if (x > 0) {
              fDots->drawPixel(x - 1, y, getPixel(after, x-1, y) ? MAX3000_LIGHT : MAX3000_DARK);
            }
          }

          // Draw it as fast as possible.
          fDots->display();
          delay(5);
        }

        break;
      case 2: // Transition 2: A diagonal swipe.
        for (int x = 0; x < dWidth + dHeight; x++) {

          // Draw the line top to bottom
          for (int y = 0; y < dHeight; y++) {
            // double thickness to make it more obvious
            fDots->drawPixel(x - y, y, MAX3000_LIGHT);
            fDots->drawPixel(x - y + 1, y, MAX3000_LIGHT);

            // Draw the 'after' in the row behind the line
            if (x - y - 1 >= 0 && x - y - 1 < dWidth) {
              fDots->drawPixel(x - y - 1, y, getPixel(after, x-y-1, y) ? MAX3000_LIGHT : MAX3000_DARK);
            }
          }

          // Draw it as fast as possible.
          fDots->display();
          delay(5);
        }

        break;
    }
}

bool MAX3000::getPixel(uint8_t *buffer, int16_t x, int16_t y) {
    if((x >= 0) && (x < dWidth) && (y >= 0) && (y < dHeight)) {
        return buffer[x + (y / 8) * dWidth] & (1 << (y & 7));
    }
    return false;    // Pixel out of bounds
}

void MAX3000::fill(Color color) {
    // Fill it with one color
    for (int x = 0; x < dWidth; x++) {
        for (int y = 0; y < dHeight; y++) {
            fDots->drawPixel(x, y, color.is_on());
        }
    }
}

int MAX3000::get_height_internal() {
    return dHeight;
}
int MAX3000::get_width_internal() {
    return dWidth;
}


}  // namespace max3000
}  // namespace esphome