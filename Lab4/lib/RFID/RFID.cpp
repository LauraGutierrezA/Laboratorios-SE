/* EN EL MAIN ESTO SE UTILIZARÍA ASÍ

1. PARA AVERIGUAR LA TARJETA
if (lector.leerTarjeta() == true) {
    printf("Tarjeta detectada. Tamaño: %d bytes. UID: ", lector.getLongitud());
    for(int i = 0; i < lector.getLongitud(); i++) {
        printf("%02X ", lector.getByteUID(i));
    }
    printf("\n");
}    

2. PARA BLOQUEAR O PERMITIR EL ACCESO
uint8_t SUPERVISOR_KEY[4] = {0x0f, 0x0f, 0x0f, 0x0f};
if (lector.leerTarjeta() == true){ // SI leyó bien
    if (lector.verificarTarjeta(SUPERVISOR_KEY) == true){
        //hacer lo que sea que hace si es supervisor
    }
}

*/

#include "RFID.h"

Rfid::Rfid(SPI rfid){
    _rfid = rfid;
    _longitud = 0; 
    for(int i = 0; i < 8; i++) _uid[i] = 0; 
}

Rfid::~Rfid(){

}

void Rfid::init(){
    // Soft reset: limpia todos los registros internos
    _rfid.writeSPI(0x01, 0x0F);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Limpiar bits de modo en TxModeReg y RxModeReg
    // (dejar en 0x00 = modo estándar ISO 14443A a 106 kbps)
    _rfid.writeSPI(0x12, 0x00); // TxModeReg
    _rfid.writeSPI(0x13, 0x00); // RxModeReg

    // Configurar timer igual al bare-metal probado (15 ms de timeout)
    _rfid.writeSPI(0x2A, 0x8D); // TModeReg: TAuto=1, TPrescalerHigh=0xD
    _rfid.writeSPI(0x2B, 0x3E); // TPrescalerReg: TPrescalerLow=0x3E → f_timer≈2kHz
    _rfid.writeSPI(0x2C, 0x00); // TReloadReg Hi
    _rfid.writeSPI(0x2D, 30);   // TReloadReg Lo → timeout = 30/2kHz ≈ 15ms

    // TxASKReg: forzar modulación ASK al 100%
    _rfid.writeSPI(0x15, 0x40);

    // ModeReg: CRC preset 0x6363 (estándar ISO 14443)
    _rfid.writeSPI(0x11, 0x3D);

    // TxControlReg: habilitar antenas Tx1 y Tx2 preservando bits superiores.
    // Tras reset TxControlReg=0x80; OR con 0x03 da 0x83 (igual que bare-metal).
    uint8_t txCtrl = _rfid.readSPI(0x14);
    _rfid.writeSPI(0x14, txCtrl | 0x03);
}

void Rfid::buscarTarjeta(){
    // 1. Cancelar cualquier comando activo → IDLE
    _rfid.writeSPI(0x01, 0x00);

    // 2. Limpiar flags de interrupción en ComIrqReg
    _rfid.writeSPI(0x04, 0x7F);

    // 3. Vaciar el buffer FIFO (FlushBuffer=1)
    _rfid.writeSPI(0x0A, 0x80);

    // 4. Escribir comando REQA (0x26) en el FIFO
    _rfid.writeSPI(0x09, 0x26);

    // 5. CORRECCIÓN CRÍTICA: BitFramingReg → TxLastBits=7
    // REQA es un frame de 7 bits (ISO 14443A). Sin esto el RC522
    // transmite 8 bits y la tarjeta nunca responde.
    _rfid.writeSPI(0x0D, 0x07);

    // 6. Ejecutar comando Transceive (transmite FIFO y espera respuesta)
    _rfid.writeSPI(0x01, 0x0C);

    // 7. Activar transmisión seteando StartSend=1 en BitFramingReg
    _rfid.writeSPI(0x0D, 0x87);
}

