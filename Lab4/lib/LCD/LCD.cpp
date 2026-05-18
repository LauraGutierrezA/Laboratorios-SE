#include "LCD.h"

LCD::LCD(I2CMaster *i2c, uint8_t addr)
{
    _i2c = i2c;
    _addr = addr;
}

void LCD::writeByte(uint8_t data)
{
    uint8_t buffer[1];
    buffer[0] = data;

    _i2c->writeRawBytes(_addr, buffer, 1);
}

void LCD::sendNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT | rs;

    writeByte(data | LCD_ENABLE);
    vTaskDelay(pdMS_TO_TICKS(1));

    writeByte(data & ~LCD_ENABLE);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void LCD::sendByte(uint8_t data, uint8_t rs)
{
    sendNibble(data & 0xF0, rs);
    sendNibble((data << 4) & 0xF0, rs);
}

void LCD::command(uint8_t cmd)
{
    sendByte(cmd, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void LCD::data(uint8_t data)
{
    sendByte(data, LCD_RS);
}

void LCD::init()
{
    vTaskDelay(pdMS_TO_TICKS(50));

    sendNibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    sendNibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    sendNibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    sendNibble(0x20, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    command(0x28);
    command(0x0C);
    command(0x06);
    clear();
}