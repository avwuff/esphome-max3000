/**
 * @file MAX3000_Lib.cpp
 *
 * Library used to control the output of a Luminator MAX3000 Display.
 *
 * For use with https://github.com/NietoSkunk/FlippyDriver MAX3000 Driver.
 *
 * Uses either hardware or software SPI to load data into the shift registers
 * of the driver, as well as I2C to set PWM and read temperature.
 *
 * Derived from Adafruit_SSD1306: https://github.com/adafruit/Adafruit_SSD1306
 * Originally licensed under BSD License, with the following text:
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * BSD license, all text above, and the splash screen header file,
 * must be included in any redistribution.
 */

#include "MAX3000_Lib.h"

static const char *const TAG = "max3000_base";


namespace esphome {
namespace max3000 {

// Extra delay when bitbanging on ESP32, which updates much faster than AVR
#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_STM32)
#define BITBANG_DELAY delayMicroseconds(3);
#else
#define BITBANG_DELAY
#endif

#define MAX3000_swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))
#define LOAD_SR(_b, _p, _e)     \
    shiftReg[_b] &= ~(1 << _p); \
    shiftReg[_b] |= ((_e ? 1 : 0) << _p);

#define MAX3000_LATCH config.lat_pin->digital_write(1);          ///< Shift Register Latch
#define MAX3000_UNLATCH config.lat_pin->digital_write(0);         ///< Shift Register Unlatch
#define MAX3000_PULSE config.pulse_pin->digital_write(1);        ///< PULSE_ENABLE active
#define MAX3000_UNPULSE config.pulse_pin->digital_write(0);       ///< PULSE_ENABLE inactive
#define MAX3000_PULSE_ROW config.row_pin->digital_write(0);       ///< ROW_ENABLE_N active
#define MAX3000_UNPULSE_ROW config.row_pin->digital_write(1);    ///< ROW_ENABLE_N inactive
#define MAX3000_PULSE_COL config.col_pin->digital_write(0);       ///< COL_ENABLE_N active
#define MAX3000_UNPULSE_COL config.col_pin->digital_write(1);    ///< COL_ENABLE_N inactive

// Shift Register bit definitions on each driver
#define SR_PIN_COL_A2 0
#define SR_PIN_COL_A1 1
#define SR_PIN_COL_A0 2
#define SR_PIN_ROW_A0 3
#define SR_PIN_ROW_A1 4
#define SR_PIN_ROW_A2 5
#define SR_PIN_ROW_BANK 6
#define SR_PIN_COL_BANK0 7
#define SR_PIN_COL_SOURCE 8
#define SR_PIN_ROW_SOURCE 10
#define SR_PIN_COL_BANK1 11
#define SR_PIN_USER_LED 13

MAX3000_Base::MAX3000_Base(const MAX3000_Config & config_)
    : config(config_) {
    localWidth          = config.width;
    localHeight         = config.height;
    invertEnabled   = false;
    dissolveEnabled = false;
    constantRate    = false;
    firstUpdate     = true;

    // 250uS has been determined to be a decent compromise between frame rate and flip reliability
    pulseDuration = 150;
}

MAX3000_Base::~MAX3000_Base(void) {

}

inline void
MAX3000_Base::shiftRegWrite() {
    // Push each board through the chain, starting with the last board
    for(size_t board = 0; board < config.numHBoards * config.numVBoards; ++board) {
        for(uint16_t bit = 0x8000; bit; bit >>= 1) {
            config.mosi_pin->digital_write((bool)(shiftReg[board] & bit));
            BITBANG_DELAY
            config.sclk_pin->digital_write(1);
            BITBANG_DELAY
            config.sclk_pin->digital_write(0);
            BITBANG_DELAY
        }
    }

    // Once shift register buffer has been shifted in, latch the output pins.
    MAX3000_LATCH
    MAX3000_UNLATCH
}

bool MAX3000_Base::begin(bool reset, bool periphBegin) {
    char copy[100];
    sprintf(copy, "[BEGIN] shifreg: %d", int(config.numVBoards * config.numHBoards));
    ESP_LOGCONFIG(TAG, copy);

    // Set up the memory
    buffer = new uint8_t[BUFFER_SIZE];
    oldBuffer = new uint8_t[BUFFER_SIZE];
    shiftReg = new uint16_t[config.numVBoards * config.numHBoards];


    memset(buffer, 0, BUFFER_SIZE);
    memset(oldBuffer, 0, BUFFER_SIZE);

    //ESP_LOGCONFIG(TAG, "[BEGIN] Shuffle");
    // Create initial index buffer, which will get shuffled on the first load.
    // Each panel received the same shuffled index for space concerns.
    for(int i = 0; i < PANEL_HEIGHT * PANEL_WIDTH; i++) {
        shuffledIndex[i] = i;
    }
    shuffleIndex();

    memset(shiftReg, 0, config.numVBoards * config.numHBoards * sizeof(uint16_t));

    // Initialize SPI (either hardware or software)
    config.sclk_pin->digital_write(0);


    // Set initial non-pulse state
    MAX3000_UNPULSE
    MAX3000_UNPULSE_ROW
    MAX3000_UNPULSE_COL

    // Reset MAX3000 if requested and reset pin specified in constructor
    if(reset) {
        config.rst_pin->digital_write(1);
        delay(1);
        config.rst_pin->digital_write(0);
        delay(10);
        config.rst_pin->digital_write(1);
        delay(5);
    }
    return true;
}

void MAX3000_Base::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if((x >= 0) && (x < localWidth) && (y >= 0) && (y < localHeight)) {
        // Pixel is in-bounds. Rotate coordinates if needed.
        switch(localRotation) {
            case 1:
                MAX3000_swap(x, y);
                x = config.width - x - 1;
                break;
            case 2:
                x = config.width - x - 1;
                y = config.height - y - 1;
                break;
            case 3:
                MAX3000_swap(x, y);
                y = config.height - y - 1;
                break;
        }

        switch(color) {
            case MAX3000_LIGHT:
                buffer[x + (y / 8) * config.width] |= (1 << (y & 7));
                break;
            case MAX3000_DARK:
                buffer[x + (y / 8) * config.width] &= ~(1 << (y & 7));
                break;
            case MAX3000_INVERSE:
                buffer[x + (y / 8) * config.width] ^= (1 << (y & 7));
                break;
        }
    }
}

void MAX3000_Base::clearDisplay(void) {
    memset(buffer, 0, BUFFER_SIZE);
}

bool MAX3000_Base::getPixel(int16_t x, int16_t y) {
    if((x >= 0) && (x < localWidth) && (y >= 0) && (y < localHeight)) {
        // Pixel is in-bounds. Rotate coordinates if needed.
        switch(localRotation) {
            case 1:
                MAX3000_swap(x, y);
                x = config.width - x - 1;
                break;
            case 2:
                x = config.width - x - 1;
                y = config.height - y - 1;
                break;
            case 3:
                MAX3000_swap(x, y);
                y = config.height - y - 1;
                break;
        }
        return buffer[x + (y / 8) * config.width] & (1 << (y & 7));
    }
    return false;    // Pixel out of bounds
}

void MAX3000_Base::setUserLED(size_t board, bool state) {
    LOAD_SR(board, SR_PIN_USER_LED, state);
    shiftRegWrite();
}

void MAX3000_Base::display(bool force) {
    int numChanged = 0;

    for(int i = 0; i < PANEL_HEIGHT * PANEL_WIDTH; ++i) {
#if defined(ESP8266)
        if(i == 0) {
            yield();
        }
#endif

        // If dissolving, pick the shuffled index
        int index = (dissolveEnabled) ? shuffledIndex[i] : i;

        size_t col = (index / PANEL_HEIGHT);
        size_t row = (index % PANEL_HEIGHT);

        bool pixelsToSet[config.numHBoards * config.numVBoards];
        bool pixelsToClear[config.numHBoards * config.numVBoards];
        memset(pixelsToSet, 0, config.numHBoards * config.numVBoards * sizeof(bool));
        memset(pixelsToClear, 0, config.numHBoards * config.numVBoards * sizeof(bool));

        // First pass: Get the set/clear
        // Since all boards share the same pulse lines, we have to figure out the
        // required operation for every board first.
        // Thus we just store what we want to do in an array, and then actually load
        // and pulse once we've checked every board.
        for(size_t board = 0; board < config.numHBoards * config.numVBoards; ++board) {
            size_t boardCol     = board % config.numHBoards;
            size_t bufferOffset = (col + boardCol * PANEL_WIDTH) + ((row / 8) * config.numHBoards * PANEL_WIDTH);

            bool newPixVal = buffer[bufferOffset] & (1 << (row & 7));
            bool oldPixVal = oldBuffer[bufferOffset] & (1 << (row & 7));

            // Check if we can skip the update.
            // If it's the first update, we need to refresh everything.
            if(!force && !firstUpdate && newPixVal == oldPixVal) {
                continue;
            }

            // Pre-select the decoder inputs for each board now.
            selectRowColumn(board, row, col);

            bool setPixel        = (newPixVal != invertEnabled);
            pixelsToSet[board]   = setPixel;
            pixelsToClear[board] = !setPixel;
            numChanged++;
        }

        // Second Pass: Turn on pixels that need to be set
        // If no change is necessary for a board, neither row or column will
        // be sourced and the pixel will remain in its existing state
        bool setChanged = false;
        for(size_t board = 0; board < config.numHBoards * config.numVBoards; ++board) {
            // Setting -> Row Set Source, Column sink
            LOAD_SR(board, SR_PIN_COL_SOURCE, 0);
            LOAD_SR(board, SR_PIN_ROW_SOURCE, pixelsToSet[board]);
            if(pixelsToSet[board]) setChanged = true;
        }
        if(setChanged) {
            shiftRegWrite();
            setPixel();
        }

        // Second Pass: Turn off pixels that need to be cleared
        // If no change is necessary for a board, neither row or column will
        // be sourced and the pixel will remain in its existing state
        bool resetChanged = false;
        for(size_t board = 0; board < config.numHBoards * config.numVBoards; ++board) {
            // Setting -> Row Set Source, Column sink
            LOAD_SR(board, SR_PIN_COL_SOURCE, pixelsToClear[board]);
            LOAD_SR(board, SR_PIN_ROW_SOURCE, 0);
            if(pixelsToClear[board]) resetChanged = true;
        }
        if(resetChanged) {
            shiftRegWrite();
            clearPixel();
        }
    }

    // Store current buffer to avoid unnecessary changes on next update.
    memcpy(oldBuffer, buffer, BUFFER_SIZE);
    firstUpdate = false;

    if(constantRate) {
        for(int i = numChanged; i < PANEL_HEIGHT * PANEL_WIDTH; ++i) {
#if defined(ESP8266)
            yield();
#endif
            shiftRegWrite();
            delayMicroseconds(5);
            delayMicroseconds(pulseDuration);
            delayMicroseconds(5);
        }
    }
}

void MAX3000_Base::copyBuffer(uint8_t *toBuffer) {
  memcpy(toBuffer, buffer, BUFFER_SIZE);
}

void MAX3000_Base::replaceBuffer(uint8_t *fromBuffer) {
  memcpy(buffer, fromBuffer, BUFFER_SIZE);
}

void MAX3000_Base::invertDisplay(bool i) {
    invertEnabled = i;

    // Send an immediate update. We didn't change the actual buffer so we have to force an update.
    display(true);
}

void MAX3000_Base::setDisplayRotation(uint8_t x) {
    localRotation = (x & 3);
    switch(localRotation) {
        case 0:
        case 2:
            localWidth  = config.width;
            localHeight = config.height;
            break;
        case 1:
        case 3:
            localWidth  = config.height;
            localHeight = config.width;
            break;
    }
}

void MAX3000_Base::setDissolveEnable(bool param) {
    dissolveEnabled = param;
}

void MAX3000_Base::setPulseDurationUs(uint16_t param) {
    pulseDuration = param;
}

void MAX3000_Base::setConstantFrameRate(bool param) {
    constantRate = param;
}

void MAX3000_Base::selectRowColumn(size_t board, size_t row, size_t column) {    // TODO Board Order
    // Map sequential rows and columns to the hardware pins
    const uint8_t colToCode[] = { 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12,
        15, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 };
    const uint8_t rowToCode[] = { 14, 1, 15, 0, 12, 3, 13, 2, 10, 5, 11, 4, 8, 7, 9, 6 };

    // Hack - Rows are reversed on the panel
    row = 15 - row;

    uint8_t rowSubCode = rowToCode[row] % 8;
    uint8_t rowBank    = rowToCode[row] / 8;

    uint8_t colSubCode = colToCode[column] % 8;
    uint8_t colBank    = colToCode[column] / 8;

    LOAD_SR(board, SR_PIN_COL_A2, colSubCode & 0x4);
    LOAD_SR(board, SR_PIN_COL_A1, colSubCode & 0x2);
    LOAD_SR(board, SR_PIN_COL_A0, colSubCode & 0x1);
    LOAD_SR(board, SR_PIN_ROW_A2, rowSubCode & 0x4);
    LOAD_SR(board, SR_PIN_ROW_A1, rowSubCode & 0x2);
    LOAD_SR(board, SR_PIN_ROW_A0, rowSubCode & 0x1);
    LOAD_SR(board, SR_PIN_ROW_BANK, rowBank & 0x1);
    LOAD_SR(board, SR_PIN_COL_BANK1, colBank & 0x2);
    LOAD_SR(board, SR_PIN_COL_BANK0, colBank & 0x1);
}

void MAX3000_Base::setPixel() {
    // Global Pulse Enable
    MAX3000_PULSE

    // Turn on Source first, then Sink
    MAX3000_PULSE_ROW
    delayMicroseconds(5);
    MAX3000_PULSE_COL

    // Wait for Pulse Duration
    delayMicroseconds(pulseDuration);

    // Turn off Sink first, then Source
    MAX3000_UNPULSE_COL
    delayMicroseconds(5);
    MAX3000_UNPULSE_ROW

    // Global Pulse Disable
    MAX3000_UNPULSE
}

void MAX3000_Base::clearPixel() {
    // Global Pulse Enable
    MAX3000_PULSE

    // Turn on Source first, then Sink
    MAX3000_PULSE_COL
    delayMicroseconds(5);
    MAX3000_PULSE_ROW

    // Wait for Pulse Duration
    delayMicroseconds(pulseDuration);

    // Turn off Sink first, then Source
    MAX3000_UNPULSE_ROW
    delayMicroseconds(5);
    MAX3000_UNPULSE_COL

    // Global Pulse Disable
    MAX3000_UNPULSE
}

void MAX3000_Base::shuffleIndex() {
    for(int i = 0; i < (int)(PANEL_HEIGHT * PANEL_WIDTH); i++) {
        int n            = rand() % (PANEL_HEIGHT * PANEL_WIDTH);
        int temp         = shuffledIndex[n];
        shuffledIndex[n] = shuffledIndex[i];
        shuffledIndex[i] = temp;
    }
}

}  // namespace max3000
}  // namespace esphome