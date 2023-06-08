# espidf_ble_ota
Thư viện ota qua ble cho esp32 idf. Tải app BLE OTA trên apk và ios.

APK: https://play.google.com/store/apps/details?id=com.esp32.ble.ota

Phiên bản cho arduino: https://github.com/daonguyen207/arduino_ble_ota

# Sử dụng:
1. Vào sdk config, bật bluetooth
2. Add thư viện espidf_ble_ota.h và easy_ble.h

# Sử dụng với lớp bọc easy_ble
easy_ble là lớp bọc khởi tạo bluetooth giúp bạn nhanh chóng sử dụng thư viện chỉ với 3 dòng khởi tạo. Tuy nhiên bạn sẽ ít có quyền can thiệp vào việc khởi tạo ble hơn
```
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "espidf_ble_ota.h"
#include "easy_ble.h"
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    easy_ble_init(mac);

    ble_ota_init();
}
```
# Đăng kí các callback
Dưới dây là các callback trong quá trình ota, bạn có thể sử dụng để biết qua trình ota đang diễn ra như nào ( Ví dụ in % download ra màn hình LCD hoặc thông báo lỗi ...)
```
typedef void (*ota_callback_t)(uint32_t curen, uint32_t totol);
void iot47_ble_ota_set_begin_callback(ota_callback_t c);
void iot47_ble_ota_set_proces_callback(ota_callback_t c);
void iot47_ble_ota_set_end_callback(ota_callback_t c);
void iot47_ble_ota_set_error_callback(ota_callback_t c);
```
Trong trường hợp bạn muốn xử lí các dữ liệu rx ble phục vụ cho ứng dụng của bạn, có thể đăng kí rx_callback
```
typedef void (*easy_ble_callback_t)(uint8_t *rxValue, uint8_t len,esp_gatt_if_t *gatts_if, uint16_t conn_id, uint16_t attr_handle);
void easy_ble_set_rx_callback(easy_ble_callback_t c);
```
 iot47_ble_ota_task vào trong sự kiện 
# Không sử dụng lớp bọc easy_ble
Bằng cách sử dụng trực tiếp các api của espidf_ble_ota, bạn có thể toàn quyền khởi tạo ble và sử dụng ble theo ý muốn bạn, tuy nhiên sẽ cần 1 chút kiến thức về ble. Chỉ cần đặt hàm iot47_ble_ota_task vào trong sự kiện ESP_GATTS_WRITE_EVT
Ví dụ:
```
case ESP_GATTS_WRITE_EVT: {
        if (!param->write.is_prep){ 
            if(iot47_ble_ota_task(param->write.value,param->write.len,&gatts_if,param->write.conn_id,param->write.handle) == 0)
            {
                //user code
                ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT A, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
                ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT A, value len %d, value :", param->write.len);
                esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);   
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
```
Hàm này là hàm ưu tiên nên hãy đặt nó ở đầu. Nếu nó trả về khác 0 , tức là quá trình OTA đang diễn ra, do vậy bạn chỉ xử lí các tác vụ ble của user nếu hàm này trả về 0
Và để an toàn hơn, hãy đặt  iot47_stop_ota(); vào sự kiện ESP_GATTS_DISCONNECT_EVT.
Sau khi khởi tạo ble xong, chỉ cần gọi ble_ota_init();

