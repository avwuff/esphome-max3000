#include "esphome/core/log.h"
#include "max3000.h"

namespace esphome {
namespace max3000 {


static const char *TAG = "max3000";

#define COLOR_ON Color(255, 255, 255)

void MAX3000::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SPI MAX3000...");

  // Set up the pins
  this->clk_pin_->setup();
  this->mosi_pin_->setup();
  this->col_pin_->setup();
  this->row_pin_->setup();
  this->pulse_pin_->setup();
  this->latch_pin_->setup();
  this->reset_pin_->setup();

  // Initialize our copy of the display
  this->fDots = new MAX3000_Display(MAX3000_Config(DISPLAY_WIDTH, DISPLAY_HEIGHT,
      this->mosi_pin_, this->clk_pin_, this->latch_pin_, this->reset_pin_,
      this->pulse_pin_, this->col_pin_, this->row_pin_));

  // Initialize the display
  this->fDots->begin();
  this->fDots->setDissolveEnable(this->dissolveEnabled);

  // Clear it and get it ready
  this->fDots->clearDisplay();
  this->fDots->display();

  ESP_LOGCONFIG(TAG, "Display Ready");
}

void MAX3000::set_dissolve(bool dissolve) {
  this->dissolveEnabled = dissolve;
}

void MAX3000::dump_config(){
    ESP_LOGCONFIG(TAG, "MAX3000 SPI");
}

// no idea what this HOT does
void HOT MAX3000::draw_absolute_pixel_internal(int x, int y, Color color) {
    this->fDots->drawPixel(x, y, color.is_on());
}

void MAX3000::update() {

    // If transition is turned on...
    if (nextTransition > 0) {
      // Copy the current look into a buffer, this will be the 'before' of the transition.
      // TODO: Use a faster way of doing this
      for (int x = 0; x < DISPLAY_WIDTH; x++) {
          for (int y = 0; y < DISPLAY_HEIGHT; y++) {
              before[x][y] = this-fDots->getPixel(x, y);
          }
      }
    }

    // Without this one do_update call, NONE of the ESPHome drawing functions work.
    // This seems to be what causes ESPHome to actually issue all the draw calls to the draw_absolute_pixel above.
    this->do_update_();

    if (nextTransition > 0) {
        // Copy the after into a buffer
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            for (int y = 0; y < DISPLAY_HEIGHT; y++) {
                after[x][y] = this-fDots->getPixel(x, y);
            }
        }

        doTransition(nextTransition);

        nextTransition = 0;
    }

    // Send the pixels to the display
    this->fDots->display();
}

void MAX3000::transitionOnNextUpdate(int transition) {
    this->nextTransition = transition;
}

void MAX3000::doTransition(int transition) {

    // Put the BEFORE back on the display.
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            this->fDots->drawPixel(x, y, before[x][y] ? MAX3000_LIGHT : MAX3000_DARK);
        }
    }

    switch (transition) {
      case 1: // Transition 1: A simple horizontal wipe.
        for (int x = 0; x < DISPLAY_WIDTH; x++) {

          // Draw the line top to bottom
          for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            this->fDots->drawPixel(x, y, true);

            // Draw the 'after' in the row behind the line
            if (x > 0) {
              this->fDots->drawPixel(x - 1, y, after[x-1][y] ? MAX3000_LIGHT : MAX3000_DARK);
            }
          }

          // Draw it as fast as possible.
          this->fDots->display();
          delay(15);
        }

        break;
    }

}

void MAX3000::fill(Color color) {
    // Fill it with one color
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            this->fDots->drawPixel(x, y, color.is_on());
        }
    }
}

int MAX3000::get_height_internal() {
    // TODO
    return DISPLAY_HEIGHT;
}
int MAX3000::get_width_internal() {
    // TODO
    return DISPLAY_WIDTH;
}


}  // namespace max3000
}  // namespace esphome