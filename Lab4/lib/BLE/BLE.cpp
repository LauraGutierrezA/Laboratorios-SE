#include "BleNus.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_CLASS";

// --- Variables estáticas globales necesarias por NimBLE ---
static uint8_t own_addr_type;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t nus_tx_val_handle;
static BleNus* _instanciaBle = nullptr; // Puntero mágico para conectar C con C++

// UUIDs oficiales del Nordic UART Service
static const ble_uuid128_t nus_service_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

static const ble_uuid128_t nus_rx_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

static const ble_uuid128_t nus_tx_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

// --- Declaración anticipada de funciones ---
static void ble_app_advertise(void);

// --- Funciones Callback de NimBLE (Estilo C) ---

int nus_rx_access_cb(uint16_t conn_handle_param, uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt, void *arg) {
    uint16_t len = 0;
    
    // Si no hay instancia creada, no hacemos nada
    if (_instanciaBle == nullptr) return 0; 

    int rc = ble_hs_mbuf_to_flat(ctxt->om, _instanciaBle->_ultimoMensaje, sizeof(_instanciaBle->_ultimoMensaje) - 1, &len);
    if (rc == 0) {
        _instanciaBle->_ultimoMensaje[len] = '\0'; // Aseguramos fin de string
        _instanciaBle->_hayDatoNuevo = true;       // Levantamos la bandera de que hay datos
        ESP_LOGI(TAG, "Recibido: %s", _instanciaBle->_ultimoMensaje);
    }
    return 0;
}

static int nus_tx_access_cb(uint16_t conn_handle_param, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    return 0;
}

int ble_gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                conn_handle = event->connect.conn_handle;
                if (_instanciaBle) _instanciaBle->_conectado = true;
                ESP_LOGI(TAG, "Cliente conectado");
            } else {
                ble_app_advertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            if (_instanciaBle) {
                _instanciaBle->_conectado = false;
                _instanciaBle->_notificacionesActivas = false;
            }
            ESP_LOGI(TAG, "Cliente desconectado");
            ble_app_advertise();
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            if (event->subscribe.attr_handle == nus_tx_val_handle) {
                if (_instanciaBle) _instanciaBle->_notificacionesActivas = event->subscribe.cur_notify;
            }
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ble_app_advertise();
            return 0;

        default:
            return 0;
    }
}

// Configuración de la tabla GATT
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &nus_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &nus_rx_uuid.u,
                .access_cb = nus_rx_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = &nus_tx_uuid.u,
                .access_cb = nus_tx_access_cb,
                .val_handle = &nus_tx_val_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 }
        },
    },
    { 0 }
};

static void ble_app_advertise(void) {
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    struct ble_gap_adv_params adv_params;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = (ble_uuid128_t *)&nus_service_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    memset(&rsp_fields, 0, sizeof(rsp_fields));
    const char *name = ble_svc_gap_device_name();
    rsp_fields.name = (uint8_t *)name;
    rsp_fields.name_len = strlen(name);
    rsp_fields.name_is_complete = 1;
    ble_gap_adv_rsp_set_fields(&rsp_fields);

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
}

static void ble_app_on_sync(void) {
    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_app_advertise();
}

static void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}


// === IMPLEMENTACIÓN DE LA CLASE BleNus ===

BleNus::BleNus() {
    _conectado = false;
    _notificacionesActivas = false;
    _hayDatoNuevo = false;
    memset(_ultimoMensaje, 0, sizeof(_ultimoMensaje));
    _instanciaBle = this; // Guardamos esta instancia para que las funciones de C la vean
}

BleNus::~BleNus() {
    _instanciaBle = nullptr;
}

void BleNus::init(const char* nombreDispositivo) {
    // Inicializar NVS (Requisito de NimBLE)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    nimble_port_init();

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(nombreDispositivo);

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    nimble_port_freertos_init(ble_host_task);
}

void BleNus::enviar(const char* mensaje) {
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE || !_notificacionesActivas) {
        return; // No hay a quien enviarle
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(mensaje, strlen(mensaje));
    if (om != NULL) {
        ble_gatts_notify_custom(conn_handle, nus_tx_val_handle, om);
    }
}

bool BleNus::leer(char* buffer) {
    if (_hayDatoNuevo) {
        strcpy(buffer, _ultimoMensaje); // Copiamos el dato al buffer del usuario
        _hayDatoNuevo = false;          // Bajamos la bandera
        return true;
    }
    return false;
}

bool BleNus::estaConectado() {
    return _conectado;
}