bool Rfid::procesarTarjeta(){
    // Poll ComIrqReg: bit5=RxIrq (respuesta recibida), bit0=TimerIrq (timeout)
    uint8_t irqReg = 0;
    for (int i = 0; i < 50; i++) {
        irqReg = _rfid.readSPI(0x04);
        if (irqReg & 0x20) break;
        if (irqReg & 0x01) {
            _rfid.writeSPI(0x01, 0x00);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    // Si el loop se agotó sin RxIrq, no hay tarjeta
    if (!(irqReg & 0x20)) {
        _rfid.writeSPI(0x01, 0x00);
        return false;
    }

    uint8_t errorStatus = _rfid.readSPI(0x06);
    if (errorStatus & 0x1B) {
        _rfid.writeSPI(0x01, 0x00);
        return false;
    }

    // Tarjeta presente (ATQA recibido). El FIFO se vacía en antiColision().
    _rfid.writeSPI(0x01, 0x00);
    return true;
}

bool Rfid::antiColision() {
    _rfid.writeSPI(0x01, 0x00);    // IDLE
    _rfid.writeSPI(0x04, 0x7F);    // Limpiar ComIrqReg
    _rfid.writeSPI(0x0A, 0x80);    // Vaciar FIFO (descarta los 2 bytes de ATQA)

    // Anti-colisión CL1: SEL=0x93, NVB=0x20 (ningún bit de UID conocido)
    // CRC deshabilitado para anti-colisión (ISO 14443A)
    _rfid.writeSPI(0x12, 0x00);    // TxModeReg: TxCRCEn=0
    _rfid.writeSPI(0x13, 0x00);    // RxModeReg: RxCRCEn=0

    _rfid.writeSPI(0x09, 0x93);    // FIFODataReg: SEL Cascada Nivel 1
    _rfid.writeSPI(0x09, 0x20);    // FIFODataReg: NVB

    _rfid.writeSPI(0x0D, 0x00);    // BitFramingReg: tramas completas de 8 bits
    _rfid.writeSPI(0x01, 0x0C);    // Transceive
    _rfid.writeSPI(0x0D, 0x80);    // StartSend=1, TxLastBits=0

    uint8_t irqReg = 0;
    for (int i = 0; i < 50; i++) {
        irqReg = _rfid.readSPI(0x04);
        if (irqReg & 0x20) break;
        if (irqReg & 0x01) {
            _rfid.writeSPI(0x01, 0x00);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!(irqReg & 0x20)) {
        _rfid.writeSPI(0x01, 0x00);
        return false;
    }

    uint8_t errorStatus = _rfid.readSPI(0x06);
    if (errorStatus & 0x1B) {
        _rfid.writeSPI(0x01, 0x00);
        return false;
    }

    // La tarjeta responde con 5 bytes: UID[0..3] + BCC
    uint8_t rxLen = _rfid.readSPI(0x0A) & 0x7F;   // máscara bit7 (FlushBuffer)
    if (rxLen < 5) {
        _rfid.writeSPI(0x01, 0x00);
        return false;
    }

    uint8_t buf[5];
    for (int i = 0; i < 5; i++) {
        buf[i] = _rfid.readSPI(0x09);
    }

    // Verificar BCC: UID[0]^UID[1]^UID[2]^UID[3] debe ser igual a BCC
    if ((buf[0] ^ buf[1] ^ buf[2] ^ buf[3]) != buf[4]) {
        _rfid.writeSPI(0x01, 0x00);
        return false;
    }

    _longitud = 4;
    for (int i = 0; i < 4; i++) _uid[i] = buf[i];

    _rfid.writeSPI(0x01, 0x00);
    _rfid.writeSPI(0x0D, 0x00);
    return true;
}

bool Rfid::leerTarjeta() {
    _longitud = 0;
    buscarTarjeta();
    if (!procesarTarjeta()) return false;  // detecta ATQA (presencia de tarjeta)
    return antiColision();                 // obtiene UID real via anti-colisión CL1
}

bool Rfid::verificarTarjeta(const uint8_t* uidCard){
    if (_longitud == 0) return false; 

    for (int i = 0; i < _longitud; i++){
        if (_uid[i] != uidCard[i]){
            return false;
        }
    }
    return true;
}

void Rfid::enableAntenna() {
    uint8_t txCtrl = _rfid.readSPI(0x14);
    _rfid.writeSPI(0x14, txCtrl | 0x03);
}

void Rfid::disableAntenna() {
    uint8_t txCtrl = _rfid.readSPI(0x14);
    _rfid.writeSPI(0x14, txCtrl & ~0x03);
}

uint8_t Rfid::getLongitud(){
    return _longitud;
}

uint8_t Rfid::getByteUID(int indice){
    if (indice >= 0 && indice < 8) return _uid[indice];
    return 0; 
}