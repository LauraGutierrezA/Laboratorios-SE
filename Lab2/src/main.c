#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// --- PINES DE LA MATRIZ VERDE ---
#define MTX_G_2 16 
#define MTX_G_3 19
#define MTX_G_4 18
#define MTX_G_5 5

// --- PINES DE LAS FILAS ---
#define FILA_0 13
#define FILA_1 12
#define FILA_2 14
#define FILA_3 27
#define FILA_4 15
#define FILA_5 2
#define FILA_6 0
#define FILA_7 4

// --- PINES DE LA MATRIZ ROJA ---
#define MTX_R_0 26
#define MTX_R_1 21
#define MTX_R_2 33
#define MTX_R_3 32
#define MTX_R_4 25
#define MTX_R_5 1
#define MTX_R_6 22
#define MTX_R_7 23

const uint8_t FILA_PINES[8] = {FILA_0, FILA_1, FILA_2, FILA_3, FILA_4, FILA_5, FILA_6, FILA_7};

uint8_t matriz_roja[8] = {0};
uint8_t matriz_verde[8] = {0};
volatile uint8_t fila_actual = 0; 

static inline void apagar_columnas() {
    gpio_set_level(MTX_G_2, 0); 
    gpio_set_level(MTX_G_3, 0); 
    gpio_set_level(MTX_G_4, 0); 
    gpio_set_level(MTX_G_5, 0); 
    gpio_set_level(MTX_R_0, 0); 
    gpio_set_level(MTX_R_1, 0); 
    gpio_set_level(MTX_R_2, 0); 
    gpio_set_level(MTX_R_3, 0); 
    gpio_set_level(MTX_R_4, 0); 
    gpio_set_level(MTX_R_5, 0); 
    gpio_set_level(MTX_R_6, 0); 
    gpio_set_level(MTX_R_7, 0); 
};

void rutina_timer(void* arg) { 
    static uint8_t color_actual = 0;
    apagar_columnas();
    
    for(int i = 0; i<8; i++) {
        gpio_set_level(FILA_PINES[i], (i == fila_actual) ? 1 : 0);
    }

    if (color_actual == 0){
        // TURNO ROJO
        gpio_set_level(MTX_R_0, (matriz_roja[fila_actual] & (1 << 0)) ? 1 : 0); 
        gpio_set_level(MTX_R_1, (matriz_roja[fila_actual] & (1 << 1)) ? 1 : 0); 
        gpio_set_level(MTX_R_2, (matriz_roja[fila_actual] & (1 << 2)) ? 1 : 0); 
        gpio_set_level(MTX_R_3, (matriz_roja[fila_actual] & (1 << 3)) ? 1 : 0);
        gpio_set_level(MTX_R_4, (matriz_roja[fila_actual] & (1 << 4)) ? 1 : 0);
        gpio_set_level(MTX_R_5, (matriz_roja[fila_actual] & (1 << 5)) ? 1 : 0);
        gpio_set_level(MTX_R_6, (matriz_roja[fila_actual] & (1 << 6)) ? 1 : 0);
        gpio_set_level(MTX_R_7, (matriz_roja[fila_actual] & (1 << 7)) ? 1 : 0);
        color_actual = 1;
    }
    else{
        // TURNO VERDE
        gpio_set_level(MTX_G_2, (matriz_verde[fila_actual] & (1 << 2)) ? 1 : 0);
        gpio_set_level(MTX_G_3, (matriz_verde[fila_actual] & (1 << 3)) ? 1 : 0);
        gpio_set_level(MTX_G_4, (matriz_verde[fila_actual] & (1 << 4)) ? 1 : 0);
        gpio_set_level(MTX_G_5, (matriz_verde[fila_actual] & (1 << 5)) ? 1 : 0);
        color_actual = 0;

        if (fila_actual == 7) fila_actual = 0;
        else fila_actual++;
    }
}

void app_main() {
    // 1. LA MAGIA DEL RESET: Agrupamos todos los pines usados y los purificamos en un ciclo
    const uint8_t TODOS_LOS_PINES[] = {
        FILA_0, FILA_1, FILA_2, FILA_3, FILA_4, FILA_5, FILA_6, FILA_7,
        MTX_R_0, MTX_R_1, MTX_R_2, MTX_R_3, MTX_R_4, MTX_R_5, MTX_R_6, MTX_R_7,
        MTX_G_2, MTX_G_3, MTX_G_4, MTX_G_5
    };

    for(int i = 0; i < 20; i++) {
        gpio_reset_pin(TODOS_LOS_PINES[i]);
    }
    
    // 2. Configurar salidas en bloque
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << FILA_0) | (1ULL << FILA_1) | (1ULL << FILA_2) | (1ULL << FILA_3) | (1ULL << FILA_4) | (1ULL << FILA_5) | (1ULL << FILA_6) | (1ULL << FILA_7) |
        (1ULL << MTX_G_2) | (1ULL << MTX_G_3) | (1ULL << MTX_G_4) | (1ULL << MTX_G_5) |
        (1ULL << MTX_R_0) | (1ULL << MTX_R_1) | (1ULL << MTX_R_2) | (1ULL << MTX_R_3) | (1ULL << MTX_R_4) | (1ULL << MTX_R_5) | (1ULL << MTX_R_6) | (1ULL << MTX_R_7),
        .mode = GPIO_MODE_OUTPUT, 
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);

    // 3. Arrancar el Timer
    const esp_timer_create_args_t timer_args = {
        .callback = &rutina_timer, 
        .name = "timer_multiplexacion"
    };
    esp_timer_handle_t multiplex_timer; 
    esp_timer_create(&timer_args, &multiplex_timer);
    esp_timer_start_periodic(multiplex_timer, 1000);

    // 4. DIBUJAR EL TROFEO ESTÁTICO (Estado E_GANADOR puro)
    // Trofeo en la capa Roja
    matriz_roja[7] = 0b00000000;
    matriz_roja[6] = 0b00100100; // Franja superior
    matriz_roja[5] = 0b11111111;
    matriz_roja[4] = 0b10111101;
    matriz_roja[3] = 0b01111110;
    matriz_roja[2] = 0b00011000;
    matriz_roja[1] = 0b00011000;
    matriz_roja[0] = 0b00111100;

    // Decoración en la capa Verde para crear el Naranja
    matriz_verde[7] = 0b00000000;
    matriz_verde[6] = 0b00011000; // Franja superior (Rojo + Verde = Naranja)
    matriz_verde[5] = 0b00000000;
    matriz_verde[4] = 0b00000000;
    matriz_verde[3] = 0b00000000;
    matriz_verde[2] = 0b00011000;
    matriz_verde[1] = 0b00011000;
    matriz_verde[0] = 0b00011000;

    // Bucle vacío para dejar que la matriz brille sin interrupciones
    while(1){
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}