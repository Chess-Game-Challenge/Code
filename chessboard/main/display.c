#include <sys/cdefs.h>
/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "esp_lcd_panel_vendor.h"
#include "display.h"

// Definição da variável global
lv_disp_t *global_disp = NULL;

static const char *TAG = "example";

#define I2C_BUS_PORT  0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA           21
#define EXAMPLE_PIN_NUM_SCL           22
#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_I2C_HW_ADDR           0x3C

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

// extern void example_lvgl_demo_ui(lv_disp_t *disp);
// Alteração na assinatura para aceitar texto como parâmetro
extern void example_lvgl_demo_ui(lv_disp_t *disp, const char *text);

lv_obj_t *label;
QueueHandle_t queue;
esp_timer_handle_t displayTimerHandle;

void displayText(const char *text) {
    if (lvgl_port_lock(0)) {
        example_lvgl_demo_ui(global_disp, text);
        lvgl_port_unlock(); // Immediately release lock
    }
    //xQueueSend(queue, text, 0);
}

void updateDisplay() {
    char rxBuffer[150];
    while (1) {
        if (xQueueReceive(queue, &rxBuffer, portMAX_DELAY)) {
            if (lvgl_port_lock(0)) {
                example_lvgl_demo_ui(global_disp, rxBuffer);
                ESP_LOGI("AAAAAA", "%s", rxBuffer);
                lvgl_port_unlock(); // Immediately release lock
            }
        }
    }
}

void initDisplay(void) {
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .i2c_port = I2C_BUS_PORT,
            .sda_io_num = EXAMPLE_PIN_NUM_SDA,
            .scl_io_num = EXAMPLE_PIN_NUM_SCL,
            .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = EXAMPLE_I2C_HW_ADDR,
            .scl_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .control_phase_bytes = 1,               // According to SSD1306 datasheet
            .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
            .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
            .dc_bit_offset = 6,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
            .bits_per_pixel = 1,
            .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
            .io_handle = io_handle,
            .panel_handle = panel_handle,
            .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
            .double_buffer = true,
            .hres = EXAMPLE_LCD_H_RES,
            .vres = EXAMPLE_LCD_V_RES,
            .monochrome = true,
            .rotation = {
                    .swap_xy = false,
                    .mirror_x = false,
                    .mirror_y = false,
            }
    };
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    char txBuffer[150];
    queue = xQueueCreate(5, sizeof(txBuffer));
    if (queue == 0) {
        ESP_LOGE("SmartChess Display", "Failed to create queue= %p\n", queue);
    }

    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        example_lvgl_demo_ui(disp, "Bem-vindo ao ESP32!");
        /* Rotation of the screen */
        lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_180);
        lvgl_port_unlock(); // Immediately release lock
    }

    /*const esp_timer_create_args_t displayTimer = {
            .callback = &updateDisplay,
            .name = "Display loop",
    };

    ESP_ERROR_CHECK(esp_timer_create(&displayTimer, &displayTimerHandle));

    ESP_ERROR_CHECK(esp_timer_start_periodic(displayTimerHandle, 50000));*/

    vTaskDelete(NULL);
}

//TODO
void example_lvgl_demo_ui(lv_disp_t *disp, const char *text) {
    lv_obj_t *scr = lv_disp_get_scr_act(disp); // Obtém a tela ativa
    lv_obj_clean(scr);//Limpa a tela
    label = lv_label_create(scr);  // Cria um rótulo na tela
    //lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); // Ativa o modo de rolagem circular
    lv_label_set_text(label, text);  // Define o texto recebido como parâmetro
    lv_obj_set_width(label, 128);  // Define a largura do rótulo
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0); // Alinha o rótulo ao centro no topo
}