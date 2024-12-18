#include <math.h>
#include <esp_err.h>
#include <esp_log.h>
#include <led_strip.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <display.h>
#include <board.h>
#include <esp_timer.h>

bool isNotReadyToPlay = true;

#pragma region Others
int sensorBase[64] = {};
#define LED_STRIP_GPIO 13

static const char *TAG = "SmartChess";
static const char *INIT_TAG = "SmartChess init";

SemaphoreHandle_t mutex;
led_strip_handle_t led_strip = NULL;

/* LED strip initialization with the GPIO and pixels number*/
led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO, // The GPIO that connected to the LED strip's data line
        .max_leds = 256, // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812, // LED strip model
        .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
};

led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false, // whether to enable the DMA feature
#endif
};

#pragma endregion
#pragma region Math
float getAvg(const int arr[], int n) {
    int sum = 0;
    // Find the sum of all elements
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    // Return the average
    return (float) sum / (float) n;
}
#pragma endregion
static volatile bool starting = true;

void startupLights() {
    uint16_t hue = 0;
    uint8_t sat = 255;

    while (starting) {
        hue = 0;
        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < 64; ++i) {
                ESP_ERROR_CHECK(led_strip_set_pixel_hsv(led_strip, i, hue++, 255, (sin((double) sat / 100 * 3.14)) * 255));
            }

            sat++;
            if (sat > 100) {
                sat = 0;
            }

            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            xSemaphoreGive(mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    led_strip_clear(led_strip);

    vTaskDelete(NULL);
}

void tristate(int *value, int base) {
    int low = base - 35;
    int high = base + 35;
    if (*value > low && *value < high) {
        *value = 0;
    } else if (*value <= low) {
        *value = -1;
    } else if (*value >= high) {
        *value = 1;
    }
}

const adc_channel_t channels[8] = {0, 2, 3, 5, 6, 7, 9, 8};
const gpio_num_t ports[8] = {19, 5, 33, 23, 32, 18, 17, 16};

int readSensor(adc_oneshot_unit_handle_t adcHandle, int index, int samples) {
    int values[samples] = {};
    for (int i = 0; i < samples; ++i) {
        adc_oneshot_read(adcHandle, channels[index % 8], &values[i]);
    }
    return roundf(getAvg(values, samples));
}

adc_oneshot_unit_handle_t adcHandle;

// Configure ADC (only run ONCE)
void setupAdc() {
    ESP_LOGI(INIT_TAG, "Calibrating ADC");
    adc_oneshot_unit_init_cfg_t adcInitConfig = {
            .unit_id = ADC_UNIT_2,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcInitConfig, &adcHandle));
    adc_oneshot_chan_cfg_t adcChannelConfig = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    for (int i = 0; i < sizeof(channels) / sizeof(channels[0]); ++i) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, channels[i], &adcChannelConfig));
    }
}

// Get the LED index for the sensor on the given position
int getSensorToLedPosition(int row, int col) {
    if (row % 2 != 0) {
        return row * 8 + col;
    } else {
        return row * 8 + (7 - col);
    }
}

// Render chess pattern
void renderBoard() {
    ESP_LOGI(INIT_TAG, "Rendering board");
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        esp_err_t clear_err = led_strip_clear(led_strip);
        if (clear_err != ESP_OK) {
            ESP_LOGE(TAG, "LED strip clear failed: %s", esp_err_to_name(clear_err));
        }

        esp_err_t refresh_err = led_strip_refresh(led_strip);
        if (refresh_err != ESP_OK) {
            ESP_LOGE(TAG, "LED strip refresh failed: %s", esp_err_to_name(refresh_err));
        }

        xSemaphoreGive(mutex);
    }
}

esp_timer_handle_t gameTimerHandle;
TaskHandle_t startupTask;

void gameLoop() {
    int values[8] = {};

    //INICIO LOOP JOGO
    for (int col = 0; col < 8; ++col) {

        for (int row = 0; row < 8; ++row) {
            int index = 8 * col + row;
            
            for (int i = 0; i <= 10; i++) {
                gpio_set_level(ports[col], 1);
                vTaskDelay(pdMS_TO_TICKS(5));
                values[row] = readSensor(adcHandle, index, 10); 
                gpio_set_level(ports[col], 0);
            }         
            // ESP_ERROR_CHECK(led_strip_clear(led_strip));
            // ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255));
            // ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            tristate(&values[row], sensorBase[index]);
        }

        handleData(col, values);

    }
}

