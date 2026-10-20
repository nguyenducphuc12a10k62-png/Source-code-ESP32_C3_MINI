/*
   App driver: cấu hình nút nhấn + điều khiển đèn qua light_driver
   ESP32-C3 RainMaker Light Example - COMPLETE VERSION
*/
#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "app_priv.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "iot_button.h"
#include "light_driver.h"
#include "app_wifi.h"

#include "board_esp32c3_devkitc.h"
#include "app_priv.h"

/* ========== DEFAULT VALUES ========== */
#define DEFAULT_POWER        false
#define DEFAULT_BRIGHTNESS   50
#define DEFAULT_HUE          0
#define DEFAULT_SATURATION   100

static const char *TAG = "app_driver";

/* Biến toàn cục lưu trạng thái đèn */
static bool g_light_power = false;
static int g_light_brightness = 50;
static int g_light_hue = 0;
static int g_light_saturation = 0;

/* ========== HÀM CHUYỂN ĐỔI HSV to RGB ========== */
static void hsv_to_rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    /* Giới hạn giá trị đầu vào */
    h = h % 360;
    if (h < 0) h = 0;
    s = (s < 0) ? 0 : (s > 100) ? 100 : s;
    v = (v < 0) ? 0 : (v > 100) ? 100 : v;
    
    float hue = h / 60.0f;
    float saturation = s / 100.0f;
    float value = v / 100.0f;
    
    int i = (int)hue;
    float f = hue - i;
    float p = value * (1 - saturation);
    float q = value * (1 - saturation * f);
    float t = value * (1 - saturation * (1 - f));
    
    float red, green, blue;
    
    switch(i) {
        case 0: red = value; green = t; blue = p; break;
        case 1: red = q; green = value; blue = p; break;
        case 2: red = p; green = value; blue = t; break;
        case 3: red = p; green = q; blue = value; break;
        case 4: red = t; green = p; blue = value; break;
        case 5: red = value; green = p; blue = q; break;
        default: red = value; green = p; blue = q; break;
    }
    
    *r = (uint8_t)(red * 255);
    *g = (uint8_t)(green * 255);
    *b = (uint8_t)(blue * 255);
    
    ESP_LOGD(TAG, "HSV(%d, %d, %d) -> RGB(%d, %d, %d)", h, s, v, *r, *g, *b);
}

/* Cập nhật LED dựa trên HSV hiện tại - CHỈ DÙNG LIGHT DRIVER */
static void update_led_from_hsv(void)
{
    if (!g_light_power) {
        ESP_LOGI(TAG, "Power OFF - setting RGB to 0");
        light_driver_set_rgb(0, 0, 0);
        light_driver_set_switch(false);
        return;
    }
    
    uint8_t r, g, b;
    hsv_to_rgb(g_light_hue, g_light_saturation, g_light_brightness, &r, &g, &b);
    
    ESP_LOGI(TAG, "Power ON - Setting RGB(%d, %d, %d)", r, g, b);
    
    // DÙNG LIGHT DRIVER - hỗ trợ PWM
    light_driver_set_switch(true);
    light_driver_set_rgb(r, g, b);
    
    // Gọi lại sau delay để đảm bảo
    vTaskDelay(pdMS_TO_TICKS(20));
    light_driver_set_rgb(r, g, b);
}

/* ========== CALLBACK CHO NÚT BOOT ========== */

/* Callback khi nhấn nút ngắn - toggle đèn */
static void button_single_press_cb(void *arg)
{
    ESP_LOGI(TAG, "Button pressed - toggling light");
    app_light_set_power(!g_light_power);
}

