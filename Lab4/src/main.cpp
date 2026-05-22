#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "I2C.h"
#include "SPI.h"
#include "../lib/RTC/RTC.h"
#include "BleNus.h"
#include "LCD.h"
#include "LED.h"
#include "RFID.h"

#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22
#define RTC_ADDR        0x68
#define LCD_ADDR        0x27  

#define SPI_MOSI_PIN    23
#define SPI_MISO_PIN    19
#define SPI_CLK_PIN     18
#define SPI_CS_PIN      5

#define PIN_LED_ROJO    2
#define PIN_LED_VERDE   4
#define PIN_LED_AZUL    15
#define PIN_BUZZER      13

const uint8_t SUPERVISOR_KEY[4] = {0x0F, 0x0F, 0x0F, 0x0F}; //cuando hagamos el lab, lo cambiamos por los numeros reales de la tarjeta

enum EstadoSistema {
    ESTADO_BLOQUEADO,
    ESTADO_ACTIVO
};

I2CMaster i2cBus(I2C_NUM_0, I2C_SDA_PIN, I2C_SCL_PIN, 100000);
Rtc miReloj(i2cBus, RTC_ADDR);
LCD miLcd(I2C_NUM_0, LCD_ADDR); 

SPI spiBus(SPI1_HOST, SPI_MOSI_PIN, SPI_MISO_PIN, SPI_CLK_PIN, SPI_CS_PIN, 0, true); //modo 0 y true de que se envia el MSB de primero
Rfid miLectorRfid(spiBus);     

Led ledRojo(PIN_LED_ROJO);
Led ledVerde(PIN_LED_VERDE);
Led ledAzul(PIN_LED_AZUL);
Led buzzer(PIN_BUZZER);       

BleNus miBluetooth; 

//aca lo imprimo bonito
void obtenerCadenaHora(char* buffer) {
    uint8_t h = 0, m = 0, s = 0;
    miReloj.getTime(h, m, s); 
    sprintf(buffer, "%02d:%02d:%02d", h, m, s);
}

extern "C" void app_main() {
    i2cBus.init();
    spiBus.init();
    
    miLcd.init();
    miLectorRfid.init();
    miBluetooth.init("PanelHMI"); 

    ledRojo.init();
    ledVerde.init();
    ledAzul.init();
    buzzer.init();

    EstadoSistema estadoActual = ESTADO_BLOQUEADO;
    char bufferBle[64] = "Sin mensajes"; 
    char bufferHora[16] = "";             
    char tempBle[64] = "";                
    
    bool refrescarBloqueo = true;
    bool primerIngresoActivo = true;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        switch (estadoActual) {
            
            case ESTADO_BLOQUEADO:
                if (refrescarBloqueo) {
                    ledRojo.on();      
                    ledVerde.off();    
                    ledAzul.off();
                    
                    miLcd.mostrarMensaje("Panel bloqueado", "Acerque credencial");
                    refrescarBloqueo = false;
                }

                miBluetooth.leer(tempBle);
                //preguntamos si hay una tarjeta cerca
                if (miLectorRfid.leerTarjeta() == true) {
                    //verificamos si es la llave correcta
                    if (miLectorRfid.verificarTarjeta(SUPERVISOR_KEY) == true) {
                        //se le dio acceso
                        ledRojo.off();
                        ledVerde.on();
                        buzzer.on();
                        
                        obtenerCadenaHora(bufferHora);
                        miLcd.mostrarMensaje("Acceso concedido", bufferHora);
                        
                        vTaskDelay(pdMS_TO_TICKS(500));
                        buzzer.off(); 
                        vTaskDelay(pdMS_TO_TICKS(500)); 
                        ledVerde.off();
                        
                        estadoActual = ESTADO_ACTIVO;
                        primerIngresoActivo = true;
                        strcpy(bufferBle, "Sin mensajes"); //borra los mensajes anteiores y pone sin mensajes
                    } 
                    else {
                        //no se le dio acceso
                        miLcd.mostrarMensaje("Acceso denegado", "UID no registrado");
                        
                        buzzer.on();
                        //led parpadea 3 veces
                        for (int i = 0; i < 3; i++) {
                            ledRojo.on();
                            vTaskDelay(pdMS_TO_TICKS(333));
                            ledRojo.off();
                            vTaskDelay(pdMS_TO_TICKS(333));
                        }
                        buzzer.off(); 
                        refrescarBloqueo = true; 
                    }
                }
                break;

            case ESTADO_ACTIVO:
            //esto configura los leds y el lcd solo una vez al entrar al estado activo para evitar que la pantalla parpadee por el bucle infinito
                if (primerIngresoActivo) {
                    ledRojo.off();
                    ledAzul.on();      
                    
                    miLcd.mostrarMensaje(bufferBle, ""); 
                    primerIngresoActivo = false;
                }

                if (miBluetooth.leer(tempBle) == true) {
                    strncpy(bufferBle, tempBle, 16);
                    bufferBle[16] = '\0'; 
                    
                    miLcd.setCursor(0, 0); 
                    miLcd.print("                "); 
                    miLcd.setCursor(0, 0);
                    miLcd.print(bufferBle); 
                }

                obtenerCadenaHora(bufferHora);
                miLcd.setCursor(1, 0); 
                miLcd.print(bufferHora);

                if (miLectorRfid.leerTarjeta() == true) {
                    if (miLectorRfid.verificarTarjeta(SUPERVISOR_KEY) == true) {
                        //se cierra sesion
                        buzzer.on();
                        vTaskDelay(pdMS_TO_TICKS(500)); 
                        buzzer.off();
                        
                        ledAzul.off();
                        estadoActual = ESTADO_BLOQUEADO;
                        refrescarBloqueo = true;
                    }
                }
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}