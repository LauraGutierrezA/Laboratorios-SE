#include "LCD.h"

LCD::LCD(i2c_port_t port, uint8_t addr)
{
    _port = port;
    _addr = addr;
}

void LCD::writeByte(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); //crea la transaccion

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
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

    command(0x28); // 4 bits, 2 líneas, fuente 5x8
    command(0x0C); // display ON, cursor OFF
    command(0x06); // cursor incrementa hacia la derecha
    clear();
}

void LCD::clear()
{
    command(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void LCD::setCursor(uint8_t fila, uint8_t columna)
{
    if (fila == 0) {
        command(0x80 + columna);
    } else {
        command(0xC0 + columna);
    }
}

void LCD::print(const char *texto)
{
    while (*texto) {
        data(*texto);
        texto++;
    }
}

void LCD::mostrarMensaje(const char *lineaSuperior, const char *lineaInferior)
{
    clear();

    setCursor(0, 0);
    print(lineaSuperior);

    setCursor(1, 0);
    print(lineaInferior);
}