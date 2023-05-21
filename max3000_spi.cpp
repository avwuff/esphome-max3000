#include "esphome/core/log.h"
#include "max3000_spi.h"

namespace esphome {
namespace max3000_spi {

static const char *TAG = "max3000_spi";

// TEMPORARY
#define DISPLAY_HEIGHT 16
#define DISPLAY_WIDTH 28

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


  this->fDots->begin();
  //this->fDots->setDissolveEnable(true);

  this->fDots->clearDisplay();
  this->fDots->display();

  ESP_LOGCONFIG(TAG, "Display Ready");
}


void MAX3000::dump_config(){
    ESP_LOGCONFIG(TAG, "MAX3000 SPI");
}

// no idea what this HOT does
void HOT MAX3000::draw_absolute_pixel_internal(int x, int y, Color color) {
    ESP_LOGCONFIG(TAG, "Draw Pixel");
    this->fDots->drawPixel(x, y, color.is_on());
}

int t = 0;

void MAX3000::update() {
    ESP_LOGCONFIG(TAG, "Update");

    // temp
    this->fDots->drawPixel(t, 0, true);
    t++;

    this->fDots->display();
}

void MAX3000::display() {
    ESP_LOGCONFIG(TAG, "Display");
    this->fDots->display();
}

void MAX3000::fill(Color color) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        for (int y = 0; x < DISPLAY_HEIGHT; x++) {
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


}  // namespace max3000_spi
}  // namespace esphome