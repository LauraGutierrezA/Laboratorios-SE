#ifndef __RFID_H__
#define __RFID_H__

#include "SPI.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

class Rfid {
    private: 
        SPI _rfid;
        uint8_t _uid[8];
        uint8_t _longitud;

        void buscarTarjeta();
        bool procesarTarjeta();

    public: 
        Rfid(SPI rfid);
        ~Rfid();
        void init(); 
        bool leerTarjeta();
        bool verificarTarjeta(const uint8_t* uidCard);

        uint8_t getLongitud();
        uint8_t getByteUID(int indice);

};


#endif