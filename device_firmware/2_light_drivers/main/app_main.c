#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "light_driver.h"

#define BUTTON_PIN 0

static const char *TAG = "APP";

static bool led_on = false;  // Trạng thái LED

void button_task(void *pvParameter)
{
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    int last_state = gpio_get_level(BUTTON_PIN);

    while (1)
    {
        int state = gpio_get_level(BUTTON_PIN);

        if (last_state == 1 && state == 0) {
            led_on = !led_on;
            ESP_LOGI(TAG, "Button pressed -> led_on = %s", led_on ? "ON" : "OFF");
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        last_state = state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void led_task(void *pvParameter)
{
    while (1)
    {
        if (led_on)
        {
    light_driver_fade_rgb_interruptible(255, 0, 100, 5000, &led_on);


        }
        else
        {
            // Tắt LED ngay lập tức
            light_driver_set_rgb_pwm(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}


void app_main(void)
{
    // Init PWM (GPIO 2, 1, 5 chỉ là ví dụ)
    light_driver_init_pwm(2,5,7);

    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
}
