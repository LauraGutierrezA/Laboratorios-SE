/*
ESTO EN EL MAIN SE HARÍA ASÍ: 
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "BleNus.h"

// Creamos el objeto Bluetooth
BleNus miBluetooth;

extern "C" void app_main() {
    
    // 1. Inicializamos con el nombre
    miBluetooth.init("ESP32_LAURA");

    char bufferRecepcion[64];

    while (1) {
        // --- EJEMPLO DE LECTURA ---
        // Preguntamos si llegó algo nuevo al Bluetooth
        if (miBluetooth.leer(bufferRecepcion)) {
            printf("El celular me mandó: %s\n", bufferRecepcion);
        }

        // --- EJEMPLO DE ESCRITURA ---
        // Si hay alguien conectado, enviamos telemetría
        if (miBluetooth.estaConectado()) {
            miBluetooth.enviar("Hola desde Mechatronics!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

*/


#ifndef __BLE_NUS_H__
#define __BLE_NUS_H__

#include <stdint.h>
#include <string.h>

class BleNus {
    private:
        bool _conectado;
        bool _notificacionesActivas;
        
        // Variables para guardar lo que leemos (Read)
        char _ultimoMensaje[64];
        bool _hayDatoNuevo;

        // Permitimos que la función de C de NimBLE pueda modificar nuestras variables privadas
        friend int nus_rx_access_cb(uint16_t conn_handle, uint16_t attr_handle, 
                                    struct ble_gatt_access_ctxt *ctxt, void *arg);
        friend int ble_gap_event_cb(struct ble_gap_event *event, void *arg);

    public:
        BleNus();
        ~BleNus();

        // Inicializa el Bluetooth con el nombre que quieras
        void init(const char* nombreDispositivo);
        
        // Escribe/Envía datos por Bluetooth
        void enviar(const char* mensaje);
        
        // Lee los datos recibidos. Devuelve true si llegó algo nuevo.
        bool leer(char* buffer);
        
        // Función útil para saber si el celular está conectado
        bool estaConectado();
};

#endif