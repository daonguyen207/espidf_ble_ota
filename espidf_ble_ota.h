/* Thư viện espidf_ble_ota
   Người viết: Daonguyen IoT47
   Email: daonguyen20798@gmail.com
   Website: Iot47.com
   Link thư viện: https://github.com/daonguyen207/espidf_ble_ota
   Sử dụng: 

*/
#ifndef _IOT47_BLE_OTA_H_
#define _IOT47_BLE_OTA_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "string.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

#define OTA_BEGIN        0
#define OTA_DOWNLOADDING 1
#define OTA_DOWNLOADDONE 2

typedef void (*ota_callback_t)(uint32_t curen, uint32_t totol);

int iot47_ble_ota_task(uint8_t *rxValue, uint8_t len,
                   esp_gatt_if_t *gatts_if, uint16_t conn_id, uint16_t attr_handle
);
void iot47_stop_ota(void);
void ble_ota_init(void);

#endif