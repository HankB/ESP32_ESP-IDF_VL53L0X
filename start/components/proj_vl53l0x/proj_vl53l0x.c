#include "proj_vl53l0x.h"
#include "proj_wifi.h"
#include "proj_mqtt.h"

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

#include "vl53l0x.h"

static const char *TAG = "VL53L0X_EXAMPLE";
static vl53l0x_handle_t sensor = NULL;

#define I2C_PORT_NUM I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_1
#define I2C_SCL_PIN GPIO_NUM_0

#define VL53L0X_XSHUT_PIN GPIO_NUM_2

#define VL53L0X_CALIBRATION_OFFSET_UM 15000
#define VL53L0X_CALIBRATION_XTALK_MCPS 0.0
#define VL53L0X_CALIBRATION_REF_SPAD                                           \
  &(vl53l0x_ref_spad_calibration_t) { .count = 3, .is_aperture = true }

static void print_data(const vl53l0x_data_t *data) {
  ESP_LOGI(TAG,
           "distance=%.2f in, valid=%s, status=%u (%s), signal=%.3f Mcps, "
           "ambient=%.3f Mcps",
           (float)(data->distance_mm) / 25.4, data->valid ? "true" : "false",
           (unsigned)data->range_status,
           vl53l0x_range_status_str(data->range_status),
           (double)data->signal_rate_mcps, (double)data->ambient_rate_mcps);
}

static void vl53l0x_hard_reset(void) {
  // On common VL53L0X breakout boards, XSHUT is usually pulled up to HIGH with
  // a 10 kΩ resistor. Drive LOW briefly to force a clean hardware reset before
  // I2C initialization.
  gpio_set_direction(VL53L0X_XSHUT_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(VL53L0X_XSHUT_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(2));
  gpio_set_level(VL53L0X_XSHUT_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(2));
}

/*
 *  task to periodically sample and report results from VL53L0X
 */
static void vl53l0x_task(void *arg) {
  static const size_t topic_len = 128;
  static const size_t payload_len = 128;

  char topic[topic_len];
  char payload[payload_len];

  snprintf(topic, topic_len, "HA/%s/roaming/distance", generate_hostname());

  
  vl53l0x_data_t data = {0};

  while (1) {
    // Single-shot manual polling: start one measurement, poll readiness over
    // I2C, then read and print.
    ESP_ERROR_CHECK(vl53l0x_start_measurement(sensor));

    bool ready = false;
    while (true) {
      ESP_ERROR_CHECK(vl53l0x_get_ready(sensor, &ready));
      if (ready) {
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_ERROR_CHECK(vl53l0x_get_data(sensor, &data));
    print_data(&data);
    snprintf(payload, sizeof(payload), "{\"t\":%lld, \"timestamp_ms\":%lu, \"distance\":%.2f, \"device\":\"VL53L0X\"}",
        time(0), data.timestamp_ms,  (float)(data.distance_mm) / 25.4);
    proj_mqtt_publish(topic, payload, 0, false);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void proj_init_vl53l0x(void) {
  ESP_LOGI(TAG, "proj_init_vl53l0x");

  i2c_master_bus_handle_t bus = NULL;
  const i2c_master_bus_config_t bus_cfg = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_PORT_NUM,
      .sda_io_num = 21, // I2C_SDA_PIN,
      .scl_io_num = 22, // I2C_SCL_PIN,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  // Create and configure the I2C master bus used by the sensor.
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
  ESP_LOGI(TAG, "i2c_new_master_bus()");

  ESP_ERROR_CHECK(vl53l0x_create(&sensor, bus));
  ESP_ERROR_CHECK(vl53l0x_init(sensor));
  // Apply calibration values tuned for this setup (replace with your own
  // measured values for best accuracy).
  ESP_ERROR_CHECK(
      vl53l0x_set_reference_spads(sensor, VL53L0X_CALIBRATION_REF_SPAD));
  ESP_ERROR_CHECK(
      vl53l0x_perform_ref_calibration(sensor, &(vl53l0x_ref_calibration_t){}));
  ESP_ERROR_CHECK(
      vl53l0x_set_offset_calibration(sensor, VL53L0X_CALIBRATION_OFFSET_UM));
  ESP_ERROR_CHECK(
      vl53l0x_set_xtalk_calibration(sensor, VL53L0X_CALIBRATION_XTALK_MCPS));
  ESP_ERROR_CHECK(vl53l0x_set_xtalk_compensation_enable(sensor, true));
  ESP_ERROR_CHECK(vl53l0x_set_profile(sensor, VL53L0X_PROFILE_DEFAULT));
  ESP_ERROR_CHECK(vl53l0x_set_mode(sensor, VL53L0X_MODE_SINGLE));

  xTaskCreate(vl53l0x_task, "read_vl53l0x", 2048, NULL, 5, NULL);
  ESP_LOGI(TAG, "proj_init_vl53l0x() done");
}
