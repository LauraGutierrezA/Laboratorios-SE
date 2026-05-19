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
    for(int i = 0; i<8; i++) _uid[i] = 0; 
}

Rfid::~Rfid(){

}

void Rfid::init(){
    _rfid.writeSPI(0x01, 0x0F); //Resetea y limpia registros
    vTaskDelay(pdMS_TO_TICKS(50)); 
    _rfid.writeSPI(0x12, 0x80); //Configura TxmodeReg
    _rfid.writeSPI(0x13, 0x80); //Configura RxModeReg
    _rfid.writeSPI(0x14, 0x83); //Configura Antena a Máxima potencia
}

void Rfid::buscarTarjeta(){
    _rfid.writeSPI(0x01, 0x00); //IDLE
    _rfid.writeSPI(0x0A, 0x80); //Vaciar FIFO Buffer
    _rfid.writeSPI(0x09, 0x52); // "despierten tarjetas"
    _rfid.writeSPI(0x01, 0x0C); //Transceive (preparar eleer y escribir)
    _rfid.writeSPI(0x0D, 0x80); //Greenlight para empezar a escribir
}

bool Rfid::procesarTarjeta(){
    uint8_t errorStatus = _rfid.readSPI(0x06); 
    if (errorStatus & 0x1D){ //Cualquiera de los errores del registro 0x06
        _rfid.writeSPI(0x01, 0x00); //Volver a IDLE
        return false;
    }

    _longitud = _rfid.readSPI(0x0A); // Verificar long no vacía
    if (_longitud == 0) {
        _rfid.writeSPI(0x01, 0x00); //Volver a IDLE
        return false; 
    }

    for(int i = 0; i < _longitud; i++){ //Guardar el _uid[]
        _uid[i] = _rfid.readSPI(0x09); 
    }

    _rfid.writeSPI(0x01, 0x00); 
    return true;
}

bool Rfid::leerTarjeta() {
    buscarTarjeta(); 
    return procesarTarjeta();
}


bool Rfid::verificarTarjeta(const uint8_t* uidCard){
    if (_longitud == 0) return false; 

    for (int i=0; i < _longitud; i++){
        if(_uid[i] != uidCard[i]){
            return false;
        }
    }
    return true;
}

uint8_t Rfid::getLongitud(){
    return _longitud;
}

uint8_t Rfid::getByteUID(int indice){
    if (indice >= 0 && indice < 8) return _uid[indice];
    return 0; 
}