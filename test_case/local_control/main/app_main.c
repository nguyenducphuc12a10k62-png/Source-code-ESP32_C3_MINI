#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_storage.h"
#include "app_driver.h"
#include "app_wifi.h"
#include "app_webserver.h"


static const char *TAG = "app_main";

/**
 * @brief Hàm này được gọi bởi component app_wifi khi đã kết nối mạng thành công.
 */
void on_wifi_connected(void)
{
    ESP_LOGI(TAG, "Wi-Fi Connected, starting Web Server...");
    // Khởi động webserver chỉ sau khi đã có mạng
    ESP_ERROR_CHECK(app_webserver_start());
}

void app_main(void)
{
    ESP_LOGI(TAG, "NVS init");
    app_storage_init();

    ESP_LOGI(TAG, "Driver init");
    app_driver_init();

    ESP_LOGI(TAG, "Wi-Fi init");
    // Gọi component Wi-Fi và "hẹn" nó gọi lại hàm on_wifi_connected khi xong
    app_wifi_init(on_wifi_connected);

    // Vòng lặp chính của chương trình
    int i=0;
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Main task is running... [%d]", i++);
    }
}