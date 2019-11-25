/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 * @defgroup nrf_adc_example main.c
 * @{
 * @ingroup nrf_adc_example
 * @brief ADC Example Application main file.
 *
 * This file contains the source code for a sample application using ADC.
 *
 * @image html example_board_setup_a.jpg "Use board setup A for this example."
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "boards.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"



#define SAMPLES_IN_BUFFER 3
volatile uint8_t state = 1;

static const nrf_drv_timer_t m_timer = NRF_DRV_TIMER_INSTANCE(1);
static nrf_saadc_value_t     m_buffer_pool[2][SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t     m_ppi_channel;
static uint32_t              m_adc_evt_counter;


void timer_handler(nrf_timer_event_t event_type, void * p_context)
{

}


void saadc_sampling_event_init(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    err_code = nrf_drv_timer_init(&m_timer, &timer_cfg, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event every 400ms */
    uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer, 50);
    nrf_drv_timer_extended_compare(&m_timer,
                                   NRF_TIMER_CC_CHANNEL0,
                                   ticks,
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   false);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer,
                                                                                NRF_TIMER_CC_CHANNEL0);
    uint32_t saadc_sample_task_addr   = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel,
                                          timer_compare_event_addr,
                                          saadc_sample_task_addr);
    APP_ERROR_CHECK(err_code);
}


void saadc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);

    APP_ERROR_CHECK(err_code);
}

#define MUX_1 NRF_GPIO_PIN_MAP(0, 18)
#define MUX_2 NRF_GPIO_PIN_MAP(0, 16)
#define MUX_3 NRF_GPIO_PIN_MAP(0, 15)

void clearMux(){
  nrf_gpio_pin_clear(MUX_1);
  nrf_gpio_pin_clear(MUX_2);
  nrf_gpio_pin_clear(MUX_3);
}

//0,1,2,3,4,5,6,7
void setMux(int index){
  clearMux();

//  SEGGER_RTT_printf(0,"setMux : %d\n",index);
  switch(index){
    case 0:
      //mux clear
      break;
    case 1:
      nrf_gpio_pin_set(MUX_1);
      break;
    case 2:
      nrf_gpio_pin_set(MUX_2);
      break;
    case 3:
      nrf_gpio_pin_set(MUX_3);
      break;
    case 4:
      nrf_gpio_pin_set(MUX_1);
      nrf_gpio_pin_set(MUX_2);
      break;
    case 5:
      nrf_gpio_pin_set(MUX_1);
      nrf_gpio_pin_set(MUX_3);
      break;
    case 6:
      nrf_gpio_pin_set(MUX_2);
      nrf_gpio_pin_set(MUX_3);
      break;
    case 7:
      nrf_gpio_pin_set(MUX_1);
      nrf_gpio_pin_set(MUX_2);
      nrf_gpio_pin_set(MUX_3);
      break;
  }

}
int min = 0;
int max = 0;
bool maxCom = 0;
int pause = 100;
void saadc_callback(nrf_drv_saadc_evt_t const *p_event) {
  if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {

    ret_code_t err_code;

    err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    //    SEGGER_RTT_printf(0,"ADC event number: %d\n", (int)m_adc_evt_counter);

    setMux(m_adc_evt_counter % 8);

    if (max < pause) {
      lt_ble_data_max[m_adc_evt_counter % 8 * 2] = p_event->data.done.p_buffer[0];
      lt_ble_data_max[m_adc_evt_counter % 8 * 2 + 1] = p_event->data.done.p_buffer[2];
      lt_ble_data_max[16] = p_event->data.done.p_buffer[1];
      max++;
    } else if (min < pause&&maxCom) {
      lt_ble_data_min[m_adc_evt_counter % 8 * 2] = p_event->data.done.p_buffer[0];
      lt_ble_data_min[m_adc_evt_counter % 8 * 2 + 1] = p_event->data.done.p_buffer[2];
      lt_ble_data_min[16] = p_event->data.done.p_buffer[1];
      min++;
    } else {
      lt_ble_data[m_adc_evt_counter % 8 * 2] = p_event->data.done.p_buffer[0];
      lt_ble_data[m_adc_evt_counter % 8 * 2 + 1] = p_event->data.done.p_buffer[2];
      lt_ble_data[16] = p_event->data.done.p_buffer[1];
    }
//    if(m_adc_evt_counter % 8 * 2==14)
//      SEGGER_RTT_printf(0, "===%d===\n", p_event->data.done.p_buffer[0] );
    /*
    SEGGER_RTT_printf(0, "===%d===\n", m_adc_evt_counter % 8);
    SEGGER_RTT_printf(0, "AIN2(a) = %d mV\n", p_event->data.done.p_buffer[0] * 3600 / 1024);
    SEGGER_RTT_printf(0, "AIN1(b) = %d mV\n", p_event->data.done.p_buffer[1] * 3600 / 1024);
    SEGGER_RTT_printf(0, "AIN3(c) = %d mV\n", p_event->data.done.p_buffer[2] * 3600 / 1024);
    */
    m_adc_evt_counter++;
  }
}

