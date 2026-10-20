/* main/udp_client_button.c */
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <arpa/inet.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "driver/gpio.h"

#include "app_storage.h"
#include "app_priv.h"

/* ====== Cấu hình hệ thống ====== */
#define LIGHT_UDP_CLIENT      1 // <<< THAY ĐỔI: Từ TCP sang UDP
#define LIGHT_ESP_WIFI_SSID   "MangcuaAn"
#define LIGHT_ESP_WIFI_PASS   "20092004"
#define LIGHT_ESP_MAXIMUM_RETRY 5
#define HOST_IP               "172.31.101.81" // IP máy tính Server
#define PORT                  3333

// <<< THÊM: Cấu hình cho nút nhấn (Giữ nguyên) >>>
#define BUTTON_PIN    GPIO_NUM_9
static int press_count = 0; // Biến đếm số lần nhấn

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "udp socket"; // <<< THAY ĐỔI: Tag log

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group = NULL;
static int s_retry_num = 0;

/* ===== Xử lý sự kiện và khởi tạo Wi-Fi (Giữ nguyên) ===== */
// (Không có thay đổi trong các hàm event_handler, wifi_initialize, wifi_station_initialize)
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < LIGHT_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_initialize(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
}

static void wifi_station_initialize(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = LIGHT_ESP_WIFI_SSID,
            .password = LIGHT_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_station_initialize finished.");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", LIGHT_ESP_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", LIGHT_ESP_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

/* ===== Hàm Server (Cần được viết lại cho UDP nếu muốn dùng) ===== */
static esp_err_t esp_create_udp_server(void)
{
    // TODO: Viết lại logic của UDP server ở đây nếu bạn muốn chạy chế độ server
    ESP_LOGW(TAG, "UDP server function is not implemented yet.");
    return ESP_OK;
}

/* ===== HÀM CLIENT ĐÃ CHUYỂN SANG UDP ===== */
static esp_err_t esp_create_udp_client(const char *message) // <<< SỬA: Đổi tên hàm
{
    esp_err_t err = ESP_FAIL;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    // Tạo UDP Socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0); // <<< SỬA: SOCK_DGRAM cho UDP
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return err;
    }
    ESP_LOGI(TAG, "Socket created");

    // <<< SỬA: Không cần connect() trong UDP >>>

    // Gửi UDP dữ liệu
    int ret = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); // <<< SỬA: Dùng sendto()
    if (ret < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        goto exit; // Sử dụng goto để đảm bảo socket luôn được đóng
    }
    ESP_LOGI(TAG, "Sent message successfully: %s", message);
    
    err = ESP_OK;

exit:
    // <<< SỬA: Không cần shutdown() trong UDP >>>
    close(sock);
    return err;
}

/* ===== Task đọc nút nhấn (Chỉ sửa lời gọi hàm) ===== */
static void button_task(void *pvParameters)
{
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN);

    int last_state = 1;

    while (1)
    {
        int current_state = gpio_get_level(BUTTON_PIN);

        if (last_state == 1 && current_state == 0)
        {
            press_count++;
            char msg[64];
            snprintf(msg, sizeof(msg), "Button pressed: %d", press_count);

            // Gửi thông điệp UDP
            esp_create_udp_client(msg); // <<< SỬA: Gọi hàm UDP client

            vTaskDelay(pdMS_TO_TICKS(300));
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ===== Main App (Cập nhật để gọi hàm UDP) ===== */
void app_main()
{
    int i = 0;
    ESP_LOGI(TAG, "app_main");

    ESP_LOGI(TAG, "NVS Flash initialization");
    nvs_flash_init(); // Dùng hàm gốc của esp-idf cho chắc chắn

    ESP_LOGI(TAG, "Wi-Fi initialization");
    wifi_initialize();

    ESP_LOGI(TAG, "Wi-Fi Station initialization");
    wifi_station_initialize();

#if LIGHT_UDP_CLIENT // <<< THAY ĐỔI: Sử dụng macro mới
    // Tạo task theo dõi nút nhấn và gửi UDP
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
#else
    esp_create_udp_server(); // <<< SỬA: Gọi hàm UDP server
#endif

    while (1) {
        ESP_LOGI(TAG, "[%02d] System running (Heartbeat)...", i++);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}