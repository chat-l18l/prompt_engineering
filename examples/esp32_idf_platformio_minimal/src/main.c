#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *g_log_tag = "app_main";

static const gpio_num_t g_led_gpio = GPIO_NUM_2;
static const int64_t g_blink_period_us = 500000;

static bool g_led_state = false;
static esp_timer_handle_t g_blink_timer = NULL;

typedef enum app_status_tag_e {
    APP_STATUS_OK = 0,
    APP_STATUS_GPIO_CONFIG_ERR,
    APP_STATUS_TIMER_CREATE_ERR,
    APP_STATUS_TIMER_START_ERR,
} app_status_tag_t;

typedef struct app_status_s {
    app_status_tag_t tag;
    union {
        esp_err_t esp_code;
        uint32_t reserved;
    } value;
} app_status_t;

static void app_blink_timer_tick(void *arg)
{
    (void)arg;
    g_led_state = !g_led_state;
    gpio_set_level(g_led_gpio, (uint32_t)g_led_state);
}

static app_status_t app_led_init(void)
{
    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << g_led_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    const esp_err_t rc = gpio_config(&cfg);
    if (rc != ESP_OK) {
        return (app_status_t){
            .tag = APP_STATUS_GPIO_CONFIG_ERR,
            .value = {.esp_code = rc},
        };
    }

    gpio_set_level(g_led_gpio, 0);
    return (app_status_t){.tag = APP_STATUS_OK, .value = {.reserved = 0}};
}

static app_status_t app_timer_init(void)
{
    const esp_timer_create_args_t timer_cfg = {
        .callback = &app_blink_timer_tick,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "blink",
        .skip_unhandled_events = true,
    };

    esp_err_t rc = esp_timer_create(&timer_cfg, &g_blink_timer);
    if (rc != ESP_OK) {
        return (app_status_t){
            .tag = APP_STATUS_TIMER_CREATE_ERR,
            .value = {.esp_code = rc},
        };
    }

    rc = esp_timer_start_periodic(g_blink_timer, g_blink_period_us);
    if (rc != ESP_OK) {
        return (app_status_t){
            .tag = APP_STATUS_TIMER_START_ERR,
            .value = {.esp_code = rc},
        };
    }

    return (app_status_t){.tag = APP_STATUS_OK, .value = {.reserved = 0}};
}

void app_main(void)
{
    const app_status_t led_status = app_led_init();
    if (led_status.tag != APP_STATUS_OK) {
        ESP_LOGE(g_log_tag, "gpio init failed: %s", esp_err_to_name(led_status.value.esp_code));
        return;
    }

    const app_status_t timer_status = app_timer_init();
    if (timer_status.tag != APP_STATUS_OK) {
        ESP_LOGE(g_log_tag, "timer init failed: %s", esp_err_to_name(timer_status.value.esp_code));
        return;
    }

    ESP_LOGI(g_log_tag, "blink timer started (period=%lld us)", g_blink_period_us);
}
