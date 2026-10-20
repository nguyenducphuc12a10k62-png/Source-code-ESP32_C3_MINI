/* RainMaker Light + ESP Insights custom metrics (compatible with esp_diagnostics add_* API) */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

/* RainMaker */
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_standard_devices.h"
#include "esp_rmaker_ota.h"
#include "esp_rmaker_schedule.h"

/* App components from your project */
#include "app_wifi.h"
#include "app_storage.h"
#include "app_priv.h"
#include "app_insights.h"

/* ESP Insights Diagnostics */
#include "esp_diagnostics.h"
#include "esp_diagnostics_metrics.h"
#include "esp_diagnostics_variables.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *light_device;
extern const char ota_server_cert[] asm("_binary_server_crt_start");

/* ---- Custom keys (<= 15 chars recommended) ---- */
#define DIAG_TAG_LIGHT   "light"
#define K_PWR            "pwr"
#define K_BRI            "bri"
#define K_HUE            "hue"
#define K_SAT            "sat"

static void insights_light_diag_init(void)
{
    /* Register metrics */
    esp_diag_metrics_register(DIAG_TAG_LIGHT, K_PWR, "Light Power", "light", ESP_DIAG_DATA_TYPE_BOOL);
    esp_diag_metrics_register(DIAG_TAG_LIGHT, K_BRI, "Brightness",  "light", ESP_DIAG_DATA_TYPE_INT);
    esp_diag_metrics_register(DIAG_TAG_LIGHT, K_HUE, "Hue",         "light", ESP_DIAG_DATA_TYPE_INT);
    esp_diag_metrics_register(DIAG_TAG_LIGHT, K_SAT, "Saturation",  "light", ESP_DIAG_DATA_TYPE_INT);

    /* Your esp_diagnostics version: add_unit(key, unit) (NO tag param) */
    esp_diag_metrics_add_unit(K_BRI, "%");
    esp_diag_metrics_add_unit(K_HUE, "deg");
    esp_diag_metrics_add_unit(K_SAT, "%");

    /* Register variables (latest value view) */
    esp_diag_variable_register(DIAG_TAG_LIGHT, K_PWR, "Light Power", "light", ESP_DIAG_DATA_TYPE_BOOL);
    esp_diag_variable_register(DIAG_TAG_LIGHT, K_BRI, "Brightness",  "light", ESP_DIAG_DATA_TYPE_INT);
    esp_diag_variable_register(DIAG_TAG_LIGHT, K_HUE, "Hue",         "light", ESP_DIAG_DATA_TYPE_INT);
    esp_diag_variable_register(DIAG_TAG_LIGHT, K_SAT, "Saturation",  "light", ESP_DIAG_DATA_TYPE_INT);

    /* Push initial defaults so portal shows values even before first write */
    esp_diag_metrics_add_bool(K_PWR, DEFAULT_POWER);
    esp_diag_metrics_add_int(K_BRI, (int32_t)DEFAULT_BRIGHTNESS);
    esp_diag_metrics_add_int(K_HUE, (int32_t)DEFAULT_HUE);
    esp_diag_metrics_add_int(K_SAT, (int32_t)DEFAULT_SATURATION);

    esp_diag_variable_add_bool(K_PWR, DEFAULT_POWER);
    esp_diag_variable_add_int(K_BRI, (int32_t)DEFAULT_BRIGHTNESS);
    esp_diag_variable_add_int(K_HUE, (int32_t)DEFAULT_HUE);
    esp_diag_variable_add_int(K_SAT, (int32_t)DEFAULT_SATURATION);
}

/* Optional event helper: prefer macro if available, else fallback to esp_diag_log_event */
static void light_diag_event_power(bool on)
{
#ifdef ESP_DIAG_EVENT
    ESP_DIAG_EVENT(DIAG_TAG_LIGHT, "Power=%d", on ? 1 : 0);
#else
    esp_diag_log_event(DIAG_TAG_LIGHT, "Power=%d", on ? 1 : 0);
#endif
}

static void light_diag_event_int(const char *name, int v)
{
#ifdef ESP_DIAG_EVENT
    ESP_DIAG_EVENT(DIAG_TAG_LIGHT, "%s=%d", name, v);
#else
    esp_diag_log_event(DIAG_TAG_LIGHT, "%s=%d", name, v);
#endif
}

/* ---- RainMaker write callback ---- */
static esp_err_t write_cb(const esp_rmaker_device_t *device,
                          const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val,
                          void *priv_data,
                          esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Write via: %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name  = esp_rmaker_param_get_name(param);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "%s - %s = %s", device_name, param_name, val.val.b ? "true" : "false");
        app_light_set_power(val.val.b);

        /* Your API: add_bool(key, val) */
        esp_diag_metrics_add_bool(K_PWR, val.val.b);
        esp_diag_variable_add_bool(K_PWR, val.val.b);
        light_diag_event_power(val.val.b);

    } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
        ESP_LOGI(TAG, "%s - %s = %d", device_name, param_name, val.val.i);
        app_light_set_brightness(val.val.i);

        esp_diag_metrics_add_int(K_BRI, (int32_t)val.val.i);
        esp_diag_variable_add_int(K_BRI, (int32_t)val.val.i);
        light_diag_event_int("Bri", val.val.i);

    } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
        ESP_LOGI(TAG, "%s - %s = %d", device_name, param_name, val.val.i);
        app_light_set_hue(val.val.i);

        esp_diag_metrics_add_int(K_HUE, (int32_t)val.val.i);
        esp_diag_variable_add_int(K_HUE, (int32_t)val.val.i);
        light_diag_event_int("Hue", val.val.i);

    } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
        ESP_LOGI(TAG, "%s - %s = %d", device_name, param_name, val.val.i);
        app_light_set_saturation(val.val.i);

        esp_diag_metrics_add_int(K_SAT, (int32_t)val.val.i);
        esp_diag_variable_add_int(K_SAT, (int32_t)val.val.i);
        light_diag_event_int("Sat", val.val.i);

    } else {
        return ESP_OK; /* ignore unknown */
    }

    /* Report back to RainMaker cloud */
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting...");

    /* NVS */
    app_storage_init();

    /* Wi-Fi init */
    app_wifi_init();

    app_driver_init();
    ESP_LOGI(TAG, "App driver initialized");

    /* RainMaker node */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true, /* important for correct timestamps */
    };

    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg,
                                                   "ESP RainMaker Device",
                                                   "Lightbulb");
    if (!node) {
        ESP_LOGE(TAG, "Node init failed");
        abort();
    }

    /* Device + params */
    light_device = esp_rmaker_lightbulb_device_create("Light", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_cb(light_device, write_cb, NULL);

    esp_rmaker_device_add_param(light_device,
        esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, DEFAULT_BRIGHTNESS));
    esp_rmaker_device_add_param(light_device,
        esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, DEFAULT_HUE));
    esp_rmaker_device_add_param(light_device,
        esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, DEFAULT_SATURATION));

    esp_rmaker_node_add_device(node, light_device);

    /* OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ota_server_cert,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Schedule + TZ */
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();

    /* Enable Insights */
    ESP_LOGI(TAG, "Enabling ESP Insights...");
    app_insights_enable();
    ESP_LOGI(TAG, "ESP Insights enabled.");

    /* Register + push initial custom metrics/variables */
    insights_light_diag_init();
    ESP_LOGI(TAG, "Custom light metrics/variables registered.");

    /* Start RainMaker */
    esp_rmaker_start();

    /* Start Wi-Fi (connect / provision) */
    esp_err_t err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi start failed");
        abort();
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
