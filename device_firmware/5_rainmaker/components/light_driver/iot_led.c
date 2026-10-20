// Copyright 2017 Espressif Systems
// Licensed under the Apache License, Version 2.0 (the "License");
// http://www.apache.org/licenses/LICENSE-2.0
//
// Updated for ESP-IDF v5.5+ and ESP32-C3
// Timer functions migrated from legacy hw_timer to esp_timer

#include <stdio.h>
#include <string.h>
#include <math.h>  // ĐÃ THÊM
#include "iot_led.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_timer.h"

static const char *TAG = "iot_led";

#ifndef LIGHT_PARAM_CHECK
#define LIGHT_PARAM_CHECK(condition) do { \
    if (!(condition)) { \
        ESP_LOGE(TAG, "Parameter check failed: %s", #condition); \
        return ESP_ERR_INVALID_ARG; \
    } \
} while(0)
#endif

typedef struct {
    ledc_channel_t channel;
    uint8_t value;
    uint8_t target;
    uint32_t fade_time;
    bool fade_active;
    esp_timer_handle_t fade_timer;
} led_channel_t;

typedef struct {
    led_channel_t channels[LEDC_CHANNEL_MAX];
    uint16_t gamma_table[GAMMA_TABLE_SIZE];
    bool initialized;
} light_config_t;

static light_config_t *g_light_config = NULL;

/* ---------------------- GAMMA TABLE ---------------------- */
static void generate_gamma_table(uint16_t *table)
{
    for (int i = 0; i < GAMMA_TABLE_SIZE; i++) {
        float x = (float)i / (GAMMA_TABLE_SIZE - 1);
        table[i] = (uint16_t)(powf(x, GAMMA_CORRECTION) * ((1 << 13) - 1)); // 13-bit duty
    }
}

/* ---------------------- TIMER CALLBACK ---------------------- */
static void IRAM_ATTR fade_timercb(void *arg)
{
    led_channel_t *ch = (led_channel_t *)arg;

    if (!ch->fade_active) return;

    int step = (ch->target > ch->value) ? 1 : -1;
    if (ch->value != ch->target) {
        ch->value += step;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch->channel, g_light_config->gamma_table[ch->value]);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, ch->channel);
    } else {
        ch->fade_active = false;
        esp_timer_stop(ch->fade_timer);
    }
}

/* ---------------------- INIT / DEINIT ---------------------- */
esp_err_t iot_led_init(ledc_timer_t timer_num, ledc_mode_t speed_mode, uint32_t freq_hz,
                       ledc_clk_cfg_t clk_cfg, ledc_timer_bit_t duty_resolution)
{
    if (g_light_config) {
        ESP_LOGW(TAG, "LED driver already initialized");
        return ESP_OK;
    }

    g_light_config = calloc(1, sizeof(light_config_t));
    LIGHT_PARAM_CHECK(g_light_config != NULL);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = speed_mode,
        .timer_num = timer_num,
        .duty_resolution = duty_resolution,
        .freq_hz = freq_hz,
        .clk_cfg = clk_cfg
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    generate_gamma_table(g_light_config->gamma_table);
    g_light_config->initialized = true;

    ESP_LOGI(TAG, "LED driver initialized successfully");
    return ESP_OK;
}

esp_err_t iot_led_deinit(void)
{
    if (!g_light_config) return ESP_ERR_INVALID_STATE;

    for (int i = 0; i < LEDC_CHANNEL_MAX; i++) {
        if (g_light_config->channels[i].fade_timer) {
            esp_timer_stop(g_light_config->channels[i].fade_timer);
            esp_timer_delete(g_light_config->channels[i].fade_timer);
        }
    }

    free(g_light_config);
    g_light_config = NULL;
    ESP_LOGI(TAG, "LED driver deinitialized");
    return ESP_OK;
}

/* ---------------------- CHANNEL CONFIG ---------------------- */
esp_err_t iot_led_regist_channel(ledc_channel_t channel, gpio_num_t gpio_num)
{
    LIGHT_PARAM_CHECK(g_light_config != NULL);

    ledc_channel_config_t ledc_channel = {
        .channel = channel,
        .duty = 0,
        .gpio_num = gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Create fade timer
    const esp_timer_create_args_t t_conf = {
        .callback = fade_timercb,
        .arg = &g_light_config->channels[channel],
        .dispatch_method = ESP_TIMER_TASK,
        .name = "fade_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&t_conf, &g_light_config->channels[channel].fade_timer));

    g_light_config->channels[channel].channel = channel;
    g_light_config->channels[channel].fade_active = false;
    g_light_config->channels[channel].value = 0;

    return ESP_OK;
}

/* ---------------------- CHANNEL CONTROL ---------------------- */
esp_err_t iot_led_get_channel(ledc_channel_t channel, uint8_t *dst)
{
    LIGHT_PARAM_CHECK(g_light_config != NULL);
    LIGHT_PARAM_CHECK(dst != NULL);
    *dst = g_light_config->channels[channel].value;
    return ESP_OK;
}

esp_err_t iot_led_set_channel(ledc_channel_t channel, uint8_t value, uint32_t fade_ms)
{
    LIGHT_PARAM_CHECK(g_light_config != NULL);

    led_channel_t *ch = &g_light_config->channels[channel];
    ch->target = value;
    ch->fade_time = fade_ms;
    ch->fade_active = true;

    uint64_t interval = fade_ms * 1000 / abs((int)value - (int)ch->value + 1);
    esp_timer_stop(ch->fade_timer);
    esp_timer_start_periodic(ch->fade_timer, interval);

    return ESP_OK;
}

esp_err_t iot_led_start_blink(ledc_channel_t channel, uint8_t value, uint32_t period_ms, bool fade_flag)
{
    LIGHT_PARAM_CHECK(g_light_config != NULL);
    
    if (fade_flag) {
        iot_led_set_channel(channel, value, period_ms / 2);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, g_light_config->gamma_table[value]);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    }
    return ESP_OK;
}

esp_err_t iot_led_stop_blink(ledc_channel_t channel)
{
    LIGHT_PARAM_CHECK(g_light_config != NULL);
    led_channel_t *ch = &g_light_config->channels[channel];
    esp_timer_stop(ch->fade_timer);
    ch->fade_active = false;
    return ESP_OK;
}

/* ---------------------- GAMMA TABLE ---------------------- */
esp_err_t iot_led_set_gamma_table(const uint16_t gamma_table[GAMMA_TABLE_SIZE])
{
    LIGHT_PARAM_CHECK(g_light_config != NULL);
    memcpy(g_light_config->gamma_table, gamma_table, sizeof(uint16_t) * GAMMA_TABLE_SIZE);
    return ESP_OK;
}