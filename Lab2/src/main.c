#include <stdint.h> // Componentes básicos de C
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"



#define MTX_G_2 16 
#define MTX_G_3 19
#define MTX_G_4 18
#define MTX_G_5 5


#define FILA_0 13
#define FILA_1 12
#define FILA_2 14
#define FILA_3 27
#define FILA_4 15
#define FILA_5 2
#define FILA_6 0
#define FILA_7 4


#define MTX_R_0 26
#define MTX_R_1 21
#define MTX_R_2 33
#define MTX_R_3 32
#define MTX_R_4 25
#define MTX_R_5 1
#define MTX_R_6 22
#define MTX_R_7 23

#define BTN 34  

const uint8_t FILA_PINES[8] = {FILA_0, FILA_1, FILA_2, FILA_3, FILA_4, FILA_5, FILA_6, FILA_7};


uint8_t matriz_roja[8] = {0};
uint8_t matriz_verde[8] = {0};
// Aquí ya pusimos 8 entonces C entiende que hay otras 7 antes

volatile uint8_t fila_actual = 0; 
//le dice al procesador que la variable puede cambiar de repente en una interrupción, no la optimices y léela siempre directo de la memoria RAM

static inline void apagar_columnas() {
    //gpio_set_level(MTX_G_0, 0); 
    //gpio_set_level(MTX_G_1, 0); 
    gpio_set_level(MTX_G_2, 0); 
    gpio_set_level(MTX_G_3, 0); 
    gpio_set_level(MTX_G_4, 0); 
    gpio_set_level(MTX_G_5, 0); 
    //gpio_set_level(MTX_G_6, 0); 
    //gpio_set_level(MTX_G_7, 0); 
    gpio_set_level(MTX_R_0, 0); 
    gpio_set_level(MTX_R_1, 0); 
    gpio_set_level(MTX_R_2, 0); 
    gpio_set_level(MTX_R_3, 0); 
    gpio_set_level(MTX_R_4, 0); 
    gpio_set_level(MTX_R_5, 0); 
    gpio_set_level(MTX_R_6, 0); 
    gpio_set_level(MTX_R_7, 0); 
};


void rutina_timer(void* arg) { //void* arg hace que la librería permita usarlo como alarma/flancos 
    static uint8_t color_actual = 0;
    apagar_columnas();
    
    //gpio_set_level(DMX_0, (fila_actual & 0x01) ? 1 : 0);
    //gpio_set_level(DMX_1, (fila_actual & 0x02) ? 1 : 0);
    //gpio_set_level(DMX_2, (fila_actual & 0x04) ? 1 : 0);
    for(int i = 0; i<8; i++) {
        gpio_set_level(FILA_PINES[i], (i == fila_actual) ? 1 : 0);
    }


    if (color_actual ==0){
        //ROJO
        gpio_set_level(MTX_R_0, (matriz_roja[fila_actual] & (1 << 0)) ? 1 : 0); //desempaca el lsb de la fila
        gpio_set_level(MTX_R_1, (matriz_roja[fila_actual] & (1 << 1)) ? 1 : 0); //desempaca el segundo b de la fila
        gpio_set_level(MTX_R_2, (matriz_roja[fila_actual] & (1 << 2)) ? 1 : 0); //... and so on
        gpio_set_level(MTX_R_3, (matriz_roja[fila_actual] & (1 << 3)) ? 1 : 0);
        gpio_set_level(MTX_R_4, (matriz_roja[fila_actual] & (1 << 4)) ? 1 : 0);
        gpio_set_level(MTX_R_5, (matriz_roja[fila_actual] & (1 << 5)) ? 1 : 0);
        gpio_set_level(MTX_R_6, (matriz_roja[fila_actual] & (1 << 6)) ? 1 : 0);
        gpio_set_level(MTX_R_7, (matriz_roja[fila_actual] & (1 << 7)) ? 1 : 0);

        color_actual = 1;
    }
    else{
        //VERDE
        //gpio_set_level(MTX_G_0, (matriz_verde[fila_actual] & (1 << 0)) ? 1 : 0); 
        //gpio_set_level(MTX_G_1, (matriz_verde[fila_actual] & (1 << 1)) ? 1 : 0); 
        gpio_set_level(MTX_G_2, (matriz_verde[fila_actual] & (1 << 2)) ? 1 : 0);
        gpio_set_level(MTX_G_3, (matriz_verde[fila_actual] & (1 << 3)) ? 1 : 0);
        gpio_set_level(MTX_G_4, (matriz_verde[fila_actual] & (1 << 4)) ? 1 : 0);
        gpio_set_level(MTX_G_5, (matriz_verde[fila_actual] & (1 << 5)) ? 1 : 0);
        //gpio_set_level(MTX_G_6, (matriz_verde[fila_actual] & (1 << 6)) ? 1 : 0);
        //gpio_set_level(MTX_G_7, (matriz_verde[fila_actual] & (1 << 7)) ? 1 : 0);
        color_actual = 0;

        if (fila_actual == 7) {
            fila_actual = 0;
        }
        else {
            fila_actual = fila_actual +1;
        }

    }

}

