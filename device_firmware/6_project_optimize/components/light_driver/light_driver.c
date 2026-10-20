#include "light_driver.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <stdint.h>  // ĐÃ THÊM

// <--- BỔ SUNG INCLUDE CHO POWER MANAGEMENT --->
#include "esp_pm.h"
// <--- KẾT THÚC BỔ SUNG --->

static const char *TAG = "light_driver";

static bool light_on = false;

// <--- BỔ SUNG BIẾN STATIC CHO PM LOCK --->
static esp_pm_lock_handle_t s_pm_apb_lock = NULL;
// <--- KẾT THÚC BỔ SUNG --->

// Lưu cấu hình 3 kênh LEDC cho R, G, B
static ledc_channel_config_t s_ledc_ch[3];

esp_err_t light_driver_init(int gpio_r, int gpio_g, int gpio_b)
{
    // 1) Cấu hình timer LEDC (low speed, 12-bit, 5 kHz)
    ledc_timer_config_t tcfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT, // 0..4095
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    // 2) Cấu hình 3 kênh cho 3 chân RGB
    const int gpios[3] = { gpio_r, gpio_g, gpio_b };
    for (int i = 0; i < 3; ++i) {
        s_ledc_ch[i].speed_mode = LEDC_LOW_SPEED_MODE;
        s_ledc_ch[i].channel    = (ledc_channel_t)i;   // LEDC_CHANNEL_0/1/2
        s_ledc_ch[i].timer_sel  = LEDC_TIMER_0;
        s_ledc_ch[i].intr_type  = LEDC_INTR_DISABLE;
        s_ledc_ch[i].gpio_num   = gpios[i];
        s_ledc_ch[i].duty       = 0;
        s_ledc_ch[i].hpoint     = 0;
        ESP_ERROR_CHECK(ledc_channel_config(&s_ledc_ch[i]));
    }

    // 3) Cài fade function một lần
    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    // <--- BỔ SUNG: KHỞI TẠO PM LOCK --->
    if (s_pm_apb_lock == NULL) {
        if (esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "l_apb_lock", &s_pm_apb_lock) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create PM lock");
        } else {
            ESP_LOGI(TAG, "Created PM lock for APB frequency");
        }
    }
    // <--- KẾT THÚC BỔ SUNG --->

    ESP_LOGI(TAG, "Light driver initialized (R=%d, G=%d, B=%d)", gpio_r, gpio_g, gpio_b);
    return ESP_OK;
}

esp_err_t light_driver_set_switch(bool on)
{
    // === BỔ SUNG QUAN TRỌNG ===
    // 1. Kiểm tra xem có cần thay đổi trạng thái không
    if (on == light_on) {
        ESP_LOGD(TAG, "Light state already %s, nothing to do.", on ? "ON" : "OFF");
        return ESP_OK; // Không làm gì cả nếu trạng thái đã đúng
    }
    // ==========================

    // 2. Nếu 'on' là true (và light_on đang là false) -> BẬT
    if (on) {
        // <--- BỔ SUNG: ACQUIRE LOCK KHI BẬT --->
        if (s_pm_apb_lock) {
            ESP_ERROR_CHECK(esp_pm_lock_acquire(s_pm_apb_lock));
            ESP_LOGD(TAG, "PM lock acquired"); // Dùng LOGD để đỡ spam log
        }
        // <--- KẾT THÚC BỔ SUNG --->
        light_on = true; // Cập nhật trạng thái
        // Đèn sẽ được bật sáng thực sự khi hàm set_rgb/fade_rgb được gọi
    } else { // 'on' là false (và light_on đang là true) -> TẮT
        light_on = false; // Cập nhật trạng thái OFF trước
        // Tắt: kéo duty về 0 cho cả 3 kênh
        for (int i = 0; i < 3; ++i) {
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)i, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)i));
        }

        // <--- BỔ SUNG: RELEASE LOCK KHI TẮT --->
        if (s_pm_apb_lock) {
            ESP_ERROR_CHECK(esp_pm_lock_release(s_pm_apb_lock));
            ESP_LOGD(TAG, "PM lock released"); // Dùng LOGD
        }
        // <--- KẾT THÚC BỔ SUNG --->
    }
    ESP_LOGI(TAG, "Light %s", on ? "ON" : "OFF");
    return ESP_OK;
}

esp_err_t light_driver_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!light_on) {
        ESP_LOGW(TAG, "Light is off, cannot set RGB");
        return ESP_OK;
    }
    // 12-bit duty: scale 0..255 -> 0..4095
    const uint32_t duty[3] = {
        (uint32_t)r * 16U,
        (uint32_t)g * 16U,
        (uint32_t)b * 16U
    };
    for (int i = 0; i < 3; ++i) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)i, duty[i]));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)i));
    }
    ESP_LOGI(TAG, "RGB set to (%d, %d, %d)", r, g, b);
    return ESP_OK;
}

esp_err_t light_driver_fade_rgb(uint8_t r, uint8_t g, uint8_t b, int fade_time_ms)
{
    if (!light_on) {
        ESP_LOGW(TAG, "Light is off, cannot fade RGB");
        return ESP_OK;
    }
    const uint32_t duty[3] = {
        (uint32_t)r * 16U,
        (uint32_t)g * 16U,
        (uint32_t)b * 16U
    };
    for (int i = 0; i < 3; ++i) {
        ESP_ERROR_CHECK(ledc_set_fade_time_and_start(
            LEDC_LOW_SPEED_MODE,
            (ledc_channel_t)i,
            duty[i],
            fade_time_ms,
            LEDC_FADE_NO_WAIT));
    }
    ESP_LOGI(TAG, "RGB fading to (%d, %d, %d) in %dms", r, g, b, fade_time_ms);
    return ESP_OK;
}