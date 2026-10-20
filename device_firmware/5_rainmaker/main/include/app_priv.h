#ifndef _APP_PRIV_H_
#define _APP_PRIV_H_

#include "esp_err.h"
#include "esp_rmaker_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== HÀM KHỞI TẠO ========== */
void app_driver_init(void);

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

#ifdef __cplusplus
}
#endif

#endif /* _APP_PRIV_H_ */