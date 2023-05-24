import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES
)
# Pin names
CLK_PIN = "clk_pin"
MOSI_PIN = "mosi_pin"
COL_PIN = "col_pin"
ROW_PIN = "row_pin"
PULSE_PIN = "pulse_pin"
LATCH_PIN = "latch_pin"
RESET_PIN = "reset_pin"

# Other options
CONF_DISSOLVE = "dissolve"

max3000_ns = cg.esphome_ns.namespace('max3000')
MAX3000 = max3000_ns.class_('MAX3000', cg.Component, display.DisplayBuffer)

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(MAX3000),
            cv.Required(CLK_PIN): pins.gpio_output_pin_schema,
            cv.Required(MOSI_PIN): pins.gpio_output_pin_schema,
            cv.Required(COL_PIN): pins.gpio_output_pin_schema,
            cv.Required(ROW_PIN): pins.gpio_output_pin_schema,
            cv.Required(PULSE_PIN): pins.gpio_output_pin_schema,
            cv.Required(LATCH_PIN): pins.gpio_output_pin_schema,
            cv.Required(RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_DISSOLVE, default=True): cv.boolean,
        }
    ).extend(cv.polling_component_schema("1s")),
    cv.has_at_most_one_key(CONF_PAGES, CONF_LAMBDA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await display.register_display(var, config)

    # Apply the pin values to the C++ code
    pin = await cg.gpio_pin_expression(config[CLK_PIN])
    cg.add(var.set_clk_pin(pin))
    pin = await cg.gpio_pin_expression(config[MOSI_PIN])
    cg.add(var.set_mosi_pin(pin))
    pin = await cg.gpio_pin_expression(config[COL_PIN])
    cg.add(var.set_col_pin(pin))
    pin = await cg.gpio_pin_expression(config[ROW_PIN])
    cg.add(var.set_row_pin(pin))
    pin = await cg.gpio_pin_expression(config[PULSE_PIN])
    cg.add(var.set_pulse_pin(pin))
    pin = await cg.gpio_pin_expression(config[LATCH_PIN])
    cg.add(var.set_latch_pin(pin))
    pin = await cg.gpio_pin_expression(config[RESET_PIN])
    cg.add(var.set_reset_pin(pin))

    # Other optional settings
    cg.add(var.set_dissolve(config[CONF_DISSOLVE]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayBufferRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
