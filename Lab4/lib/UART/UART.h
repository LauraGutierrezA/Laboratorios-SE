#ifndef __UART_H__
#define __UART_H__

#include "driver/uart.h"
#include <string.h>
#include <stdio.h>

class Uart {
    private: 
        uart_port_t _port; 
        int _txPin; 
        int _rxPin; 

        char _rxBuffer[64]; 
        int _rxIndex;
    public: 
        Uart(uart_port_t port, int txPin, int rxPin); 
        void init(); 
        void sendUart(const char* mensaje);
        bool recieveUart(const char* format, int* outvalue); 

}; 

#endif