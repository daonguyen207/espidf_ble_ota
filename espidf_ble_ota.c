#include "espidf_ble_ota.h"

static const char *TAG = "ble_ota";

uint32_t ota_fw_size,ota_fw_counter;
uint32_t ota_download_paket;
uint32_t ota_state;
uint32_t ota_tranfer_mode;
int couter_process = 0;
ota_callback_t begin_callback;
ota_callback_t proces_callback;
ota_callback_t end_callback;
ota_callback_t error_callback;
static esp_err_t err;
static esp_ota_handle_t update_handle = 0 ;
static esp_partition_t *update_partition = NULL;
static esp_partition_t *configured = NULL;
static esp_partition_t *running = NULL;

void ble_ota_reset_task(void *pvParameters)
{
    for(;;)
    {
        if(ota_state == OTA_DOWNLOADDONE)
        {
            ESP_LOGI(TAG, "Reset...");
            vTaskDelay(500/portTICK_PERIOD_MS);
            esp_restart();
            vTaskDelete(NULL); 
        }
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
void ble_ota_init(void)
{
  xTaskCreate(&ble_ota_reset_task, "ble_ota_reset_task", 1024, NULL, 5, NULL);
}

void iot47_ble_ota_set_begin_callback(ota_callback_t c)
{
  begin_callback = c;
}
void iot47_ble_ota_set_proces_callback(ota_callback_t c)
{
  proces_callback = c;
}
void iot47_ble_ota_set_end_callback(ota_callback_t c)
{
  end_callback = c;
}
void iot47_ble_ota_set_error_callback(ota_callback_t c)
{
  error_callback = c;
}
void iot47_stop_ota(void)
{
  ota_state = OTA_BEGIN;
}
int iot47_ble_ota_task(uint8_t *rxValue, uint8_t len,
                   esp_gatt_if_t *gatts_if, uint16_t conn_id, uint16_t attr_handle
)
{
  if(ota_state == OTA_BEGIN)
  {
    if (len > 20 && len < 40)  //IOT47_BLE_OTA_BEGIN:1234567\r\n
    {
      if((rxValue[0] == 'I') && (rxValue[1] == 'O') && (rxValue[2] == 'T') && (rxValue[3] == '4') && (rxValue[4] == '7'))
      {
        uint8_t *header = (uint8_t *)malloc(len); 
        for (int i = 0; i < len; i++)header[i] = rxValue[i];
        uint8_t *ota_cmd = (uint8_t *)strstr((const char *)header,(const char *)"IOT47_BLE_OTA_BEGIN:"); //find header
        if(ota_cmd != 0)
        {
          ota_fw_size=0;
          for(int i=0;i<20;i++)
          {
            if((ota_cmd[20 + i] == '\r') || (ota_cmd[20 + i] == '\n'))
            {
                free(header);    
                //chuẩn bị ota
                update_handle = 0 ;
                update_partition = NULL;
                configured = esp_ota_get_boot_partition();
                running = esp_ota_get_running_partition();
                if (configured != running) {
                    ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",configured->address, running->address);
                    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
                }
                ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",running->type, running->subtype, running->address);
                update_partition = esp_ota_get_next_update_partition(NULL);
                assert(update_partition != NULL);
                ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",update_partition->subtype, update_partition->address);
                err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

                if (err != ESP_OK) {
                    esp_ota_abort(update_handle);
                    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                    return ESP_FAIL;
                }

                ota_state = OTA_DOWNLOADDING;
                ota_fw_counter = 0;
                ota_download_paket = 0;
                ESP_LOGW(TAG,"bat dau ota ble");
                esp_ble_gatts_send_indicate(*gatts_if, conn_id,attr_handle,4, (uint8_t *)"OK\r\n", true);
                if(begin_callback!=0)begin_callback(ota_fw_counter,ota_fw_size);           
                return 1;
            }
            ota_fw_size*=10;
            ota_fw_size+= ota_cmd[20 + i]-48;
          }
        }
        free(header);
      }
    }
  }
  else if(ota_state == OTA_DOWNLOADDING)
  {
    // [0][1] = số thứ tự gói tin     |      [2][3] = size payload   |       [4]...[n] play load
    uint16_t packet = ((uint16_t)rxValue[0]<<8) | (uint16_t)rxValue[1];
    if(packet == ota_download_paket)
    {
      ota_download_paket++;
      uint16_t size = ((uint16_t)rxValue[2]<<8) | (uint16_t)rxValue[3];
      err = esp_ota_write( update_handle, (const void *)&(rxValue[4]), size);
      if (err != ESP_OK) {
        esp_ota_abort(update_handle);
        ESP_LOGE(TAG, "Lỗi khi ghi tệp vào bộ nhớ");
        return ESP_FAIL;
      }
      ota_fw_counter+=size;
      couter_process++;
      if(couter_process==20)
      {
        couter_process=0;
        if(proces_callback!=0)proces_callback(ota_fw_counter,ota_fw_size);
      }
      if(ota_fw_counter == ota_fw_size)
      {
        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
            if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            } else {
                ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            }
            ESP_LOGE(TAG, "Lỗi khi kết thúc ota");
            esp_ble_gatts_send_indicate(*gatts_if, conn_id,attr_handle,6, (uint8_t *)"FAIL\r\n", true);
            return ESP_FAIL;
        }
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            ESP_LOGE(TAG, "Lỗi khi set boot partition");
            esp_ble_gatts_send_indicate(*gatts_if, conn_id,attr_handle,6, (uint8_t *)"FAIL\r\n", true);
            return ESP_FAIL;
        }
        esp_ble_gatts_send_indicate(*gatts_if, conn_id,attr_handle,10, (uint8_t *)"OTA DONE\r\n", true);
        ota_state = OTA_DOWNLOADDONE;
        if(end_callback!=0)end_callback(ota_fw_counter,ota_fw_size);
        return 3;
      }
    }
    else
    {
      ESP_LOGE(TAG, "Lỗi khi OTA");
      esp_ble_gatts_send_indicate(*gatts_if, conn_id,attr_handle,6, (uint8_t *)"FAIL\r\n", true);
      if(error_callback!=0)error_callback(ota_fw_counter,ota_fw_size);
    }
    return 2;
  }
  return 0;
}