/* Callback khi nhấn giữ nút Boot 10 giây - FACTORY RESET */
static void button_long_press_cb(void *arg)
{
    ESP_LOGI(TAG, "Boot button long pressed - FACTORY RESET!");
    
    /* Nhấp nháy LED báo hiệu (màu đỏ) */
    for(int i = 0; i < 5; i++) {
        light_driver_set_rgb(255, 0, 0); // Đỏ
        vTaskDelay(pdMS_TO_TICKS(200));
        light_driver_set_rgb(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    /* FACTORY RESET - XÓA TOÀN BỘ NVS */
    ESP_LOGI(TAG, "Performing factory reset...");
    
    // Xóa toàn bộ partition NVS
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS flash erased completely - QR code will appear after reboot");
    } else {
        ESP_LOGE(TAG, "Failed to erase NVS: %d", err);
        
        // Fallback: Xóa tất cả keys trong NVS
        nvs_handle_t nvs_handle;
        if (nvs_open("nvs", NVS_READWRITE, &nvs_handle) == ESP_OK) {
            nvs_erase_all(nvs_handle);
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
            ESP_LOGI(TAG, "All NVS keys erased");
        }
    }
    
    /* Restart */
    ESP_LOGI(TAG, "Rebooting in 3 seconds to show QR code...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}

/* ========== HÀM TEST LED VỚI LIGHT DRIVER ========== */

/* Test LED với light_driver */
void app_test_led_pwm(void)
{
    ESP_LOGI(TAG, "=== LED TEST ===");
    
    // Test các mức brightness
    light_driver_set_switch(true);
    
    // Brightness thấp
    ESP_LOGI(TAG, "Testing LOW brightness");
    light_driver_set_rgb(25, 25, 25); // ~10%
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Brightness trung bình
    ESP_LOGI(TAG, "Testing MEDIUM brightness");
    light_driver_set_rgb(127, 127, 127); // ~50%
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Brightness cao
    ESP_LOGI(TAG, "Testing HIGH brightness");
    light_driver_set_rgb(255, 255, 255); // 100%
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Test màu sắc
    ESP_LOGI(TAG, "Testing colors");
    light_driver_set_rgb(127, 0, 0); // Đỏ
    vTaskDelay(pdMS_TO_TICKS(1000));
    light_driver_set_rgb(0, 127, 0); // Xanh lá
    vTaskDelay(pdMS_TO_TICKS(1000));
    light_driver_set_rgb(0, 0, 127); // Xanh dương
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Tắt
    ESP_LOGI(TAG, "Turning OFF");
    light_driver_set_switch(false);
    light_driver_set_rgb(0, 0, 0);
    
    ESP_LOGI(TAG, "=== LED TEST COMPLETE ===");
}

/* ========== KHỞI TẠO DRIVER ========== */

void app_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing drivers...");

    /* 1. KHỞI TẠO NÚT BOOT */
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = 9,  // Nút Boot trên ESP32-C3 DevKitC
        },
    };

    button_handle_t btn_handle = iot_button_create(&btn_cfg);
    if (btn_handle) {
        // Nhấn ngắn: toggle đèn
        iot_button_register_cb(btn_handle, 0, button_single_press_cb, NULL);
        
        // Nhấn giữ: factory reset
        iot_button_register_cb(btn_handle, 1, button_long_press_cb, NULL);
        
        ESP_LOGI(TAG, "Boot button initialized");
        ESP_LOGI(TAG, "  - Short press: Toggle light");
        ESP_LOGI(TAG, "  - Hold 10s: FACTORY RESET & show QR code");
    } else {
        ESP_LOGE(TAG, "Failed to initialize boot button");
    }

    /* 2. Khởi tạo light driver */
    esp_err_t ret = light_driver_init(LIGHT_GPIO_RED, LIGHT_GPIO_GREEN, LIGHT_GPIO_BLUE);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Light driver initialized - R:%d, G:%d, B:%d", 
                 LIGHT_GPIO_RED, LIGHT_GPIO_GREEN, LIGHT_GPIO_BLUE);
        
        /* Mặc định tắt đèn khi khởi động */
        light_driver_set_switch(false);
        light_driver_set_rgb(0, 0, 0);
        g_light_power = false;
        
        ESP_LOGI(TAG, "Light initialized to OFF state");
    } else {
        ESP_LOGE(TAG, "Failed to initialize light driver: %d", ret);
    }

    /* 3. TEST LED */
    ESP_LOGI(TAG, "Testing LEDs...");
    app_test_led_pwm();
}

