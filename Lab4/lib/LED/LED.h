// Aquí van las firmas o prototipos de las funciones. Es si recibe parámetros, cómo se llaman y qué retornan

#ifndef __LED_H__
#define __LED_H_

#include <stdint.h>

class Led {
    public: 
        Led(uint8_t gpio); //aquí el gpio es un atributo global de la clase
        ~Led();
        void init(void); //no necesita entrada porque ya el atributo lo tiene
        void on(void); 
        void off(void); 
        void toggle(uint8_t freq_hz); 
        void breathe(uint16_t speed);
    private: 
        uint8_t _pin; // toma lo que le pasa el constructor o sea el uin8_t gpio
    protected: 
    //No lo vamos a utilizar
};

#endif // __LED_H__