#include "UART.h"

Uart::Uart(uart_port_t port, int txPin, int rxPin){
    _port = port; 
    _txPin = txPin; 
    _rxPin = rxPin; 

    _rxIndex = 0; 
}

void Uart::init(){
    uart_config_t uart_config = {
        .baud_rate = 9600, 
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, 
        .stop_bits = UART_STOP_BITS_1, 
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(_port, &uart_config); 
    uart_set_pin(_port, _txPin, _rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE); 
    uart_driver_install(_port, 1024, 1024, 0, NULL, 0);
}

void Uart::sendUart(const char* mensaje){
    uart_write_bytes(_port, mensaje, strlen(mensaje));
}

bool Uart::recieveUart(const char* format, int* outValue){
    uint8_t data; 
    int len = uart_read_bytes(_port, &data, 1, 0); 

    if (len >0){
        _rxBuffer[_rxIndex] = data; 
        _rxIndex ++; 
        if (_rxIndex >= 63) {
            _rxIndex = 0;
        };

        if (data == '\n' || data == '\r') {
            _rxBuffer[_rxIndex] = '\0'; 

            int tempValue; 
            bool success = false; 

            if (sscanf(_rxBuffer, format, &tempValue) == 1) {
                *outValue = tempValue; 
                success = true;
            }
            _rxIndex = 0; 
            return success; 
        }
    }
    return false; 
}