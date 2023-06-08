#ifndef __EASY_BLE_H__
#define __EASY_BLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "sdkconfig.h"

#include "espidf_ble_ota.h"

typedef void (*easy_ble_callback_t)(uint8_t *rxValue, uint8_t len,esp_gatt_if_t *gatts_if, uint16_t conn_id, uint16_t attr_handle);
void easy_ble_set_rx_callback(easy_ble_callback_t c);

void easy_ble_init(uint8_t *mac_addr);
void easy_ble_stop(void);

#endif