#ifndef __LCD_H__
#define __LCD_H__

#include <stdint.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_ADDR 0x27 //no sabemos

#define LCD_BACKLIGHT 0x08 //luz de fondo de la pantalla (0000 1000)
#define LCD_ENABLE    0x04 //el enable (0000 01000)
#define LCD_RS        0x01c//RS (Register select 0 comando y 1 caracter/texto)

//declaro la clase
class LCD {
    private:
        i2c_port_t _port; //guarda qu epuerto i2c voy a usar
        uint8_t _addr; //direccion del I2C de la panta;;a

        void writeByte(uint8_t data); //manda un byte completo
        void sendNibble(uint8_t nibble, uint8_t rs); //medio byte, primero se mandan los bytes altos y luego los bajo como lo dice el data sheet
        void sendByte(uint8_t data, uint8_t rs); //manda un byte completo pero internamente lo divide en dos

    public:
        //constructor
        LCD(i2c_port_t port, uint8_t addr = LCD_ADDR);

        void init(); //inicializa
        void command(uint8_t cmd); //manda un comando
        void data(uint8_t data); //manda un caracter a la pantalla
        void clear(); //internamente manda 0x01 = clear disply
        void setCursor(uint8_t fila, uint8_t columna); //ubica cursor en una posicion,ejempo (0, 0) = fila superior, columna 0
        void print(const char *texto); //escribe texto completo
        void mostrarMensaje(const char *lineaSuperior, const char *lineaInferior); //muestra el mensaje
};

#endif
