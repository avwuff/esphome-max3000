# ESPHome MAX3000 Flipdot Graphics Library

Provides support for driving a MAX3000 flipdot board with ESPHome.
Based heavily on the FlippyDriver code here: https://github.com/NietoSkunk/FlippyControl

# Introduction

The [Luminator MAX3000](https://github.com/NietoSkunk/FlippyDriver) is an awesome flipdot display created by Canadian 
company F-P Electronics in the late 90s.  Before LED displays were commonplace, the flipdot display reigned supreme,
as an easy-to-read and versatile display technology.

If you have access to one of these displays, and Nieto's driver board, this software package makes it easy to 
integrate your MAX3000 into your Home Assistant ESPHome infrastructure, allowing you to control it easily.

_This project is a work-in-progress and there are many improvements to be made._

# Features
- Full support for ESP monochrome [Display](https://esphome.io/components/display/index.html) component.
- Adjustable refresh rate
- Optional transitions can provide cool effects when you go between pages
- Ability to dissolve rather than 'wipe' pixels
- Support for multiple linked panels (_untested_)

# How to use
The component must be installed into your Home Assistant / ESPHome installation directory.
1. Using SSH or whichever tool you are familiar with, access your ESPHome folder.
It will contain a `.yaml` file for each of your ESP devices.
2. Create a directory called `custom_components`
3. Inside this directory, create another one called `max3000`.
4. Place all the files from this repository's `src` directory directly into that folder.
5. Create a new ESP project, or update an existing one.
6. Add a `display` component.  Here is the sample configuration for an ESP32:
```yaml
display:
  - platform: max3000
    id: maxsign
    clk_pin: GPIO18 # Listed as SCK
    mosi_pin: GPIO23
    col_pin: GPIO26
    row_pin: GPIO27
    pulse_pin: GPIO33
    reset_pin: GPIO17
    latch_pin: GPIO16
    update_interval: 25ms
    dissolve: false # dissolve transition when updating display
    num_width: 1 # one display wide
    num_height: 1 # one display high
```
The display is now available for use by any of the standard Graphics commands described in the 
ESPHome [Display](https://esphome.io/components/display/index.html) documentation.

# Examples
- [Clock](examples/clock.yaml) A simple 12-hour clock
- [Animation](examples/animated_gif.yaml) Animated GIF playback
- [Text Scroll](examples/scrolling_text.yaml) Clock and scrolling text