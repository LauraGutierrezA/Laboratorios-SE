#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BTN_PIN 34

// Mapeo actualizado según tu último mensaje
const uint8_t PINES_FILAS[8] = {13, 12, 14, 27, 15, 2, 0, 4};
const uint8_t PINES_COLS_ROJAS[8] = {26, 25, 33, 32, 3, 1, 22, 23};

void app_main() {
    // 1. Configurar botón (Entrada pura)
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // No tiene interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_cfg);

    // 2. Configurar Matriz
    for(int i = 0; i < 8; i++) {
        gpio_reset_pin(PINES_FILAS[i]);
        gpio_set_direction(PINES_FILAS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(PINES_FILAS[i], 0);

        gpio_reset_pin(PINES_COLS_ROJAS[i]);
        gpio_set_direction(PINES_COLS_ROJAS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(PINES_COLS_ROJAS[i], 0); // Empezamos apagados
    }

    bool matriz_encendida = false;
    bool ultimo_estado_btn = true; // Asumiendo Pull-up externo

    while(1) {
        int estado_btn = gpio_get_level(BTN_PIN);

        // Lógica de detección de pulso (flanco de bajada)
        if (ultimo_estado_btn == 1 && estado_btn == 0) {
            matriz_encendida = !matriz_encendida; // Cambiamos el estado
            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce simple
        }
        ultimo_estado_btn = estado_btn;

        if (matriz_encendida) {
            // Abrimos los transistores de las columnas
            for(int i=0; i<8; i++) gpio_set_level(PINES_COLS_ROJAS[i], 1);

            // Barrido de filas
            for(int fila = 0; fila < 8; fila++) {
                gpio_set_level(PINES_FILAS[fila], 1);
                vTaskDelay(pdMS_TO_TICKS(2));
                gpio_set_level(PINES_FILAS[fila], 0);
            }
        } else {
            // Aseguramos que todo esté apagado
            for(int i=0; i<8; i++) {
                gpio_set_level(PINES_FILAS[i], 0);
                gpio_set_level(PINES_COLS_ROJAS[i], 0);
            }
            vTaskDelay(pdMS_TO_TICKS(20)); // Ahorro de CPU
        }
    }
}