void sensorLoop() {
    starting = false;

    renderBoard();
    ESP_LOGI(INIT_TAG, "INICIO");
    ESP_LOGI(INIT_TAG, "Starting sensor loop...");
    displayText("Sensor Loop");
    int values[8] = {};

    //APENAS INICIAR COM PECAS NO LUGAR
    /*while (isNotReadyToPlay) {

         int readyCells = 0; // Inicialmente, assume que nenhuma linha está preenchida completamente

         ESP_LOGI(INIT_TAG, "Waiting pieces");
         for (int col = 0; col < sizeof(ports) / sizeof(ports[0]); ++col) {
             gpio_set_level(ports[col], 1);             
             
            for (int row = 0; row < sizeof(channels) / sizeof(channels[0]); ++row) {
                int index = 8 * col + row;
                for (int i = 0; i <= 10; i++) {
                    gpio_set_level(ports[col], 1);
                    vTaskDelay(pdMS_TO_TICKS(5));
                    //if(readSensor2(adcHandle, index) != 0) {
                    values[row] = readSensor(adcHandle, index, 10); 
                    //}
                    gpio_set_level(ports[col], 0);
                }         
                tristate(&values[row], sensorBase[index]);

                 if (row == 0 || row == 1 || row == 6 || row == 7) {
                     if (values[row] != 0) {
                         readyCells++;
                     }
                 }
            }
            isReadyToPlay(col, values);
            gpio_set_level(ports[col], 0);
        }


         ESP_LOGI(TAG, "READY CELLS %d", readyCells);
         if (readyCells == 0) {
             displayText("0 pronto");
             vTaskDelay(pdMS_TO_TICKS(1000));
         }
         if (readyCells == 1) {
             displayText("1 pronto");
             vTaskDelay(pdMS_TO_TICKS(1000));
         }
         if (readyCells == 2) {
            displayText("2 pronto");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
         if (readyCells == 3) {
             displayText("3 pronto");
             vTaskDelay(pdMS_TO_TICKS(1000));
         }

         if (readyCells == 32) {
             ESP_LOGI(INIT_TAG, "Ready");
             displayText("Ready");
            vTaskDelay(pdMS_TO_TICKS(1000));
             isNotReadyToPlay = false; // Todas as 4 linhas estão preenchidas, pronto para jogar
         } else {
             ESP_LOGI(INIT_TAG, "Not Ready");
             displayText("Not Ready");
             vTaskDelay(pdMS_TO_TICKS(1000));
             isNotReadyToPlay = true; // Ainda não está pronto, há linhas vazias
         }

         for (int i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
             values[i] = 0;
         }
         vTaskDelay(pdMS_TO_TICKS(10));
    }*/
    

    const esp_timer_create_args_t gameTimer = {
            .callback = &gameLoop,
            .name = "Game loop",
    };

    ESP_ERROR_CHECK(esp_timer_create(&gameTimer, &gameTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(gameTimerHandle, 50000));

    vTaskDelete(startupTask);
}

void app_main(void) {
    mutex = xSemaphoreCreateMutex();

    xSemaphoreTake(mutex, portMAX_DELAY);
    esp_err_t led_init_err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (led_init_err != ESP_OK) {
        ESP_LOGE(INIT_TAG, "LED strip initialization failed: %s", esp_err_to_name(led_init_err));
        // Handle initialization failure (maybe retry or set a flag)
        return;
    }

    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);
    xSemaphoreGive(mutex);

    xTaskCreate(startupLights, "Init blink", 4096, NULL, 1, NULL);

    xTaskCreate((TaskFunction_t) initDisplay, "Display", 8192, NULL, 2, NULL);
    // Chama a função para mostrar o texto
    vTaskDelay(pdMS_TO_TICKS(4000));
    displayText("Calibrando Sensores, Remova as Pecas");

    for (int i = 0; i < sizeof(ports) / sizeof(ports[0]); ++i) {
        gpio_set_direction(ports[i], GPIO_MODE_OUTPUT);
    }

    setupAdc();

    ESP_LOGI(INIT_TAG, "Calibrating for empty board...");
    for (int col = 0; col < sizeof(ports) / sizeof(ports[0]); ++col) {
        gpio_set_level(ports[col], 1);
        vTaskDelay(pdMS_TO_TICKS(50));

        for (int row = 0; row < sizeof(channels) / sizeof(channels[0]); ++row) {
            int index = 8 * col + row;
            sensorBase[index] = readSensor(adcHandle, index, 100);
        }

        gpio_set_level(ports[col], 0);
    }
    displayText("Calibrado com Sucesso!");

    xTaskCreate(sensorLoop, "Sensor loop", 8192, NULL, 1, &startupTask);
}