//Definir FSM 
typedef enum {
    E_PREGAME, 
    E_MOVIENDO,
    E_EVALUANDO,
    E_GAME_OVER, 
    E_NEXT_LEVEL, 
    E_GANADOR
} estado_juego_t;
estado_juego_t estado_actual = E_PREGAME;

//Variables de Inicio
int8_t fila_juego = 1; 
int8_t ancho_bloque = 6; 
int8_t posicion_x = 1; //posición horizontal del bloque 0-7
int8_t direccion = 1; //1 = hacia izq -1 = hacia der
uint32_t velocidad = 200; //ms en moverse un paso
uint32_t step_vel = 15; //lo que baja de V en cada stack
uint32_t tiempo_acumulado = 0;

static inline bool btn_pressed(gpio_num_t pin){
	return gpio_get_level(pin) == 0;
}

void app_main() {
    gpio_config_t in_cfg = {
		.pin_bit_mask = (1ULL<< BTN), //lo cambié de 1UL a 1ULL para que si pudiera cogewr el 34
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE, //eL PIN 34 NO TIENE, puse una por hardware
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&in_cfg); 


    gpio_reset_pin(1);//para decirle que no los use como comunicación
    gpio_reset_pin(3);
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << FILA_0) | (1ULL << FILA_1) | (1ULL << FILA_2) | (1ULL << FILA_3) | (1ULL << FILA_4) | (1ULL << FILA_5) | (1ULL << FILA_6) | (1ULL << FILA_7) |
        //(1ULL << DMX_0) | (1ULL << DMX_1) | (1ULL << DMX_2) |
       // (1ULL << MTX_G_0) | (1ULL << MTX_G_1) | 
       (1ULL << MTX_G_2) | (1ULL << MTX_G_3) | (1ULL << MTX_G_4) | (1ULL << MTX_G_5) |
       // | (1ULL << MTX_G_6) | (1ULL << MTX_G_7) | 
        (1ULL << MTX_R_0) | (1ULL << MTX_R_1) | (1ULL << MTX_R_2) | (1ULL << MTX_R_3) | (1ULL << MTX_R_4) | (1ULL << MTX_R_5) | (1ULL << MTX_R_6) | (1ULL << MTX_R_7),
        .mode = GPIO_MODE_OUTPUT, 
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_DISABLE
	};
    gpio_config(&out_cfg);

    const esp_timer_create_args_t timer_args = {
        .callback = &rutina_timer, 
        .name = "timer_multiplezación"
    };
    esp_timer_handle_t multiplex_timer; //variable del timer
    esp_timer_create(&timer_args, &multiplex_timer);
    esp_timer_start_periodic(multiplex_timer, 1000);

    
    bool ultimo_estado_btn = false;
    while(1){
        bool btn_actual = btn_pressed(BTN);
        bool flanco_boton = (btn_actual == true && ultimo_estado_btn == false);
        ultimo_estado_btn = btn_actual; 

        tiempo_acumulado = tiempo_acumulado + 10; //+10ms
        switch (estado_actual){
            case E_PREGAME: 
                for (int i=0; i<8; i++){
                    matriz_verde[i] = 0;
                }

                for(int i=2; i<8; i++){
                    matriz_roja[i] = 0;
                }
                
                fila_juego = 1; 
                ancho_bloque = 6; 
                posicion_x = 1;

                matriz_roja[0] = 0b01111110;
                matriz_roja[1] = 0b01111110;

                if (flanco_boton){
                    estado_actual = E_MOVIENDO;
                }
                break;
            
            case E_MOVIENDO:
                if (flanco_boton){
                    estado_actual = E_EVALUANDO;
                    break;
                }
                if (tiempo_acumulado >= velocidad) {
                    
                    if (direccion ==1) {
                        matriz_roja[fila_juego] = (matriz_roja[fila_juego] <<1);
                    }
                    else if (direccion ==-1) {
                        matriz_roja[fila_juego] = (matriz_roja[fila_juego] >>1);
                    }

                    //posicion_x = posicion_x + direccion;

                    if (matriz_roja[fila_juego] & 0b10000000) {
                        direccion = -1; // Obligamos a ir a la derecha
                    }
                    // ¿El bit 0 (extremo derecho) está prendido?
                    else if (matriz_roja[fila_juego] & 0b00000001) {
                        direccion = 1;  // Obligamos a ir a la izquierda
                    }

                    tiempo_acumulado = 0;

                }
                break; 

            case E_EVALUANDO: 
                matriz_roja[fila_juego] = (matriz_roja[fila_juego] & matriz_roja[fila_juego-1]);
                if (matriz_roja[fila_juego] == 0) {
                    estado_actual= E_GAME_OVER;
                }
                else if (fila_juego == 7){
                    estado_actual = E_NEXT_LEVEL;
                }
                else {
                    fila_juego = fila_juego +1; 
                    matriz_roja[fila_juego] = matriz_roja[fila_juego-1];
                    velocidad = velocidad - step_vel;
                    estado_actual = E_MOVIENDO;
                }
                break;
            case E_GAME_OVER:
                matriz_roja[7] = 0b11111111;
                matriz_roja[6] = 0b10011001;
                matriz_roja[5] = 0b10011001;
                matriz_roja[4] = 0b11111111;
                matriz_roja[3] = 0b11000011;
                matriz_roja[2] = 0b10111101;
                matriz_roja[1] = 0b01111110;
                matriz_roja[0] = 0b11111111;

                step_vel = 0;
                velocidad = 200;

                for (int i=0; i<8; i++){
                    matriz_verde[i] = 0;
                }

                vTaskDelay(pdMS_TO_TICKS(5000));
                estado_actual = E_PREGAME;
                break; 
            
            case E_NEXT_LEVEL: 
                if (ancho_bloque <= 2 && fila_juego ==7 ){
                    estado_actual = E_GANADOR;
                } 
                else {
                    for (int i=0; i<8; i++) {
                        matriz_roja[i] =0;
                    }
                    vTaskDelay(pdMS_TO_TICKS(500));
                    
                    fila_juego = 1; 
                    ancho_bloque = ancho_bloque -2;
                    
                    posicion_x = (8 - ancho_bloque)/2;

                    matriz_roja[0] = (((1 << ancho_bloque) - 1) << posicion_x);
                    matriz_roja[1] = matriz_roja[0];

                    estado_actual = E_MOVIENDO;
                }

                break; 

            case E_GANADOR:
                //Dibuja un Trofeo
                
                vTaskDelay(pdMS_TO_TICKS(500));
                for (int i=0; i<8; i++) {
                    matriz_roja[i] =0;
                }
                vTaskDelay(pdMS_TO_TICKS(500));

                matriz_roja[7] = 0b00000000;
                matriz_roja[6] = 0b00100100; 
                matriz_roja[5] = 0b11111111;
                matriz_roja[4] = 0b10111101;
                matriz_roja[3] = 0b01111110;
                matriz_roja[2] = 0b00011000;
                matriz_roja[1] = 0b00011000;
                matriz_roja[0] = 0b00111100;

                matriz_verde[7] = 0b00000000;
                matriz_verde[6] = 0b00011000; 
                matriz_verde[5] = 0b00000000;
                matriz_verde[4] = 0b00000000;
                matriz_verde[3] = 0b00000000;
                matriz_verde[2] = 0b00011000;
                matriz_verde[1] = 0b00011000;
                matriz_verde[0] = 0b00011000;

                vTaskDelay(pdMS_TO_TICKS(5000)); 
                estado_actual = E_PREGAME;

                break;

        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }



    

}