/* ========== HÀM ĐIỀU KHIỂN ĐÈN CHO RAINMAKER ========== */

/* Bật/tắt đèn */
esp_err_t app_light_set_power(bool power)
{
    ESP_LOGI(TAG, "=== app_light_set_power: %s ===", power ? "ON" : "OFF");

    g_light_power = power;

    if (power) {
        ESP_LOGI(TAG, "Turning ON light");

        // --- BỔ SUNG: TREO KHÓA TRƯỚC KHI BẬT ĐÈN ---
        app_pm_lock_led();

        light_driver_set_switch(true);
        vTaskDelay(pdMS_TO_TICKS(30)); // Chờ nguồn ổn định
        update_led_from_hsv();
    } else {
        ESP_LOGI(TAG, "Turning OFF light");

        light_driver_set_switch(false);
        light_driver_set_rgb(0, 0, 0);

        // --- BỔ SUNG: THẢ KHÓA SAU KHI TẮT ĐÈN ---
        app_pm_release_led();
    }

    return ESP_OK;
}

/* Điều chỉnh độ sáng (0-100) */
esp_err_t app_light_set_brightness(int brightness)
{
    ESP_LOGI(TAG, "Setting brightness: %d", brightness);
    
    /* Giới hạn giá trị */
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
    
    g_light_brightness = brightness;
    
    /* Cập nhật LED */
    update_led_from_hsv();
    
    return ESP_OK;
}

/* Điều chỉnh màu sắc (Hue) */
esp_err_t app_light_set_hue(int hue)
{
    ESP_LOGI(TAG, "Setting hue: %d", hue);
    
    /* Giới hạn giá trị */
    if (hue < 0) hue = 0;
    if (hue > 360) hue = 360;
    
    g_light_hue = hue;
    
    /* Cập nhật LED */
    update_led_from_hsv();
    
    return ESP_OK;
}

/* Điều chỉnh độ bão hòa màu */
esp_err_t app_light_set_saturation(int saturation)
{
    ESP_LOGI(TAG, "Setting saturation: %d", saturation);
    
    /* Giới hạn giá trị */
    if (saturation < 0) saturation = 0;
    if (saturation > 100) saturation = 100;
    
    g_light_saturation = saturation;
    
    /* Cập nhật LED */
    update_led_from_hsv();
    
    return ESP_OK;
}

/* ========== HÀM LẤY TRẠNG THÁI ĐÈN ========== */

/* Lấy trạng thái bật/tắt */
bool app_light_get_power(void)
{
    return g_light_power;
}

/* Lấy giá trị độ sáng hiện tại */
int app_light_get_brightness(void)
{
    return g_light_brightness;
}

/* Lấy giá trị màu sắc hiện tại */
int app_light_get_hue(void)
{
    return g_light_hue;
}

/* Lấy giá trị độ bão hòa hiện tại */
int app_light_get_saturation(void)
{
    return g_light_saturation;
}

/* ========== HÀM TIỆN ÍCH ========== */

/* Set trạng thái đèn */
esp_err_t app_driver_set_state(bool state)
{
    return app_light_set_power(state);
}

/* Get trạng thái đèn */
bool app_driver_get_state(void)
{
    return app_light_get_power();
}

/* Hàm test màu sắc từ RainMaker */
void app_light_test_colors(void)
{
    ESP_LOGI(TAG, "=== RAINMAKER COLOR TEST ===");
    
    app_light_set_power(true);
    app_light_set_brightness(80);
    app_light_set_saturation(100);

    // Test các màu cơ bản
    int test_colors[] = {0, 60, 120, 180, 240, 300};
    const char *color_names[] = {"RED", "YELLOW", "GREEN", "CYAN", "BLUE", "MAGENTA"};
    
    for (int i = 0; i < 6; i++) {
        ESP_LOGI(TAG, "Testing %s", color_names[i]);
        app_light_set_hue(test_colors[i]);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    // Quay về mặc định
    app_light_set_hue(0);
    app_light_set_saturation(100);
    app_light_set_brightness(50);
    
    ESP_LOGI(TAG, "=== COLOR TEST COMPLETE ===");
}