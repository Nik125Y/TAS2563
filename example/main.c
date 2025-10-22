#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/spi_slave.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "hal/i2s_types.h"
#include "driver/i2s_tdm.h"
#include "driver/periph_ctrl.h"
#include "esp_intr_alloc.h"
#include "portmacro.h"
#include <string.h>
#include <math.h>

/* Pins */   
#define PIN_MISO  		37
#define PIN_MOSI  		38
#define PIN_SCLK		36
#define PIN_CS    		40

#define I2S_BCLK 12
#define I2S_LRCLK 13
#define I2S_DOUT 11

#define TAS2563_I2C_ADDR 0x4C

#define I2C_MASTER_SDA 17
#define I2C_MASTER_SCL 18
#define I2C_MASTER_FREQ_HZ 400000


static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t tas2563_handle = NULL;
static i2s_chan_handle_t tx_chan; 

static const char *TAG = "Master";

static esp_err_t tas2563_write_reg (uint8_t reg, uint8_t value)
{
    uint8_t data [2] = { reg, value };
    return i2c_master_transmit(tas2563_handle, data, sizeof(data), 100);
}


static esp_err_t tas2563_read_reg (uint8_t reg, uint8_t *value)
{
    esp_err_t ret = i2c_master_transmit(tas2563_handle, &reg, 1, 100);
    if (ret != ESP_OK) return ret;
    return i2c_master_receive(tas2563_handle, value, 1, 100);
}


static esp_err_t i2c_init (void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    i2c_device_config_t dev_cfg = {
        .device_address = TAS2563_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &tas2563_handle));

    ESP_LOGI(TAG, "I2C init done");
    return ESP_OK;
}


/* I2S */
static void i2s_init (void) 
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_tdm_config_t tdm = {
        .clk_cfg  = I2S_TDM_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = {
            .data_bit_width   = I2S_DATA_BIT_WIDTH_16BIT,   
            .slot_bit_width   = I2S_SLOT_BIT_WIDTH_16BIT,   
            .slot_mode        = I2S_SLOT_MODE_STEREO,
            .total_slot       = 1,                          
            .slot_mask        = I2S_TDM_SLOT0 | I2S_TDM_SLOT1,              
            .ws_width         = 16,                         
            .ws_pol           = false,                      
            .bit_shift        = false,                     
            .left_align       = false,             			
            .big_endian       = false,
            .bit_order_lsb    = false,
            .skip_mask        = false,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,                       
            .bclk = I2S_BCLK,                              
            .ws   = I2S_LRCLK,                             
            .dout = I2S_DOUT,                            
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { 0 },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(tx_chan, &tdm));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}



static void TAS2563_Init (void)
{
	ESP_LOGI(TAG, "Init start TAS2563");
	
	gpio_set_level(GPIO_NUM_21, 0);
    vTaskDelay(100);
    gpio_set_level(GPIO_NUM_21, 1);
    vTaskDelay(100);
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	
	ESP_ERROR_CHECK(tas2563_write_reg(0x01, 0x01)); // reset
	vTaskDelay(10);
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	ESP_ERROR_CHECK(tas2563_write_reg(0x02, 0x0E)); // Software shutdown
	vTaskDelay(10);
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	ESP_ERROR_CHECK(tas2563_write_reg(0x04, 0x08));  // IRQ internal pull-up
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	ESP_ERROR_CHECK(tas2563_write_reg(0x06, 0x09));  // 44.1/48 kHz, FSYNC High to Low
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	ESP_ERROR_CHECK(tas2563_write_reg(0x07, 0x00));
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	ESP_ERROR_CHECK(tas2563_write_reg(0x08, 0x30));  //  Stereo downmix (L+R)/2
//	ESP_ERROR_CHECK(tas2563_write_reg(0x08, 0x10));  //  Mono left channel
//	ESP_ERROR_CHECK(tas2563_write_reg(0x08, 0x20));  //  Mono right channel
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
 	ESP_ERROR_CHECK(tas2563_write_reg(0x02, 0x00)); // Turn ON
 	
 	ESP_ERROR_CHECK(tas2563_write_reg(0, 0));
	ESP_ERROR_CHECK(tas2563_write_reg(0x7F, 0x00));
	
	ESP_ERROR_CHECK(tas2563_write_reg(0, 2));
	ESP_ERROR_CHECK(tas2563_write_reg(0x0C, 0x16));  // MSB
	ESP_ERROR_CHECK(tas2563_write_reg(0x0D, 0x4E));
	ESP_ERROR_CHECK(tas2563_write_reg(0x0E, 0xFB));
	ESP_ERROR_CHECK(tas2563_write_reg(0x0F, 0xD6));  // LSB

    ESP_LOGI(TAG, "Init end TAS2563");
}


#define SAMPLE_RATE     48000
#define TONE_FREQ_HZ    1000
#define AMPLITUDE       30000
#define BUF_LEN         256

static void test (void *pvParameters)
{
    ESP_LOGI("TEST", "Generating sine wave");

    static int16_t i2s_buf[BUF_LEN];
    float phase = 0.0f;
    const float delta = 2.0f * M_PI * TONE_FREQ_HZ / SAMPLE_RATE;

    while (1)
    {
        for (int i = 0; i < BUF_LEN; i++)
        {
            i2s_buf[i] = (int16_t)(AMPLITUDE * sinf(phase));
            phase += delta;
            if (phase > 2.0f * M_PI)
                phase -= 2.0f * M_PI;
        }

        for (int i = 0; i < BUF_LEN; i++)
        {
            uint16_t s = (uint16_t)i2s_buf[i];
        }

        size_t written = 0;
        esp_err_t ret = i2s_channel_write(tx_chan, i2s_buf, sizeof(i2s_buf), &written, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGE("TEST_TONE", "I2S write error: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
 
	gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_21);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
	
    i2c_init();
    i2s_init();
    
    TAS2563_Init();
    
    test(NULL);
}
