#ifndef _APP_PRIV_H_
#define _APP_PRIV_H_

#include "esp_err.h"
#include "esp_rmaker_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== HÀM KHỞI TẠO ========== */
void app_driver_init(void);
esp_err_t app_pm_init(void); // <--- ĐÂY LÀ DÒNG ĐÚNG

/* ========== HÀM ĐIỀU KHIỂN ĐÈN ========== */
esp_err_t app_light_set_power(bool power);
esp_err_t app_light_set_brightness(int brightness);
esp_err_t app_light_set_hue(int hue);
esp_err_t app_light_set_saturation(int saturation);

/* ========== HÀM LẤY TRẠNG THÁI ĐÈN ========== */
bool app_light_get_power(void);
int app_light_get_brightness(void);
int app_light_get_hue(void);
int app_light_get_saturation(void);

/* ========== HÀM TIỆN ÍCH & TEST ========== */
esp_err_t app_driver_set_state(bool state);
bool app_driver_get_state(void);
void app_light_test_colors(void);
void app_light_test_on_startup(void);
void app_test_led_simple(void);

/* ========== HÀM POWER MANAGEMENT ========== */
esp_err_t app_pm_lock_acquire();
esp_err_t app_pm_lock_release();

// --- BẠN CHỈ CẦN THÊM 2 KHAI BÁO NÀY VÀO ---

/**
 * @brief Treo khóa APB lock cho đèn LED (khi BẬT đèn)
 */
void app_pm_lock_led(void);

/**
 * @brief Thả khóa APB lock cho đèn LED (khi TẮT đèn)
 */
void app_pm_release_led(void);

// --- KẾT THÚC PHẦN THÊM VÀO ---
#ifdef __cplusplus
}
#endif

#endif /* _APP_PRIV_H_ */