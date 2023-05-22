#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/display/display_buffer.h"
#include "MAX3000_Lib.h"

// TEMPORARY - FIGURE OUT HOW TO MAKE THESE DYNAMIC
#define DISPLAY_HEIGHT 16
#define DISPLAY_WIDTH 28

namespace esphome {
namespace max3000 {

class MAX3000 : public PollingComponent, public display::DisplayBuffer {
 public:
  void setup() override;
  void dump_config() override;

  void update() override;
  void fill(Color color) override;

  // Cause the next update to be drawn with a transition first
  void transitionOnNextUpdate(int transition);

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  // Pin set functions called by the display.py file
  void set_clk_pin(GPIOPin *clk_pin) { this->clk_pin_ = clk_pin; }
  void set_mosi_pin(GPIOPin *mosi_pin) { this->mosi_pin_ = mosi_pin; }
  void set_col_pin(GPIOPin *col_pin) { this->col_pin_ = col_pin; }
  void set_row_pin(GPIOPin *row_pin) { this->row_pin_ = row_pin; }
  void set_pulse_pin(GPIOPin *pulse_pin) { this->pulse_pin_ = pulse_pin; }
  void set_latch_pin(GPIOPin *latch_pin) { this->latch_pin_ = latch_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }

  // Other optional functions
  void set_dissolve(bool dissolve);

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

 protected:
  // required for the display buffer
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_height_internal() override;
  int get_width_internal() override;

  MAX3000_Display *fDots;

  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *col_pin_{nullptr};
  GPIOPin *row_pin_{nullptr};
  GPIOPin *pulse_pin_{nullptr};
  GPIOPin *latch_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};

  // Outputs for the pins
  void write_clk(bool state) { this->clk_pin_->digital_write(state); }
  void write_mosi(bool state) { this->mosi_pin_->digital_write(state); }
  void write_col(bool state) { this->col_pin_->digital_write(state); }
  void write_row(bool state) { this->row_pin_->digital_write(state); }
  void write_pulse(bool state) { this->pulse_pin_->digital_write(state); }
  void write_latch(bool state) { this->latch_pin_->digital_write(state); }
  void write_reset(bool state) { this->reset_pin_->digital_write(state); }

  bool dissolveEnabled;

  // Transition system
  int nextTransition;
  bool before[DISPLAY_WIDTH][DISPLAY_HEIGHT];
  bool after[DISPLAY_WIDTH][DISPLAY_HEIGHT];
  void doTransition(int transition);

};




}  // namespace max3000
}  // namespace esphome