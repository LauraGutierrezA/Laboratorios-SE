#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "SPI.h"
#include "RFID.h"
#include "I2C.h"
#include "LCD.h"
#include "BleNus.h"

// Pines SPI
#define SPI_MOSI_PIN  23
#define SPI_MISO_PIN  19
#define SPI_CLK_PIN   18
#define SPI_CS_PIN     5

#define PIN_RST        4

// Pines I2C
#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define LCD_I2C_ADDR  0x27
#define RTC_ADDR      0x68

// Llaves registradas
static const uint8_t KEY_SUPERVISOR[4]    = {0xFB, 0x55, 0x14, 0x07};
static const uint8_t KEY_NO_AUTORIZADO[4] = {0x74, 0xA9, 0x2C, 0x07};
static const uint8_t KEY_REINAS[4]        = {0x88, 0x04, 0x5B, 0x9C};

// Estado del sistema
enum EstadoSistema {
    ESTADO_BLOQUEADO,
    ESTADO_ACTIVO
};

// --- RTC nativo ---
uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }
uint8_t bcd2dec(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }

void rtc_set_time(uint8_t h, uint8_t m, uint8_t s, uint8_t dow, uint8_t dom, uint8_t month, uint8_t year) {
    uint8_t data[8];
    data[0] = 0x00;
    data[1] = dec2bcd(s) & 0x7F; // CH=0: oscilador encendido
    data[2] = dec2bcd(m);
    data[3] = dec2bcd(h) & 0x3F; // formato 24h
    data[4] = dec2bcd(dow);
    data[5] = dec2bcd(dom);
    data[6] = dec2bcd(month);
    data[7] = dec2bcd(year);
    i2c_master_write_to_device(I2C_NUM_0, RTC_ADDR, data, 8, pdMS_TO_TICKS(100));
}

bool rtc_get_time(uint8_t *h, uint8_t *m, uint8_t *s) {
    uint8_t reg = 0x00;
    if (i2c_master_write_to_device(I2C_NUM_0, RTC_ADDR, &reg, 1, pdMS_TO_TICKS(50)) != ESP_OK) return false;
    uint8_t data[3];
    if (i2c_master_read_from_device(I2C_NUM_0, RTC_ADDR, data, 3, pdMS_TO_TICKS(50)) != ESP_OK) return false;
    *s = bcd2dec(data[0] & 0x7F);
    *m = bcd2dec(data[1]);
    *h = bcd2dec(data[2] & 0x3F);
    return true;
}

// ======================================================================
// ========================== FUNCIÓN PRINCIPAL =========================
// ======================================================================

extern "C" void app_main() {
    // 1. Reset físico del lector RFID
    gpio_set_direction((gpio_num_t)PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level((gpio_num_t)PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // 2. Inicializar I2C y LCD
    I2CMaster i2cBus(I2C_NUM_0, I2C_SDA_PIN, I2C_SCL_PIN, 100000);
    i2cBus.init();
    LCD miLcd(I2C_NUM_0, LCD_I2C_ADDR);
    miLcd.init();

    // 3. Ajuste de hora en el RTC (Viernes 22/05/2026 17:44:37)
    rtc_set_time(17, 44, 37, 5, 22, 5, 26);

    // 4. Inicializar SPI y RFID
    SPI spiBus(SPI2_HOST, SPI_MOSI_PIN, SPI_MISO_PIN, SPI_CLK_PIN, SPI_CS_PIN, 0, true);
    spiBus.init();
    Rfid lector(spiBus);
    lector.init();

    // 5. Inicializar Bluetooth
    BleNus miBluetooth;
    miBluetooth.init("PanelHMI");

    printf("=== CONTROL DE ACCESO LISTO ===\n");

    EstadoSistema estadoActual = ESTADO_BLOQUEADO;
    char bufferBle[64] = "Sin mensajes";
    char bufferHora[20] = "";
    char tempBle[64]  = "";
    bool refrescarBloqueo    = true;
    bool primerIngresoActivo = true;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        uint8_t h = 0, m = 0, s = 0;
        bool tieneHora = rtc_get_time(&h, &m, &s);

        switch (estadoActual) {

            case ESTADO_BLOQUEADO:
                if (refrescarBloqueo) {
                    miLcd.mostrarMensaje("Panel bloqueado", "Acerque credencial");
                    refrescarBloqueo = false;
                }

                // Drena mensajes BLE para que no se acumulen mientras está bloqueado
                miBluetooth.leer(tempBle);

                if (lector.leerTarjeta()) {
                    lector.disableAntenna(); // apagar RF antes de escribir por I2C

                    if (lector.verificarTarjeta(KEY_SUPERVISOR)) {
                        if (tieneHora) snprintf(bufferHora, sizeof(bufferHora), "%02d:%02d:%02d", h, m, s);
                        miLcd.mostrarMensaje("Acceso concedido", tieneHora ? bufferHora : "");
                        printf("ACCESO CONCEDIDO [%s]\n", bufferHora);
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        estadoActual = ESTADO_ACTIVO;
                        primerIngresoActivo = true;
                        strcpy(bufferBle, "Sin mensajes");
                        // antena se reactiva en primerIngresoActivo

                    } else if (lector.verificarTarjeta(KEY_REINAS)) {
                        miLcd.mostrarMensaje("Hola Reinas!", "");
                        printf("REINAS DE LA ELECTRONICA [%s]\n", bufferHora);
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        lector.enableAntenna();
                        refrescarBloqueo = true;

                    } else {
                        miLcd.mostrarMensaje("Acceso denegado", "UID no registrado");
                        printf("ACCESO DENEGADO [%s]\n", bufferHora);
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        lector.enableAntenna();
                        refrescarBloqueo = true;
                    }
                }
                break;

            case ESTADO_ACTIVO:
                if (primerIngresoActivo) {
                    lector.enableAntenna(); // reactivar RF al entrar al estado activo
                    miLcd.mostrarMensaje(bufferBle, "");
                    primerIngresoActivo = false;
                }

                if (miBluetooth.leer(tempBle)) {
                    strncpy(bufferBle, tempBle, 16);
                    bufferBle[16] = '\0';
                    miLcd.setCursor(0, 0);
                    miLcd.print("                ");
                    miLcd.setCursor(0, 0);
                    miLcd.print(bufferBle);
                }

                if (tieneHora) {
                    snprintf(bufferHora, sizeof(bufferHora), "%02d:%02d:%02d", h, m, s);
                    miLcd.setCursor(1, 0);
                    miLcd.print(bufferHora);
                }

                if (lector.leerTarjeta()) {
                    if (lector.verificarTarjeta(KEY_SUPERVISOR)) {
                        lector.disableAntenna();
                        miLcd.mostrarMensaje("Sesion cerrada", "");
                        printf("SESION CERRADA\n");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        lector.enableAntenna();
                        estadoActual = ESTADO_BLOQUEADO;
                        refrescarBloqueo = true;
                    }
                }
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}