void saadc_init(void)
{
  ret_code_t err_code;
  nrf_saadc_channel_config_t channel_config0 =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);
  nrf_saadc_channel_config_t channel_config1 =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1);
  nrf_saadc_channel_config_t channel_config2 =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN3);

  err_code = nrf_drv_saadc_init(NULL, saadc_callback);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_saadc_channel_init(0, &channel_config0);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_saadc_channel_init(1, &channel_config1);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_saadc_channel_init(2, &channel_config2);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAMPLES_IN_BUFFER);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAMPLES_IN_BUFFER);
  APP_ERROR_CHECK(err_code);

}


/**
 * @brief Function for main application entry.
 */
 #define SEN NRF_GPIO_PIN_MAP(0, 14)
 #define RGB_R NRF_GPIO_PIN_MAP(0, 6)
#define RGB_G NRF_GPIO_PIN_MAP(0, 7)
#define RGB_B NRF_GPIO_PIN_MAP(0, 8)
int adcinit(void)
{
    uint32_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
      nrf_gpio_cfg_output(MUX_1);
  nrf_gpio_cfg_output(MUX_2);
  nrf_gpio_cfg_output(MUX_3);
  nrf_gpio_cfg_output(SEN);
nrf_gpio_pin_set(SEN);
    ret_code_t ret_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(ret_code);

    saadc_init();
    saadc_sampling_event_init();
    saadc_sampling_event_enable();
    SEGGER_RTT_printf(0,"SAADC HAL simple example started.");

  init_setting();

}
void init_setting() {

  SEGGER_RTT_printf(0, "TEST\n");
  while (max < pause) {
    //    SEGGER_RTT_printf(0,"WHILE %d\n",lt_ble_data_max[16]);
    if (nrf_gpio_pin_out_read(RGB_G))
      nrf_gpio_pin_clear(RGB_G);
    else
      nrf_gpio_pin_set(RGB_G);
    nrf_delay_ms(100);
  }
  SEGGER_RTT_printf(0, "LOAD %d\n", lt_ble_data_max[16]);
  nrf_gpio_pin_clear(RGB_G);
  nrf_delay_ms(5000);
  nrf_gpio_pin_set(RGB_G);
  SEGGER_RTT_printf(0, "LOADEND\n");
  maxCom=1;
  while (min < pause) {
    if (nrf_gpio_pin_out_read(RGB_G))
      nrf_gpio_pin_clear(RGB_G);
    else
      nrf_gpio_pin_set(RGB_G);
    nrf_delay_ms(100);
  }
  nrf_gpio_pin_set(RGB_G);
  SEGGER_RTT_printf(0, "MAX END %d %d\n", lt_ble_data_min[16], lt_ble_data_max[16]);
  nrf_delay_ms(1000);
}


/** @} */
