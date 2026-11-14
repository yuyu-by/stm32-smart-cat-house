/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "HX711.h"
#include "DHT11.h"
#include "Servo.h"
#include "TCRT5000.h"
#include "LightSensor.h"
#include "FillLight.h"
#include "Buzzer.h"
#include "ESP8266.h"
#include "oled.h"
#include <stdio.h> //sprintf,把整数转换成可以显示的字符串

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  FEEDER_STATE_IDLE = 0,
  FEEDER_STATE_DISPENSING,
  FEEDER_STATE_SETTLING,
  FEEDER_STATE_COOLDOWN
} Feeder_State;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HX711_READ_TIMEOUT_MS 1000U
#define HX711_AVERAGE_SAMPLES 10U
#define HX711_WARMUP_MS        5000U
#define HX711_CALIBRATION_MODE    0U
#define FEEDER_BOWL_TARE_G        250
#define FEEDER_LOW_WEIGHT_G       20
#define FEEDER_TARGET_WEIGHT_G    60
#define FEEDER_OPEN_TIME_MS       1000U
#define FEEDER_SETTLE_TIME_MS     2000U
#define FEEDER_COOLDOWN_TIME_MS   600000U
#define FEEDER_MAX_PORTIONS       5U
#define LIGHT_SENSOR_READ_INTERVAL_MS 500U
#define LIGHT_SENSOR_SAMPLES          16U
#define LIGHT_SENSOR_TIMEOUT_MS       10U
#define WIFI_AP_SSID                  "SmartCatHouse"
#define WIFI_AP_PASSWORD              "12345678"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile int32_t HX711_average = 0;
int32_t HX711_new_average = 0;
volatile int32_t HX711_previous_average = 0;
volatile int32_t HX711_difference = 0;
int32_t HX711_offset = 0;
volatile int32_t HX711_net = 0;
volatile int32_t HX711_total_weight_gram = 0;
volatile int32_t HX711_weight_gram = 0;
volatile HX711_Status HX711_status = HX711_STATUS_BAD_ARGUMENT;
volatile uint8_t OLED_ok = 0; // 定义oled状态
char OLED_text[24];
volatile uint32_t HX711_read_count = 0;
volatile uint8_t HX711_tared = 0;
uint32_t HX711_warmup_start = 0;
int32_t HX711_warmup_raw = 0;
uint8_t DHT11_temperature = 0;
uint8_t DHT11_humidity = 0;
volatile DHT11_Status DHT11_status = DHT11_STATUS_BAD_ARGUMENT;
uint32_t DHT11_last_read_tick = 0;
volatile Servo_Status Servo_status = SERVO_STATUS_HAL_ERROR;
volatile uint8_t Servo_angle = 0U;
volatile GPIO_PinState TCRT5000_raw = GPIO_PIN_SET;
volatile uint8_t TCRT5000_detected = 0U;
volatile Feeder_State Feeder_state = FEEDER_STATE_IDLE;
volatile uint8_t Feeder_portions = 0U;
uint8_t Feeder_has_fed = 0U;
uint32_t Feeder_state_tick = 0U;
uint32_t Feeder_last_finish_tick = 0U;
volatile LightSensor_Status LightSensor_status =
    LIGHT_SENSOR_STATUS_BAD_ARGUMENT;
volatile uint16_t LightSensor_raw = 0U;
uint16_t LightSensor_new_raw = 0U;
volatile uint8_t LightSensor_percent = 0U;
uint32_t LightSensor_last_read_tick = 0U;
volatile uint8_t FillLight_on = 0U;
volatile Buzzer_Status Buzzer_status = BUZZER_STATUS_HAL_ERROR;
volatile ESP8266_Status ESP8266_status = ESP8266_STATUS_TIMEOUT;
volatile ESP8266_Status ESP8266_wifi_status = ESP8266_STATUS_TIMEOUT;
volatile ESP8266_Status ESP8266_server_status = ESP8266_STATUS_TIMEOUT;
volatile ESP8266_Status ESP8266_http_status = ESP8266_STATUS_TIMEOUT;
volatile uint32_t ESP8266_http_request_count = 0U;
uint8_t ESP8266_connection_id = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_ok = OLED_Init();
  HX711_Init();
  DHT11_Init();
  Servo_status = Servo_Init();
  Servo_angle = Servo_GetAngle();
  TCRT5000_Init();
  LightSensor_status = LightSensor_Init();
  FillLight_Init();
  Buzzer_status = Buzzer_Init();
  if (Buzzer_status == BUZZER_STATUS_OK)
  {
    Buzzer_Beep(200U);
  }
  HAL_Delay(1500U);
  ESP8266_status = ESP8266_TestAT(1500U);
  if (ESP8266_status != ESP8266_STATUS_OK)
  {
    HAL_Delay(500U);
    ESP8266_status = ESP8266_TestAT(1500U);
  }
  if (ESP8266_status == ESP8266_STATUS_OK)
  {
    ESP8266_wifi_status = ESP8266_StartAccessPoint(
        WIFI_AP_SSID, WIFI_AP_PASSWORD, 5000U);
  }

  if (OLED_ok)
  {
    OLED_Clear();
    OLED_ShowString(0, 0, "ESP8266 AT TEST", 16);
    if (ESP8266_status != ESP8266_STATUS_OK)
    {
      snprintf(OLED_text, sizeof(OLED_text),
               "AT ERR:%u", (unsigned int)ESP8266_status);
      OLED_ShowString(0, 20, OLED_text, 16);
    }
    else if (ESP8266_wifi_status == ESP8266_STATUS_OK)
    {
      OLED_ShowString(0, 20, "AP:READY", 16);
    }
    else
    {
      snprintf(OLED_text, sizeof(OLED_text),
               "WIFI ERR:%u", (unsigned int)ESP8266_wifi_status);
      OLED_ShowString(0, 20, OLED_text, 16);
    }
    OLED_Refresh();
    HAL_Delay(1000U);
  }

  if (OLED_ok)
  {
    OLED_Clear();
    OLED_ShowString(0, 0, "HX711 HAL TEST", 16);
    OLED_ShowString(0, 20, "REMOVE BOWL", 16);
    OLED_ShowString(0, 40, "WARMING...", 16);
    OLED_Refresh();
  }

  /* Keep the HX711 converting before capturing the empty-scale zero point. */
  HX711_warmup_start = HAL_GetTick();
  while ((HAL_GetTick() - HX711_warmup_start) < HX711_WARMUP_MS)
  {
    HX711_status = HX711_ReadRaw(&HX711_warmup_raw,
                                 HX711_READ_TIMEOUT_MS);
  }

  if (OLED_ok)
  {
    OLED_Clear();
    OLED_ShowString(0, 0, "HX711 HAL TEST", 16);
    OLED_ShowString(0, 20, "REMOVE BOWL", 16);
    OLED_ShowString(0, 40, "TARING...", 16);
    OLED_Refresh();
  }

  HX711_status = HX711_Tare(&HX711_offset,
                            HX711_AVERAGE_SAMPLES,
                            HX711_READ_TIMEOUT_MS);
  if (HX711_status == HX711_STATUS_OK)
  {
    HX711_tared = 1U;
  }

  /* Start the web server only after the blocking HX711 warm-up is finished. */
  if (ESP8266_wifi_status == ESP8266_STATUS_OK)
  {
    ESP8266_server_status = ESP8266_StartTcpServer(80U, 3000U);
    if (ESP8266_server_status == ESP8266_STATUS_OK)
    {
      ESP8266_server_status = ESP8266_StartInterruptReceive();
    }
  }

  if (OLED_ok && (ESP8266_server_status == ESP8266_STATUS_OK))
  {
    OLED_Clear();
    OLED_ShowString(0, 20, "WEB:READY", 16);
    OLED_Refresh();
    HAL_Delay(500U);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if ((ESP8266_server_status == ESP8266_STATUS_OK) &&
        (ESP8266_PollHttpRequest(&ESP8266_connection_id) != 0U))
    {
      ESP8266_http_status = ESP8266_SendHttpResponse(
          ESP8266_connection_id,
          "<h1>Smart Cat House Online</h1>",
          3000U);
      ESP8266_http_request_count++;
    }

    TCRT5000_Update();
    TCRT5000_raw = TCRT5000_GetRawLevel();
    TCRT5000_detected = TCRT5000_IsDetected();

    if ((HAL_GetTick() - LightSensor_last_read_tick) >=
        LIGHT_SENSOR_READ_INTERVAL_MS)
    {
      LightSensor_last_read_tick = HAL_GetTick();
      LightSensor_status = LightSensor_ReadAverage(
          &LightSensor_new_raw,
          LIGHT_SENSOR_SAMPLES,
          LIGHT_SENSOR_TIMEOUT_MS);
      if (LightSensor_status == LIGHT_SENSOR_STATUS_OK)
      {
        LightSensor_raw = LightSensor_new_raw;
        LightSensor_percent = LightSensor_RawToPercent(LightSensor_raw);
        FillLight_AutoUpdate(LightSensor_percent);
        FillLight_on = FillLight_IsOn();
      }
    }

    if (HX711_tared != 0U)
    {
      HX711_status = HX711_ReadAverage(&HX711_new_average,
                                       HX711_AVERAGE_SAMPLES,
                                       HX711_READ_TIMEOUT_MS);
      if (HX711_status == HX711_STATUS_OK)
      {
        if (HX711_read_count == 0U)
        {
          HX711_previous_average = HX711_new_average;
        }

        HX711_average = HX711_new_average;
        HX711_difference = HX711_average - HX711_previous_average;
        HX711_previous_average = HX711_average;
        HX711_net = HX711_GetNetValue(HX711_average, HX711_offset);
        HX711_total_weight_gram = HX711_GetWeightGram(HX711_net);
        if (HX711_CALIBRATION_MODE != 0U)
        {
          HX711_weight_gram = HX711_total_weight_gram;
        }
        else
        {
          HX711_weight_gram = HX711_total_weight_gram - FEEDER_BOWL_TARE_G;
        }
        if (HX711_weight_gram < 0)
        {
          HX711_weight_gram = 0;
        }
        HX711_read_count++;
      }
    }

    /*
     * Feed only when a pet is present and the remaining food is low.
     * Each opening dispenses one small portion. The bowl is weighed again
     * after the food has settled, so unfinished food does not trigger a refill.
     */
    if ((HX711_CALIBRATION_MODE == 0U) &&
        (Servo_status == SERVO_STATUS_OK) &&
        (HX711_status == HX711_STATUS_OK) &&
        (HX711_tared != 0U))
    {
      if (Feeder_state == FEEDER_STATE_IDLE)
      {
        if ((TCRT5000_detected != 0U) &&
            (HX711_weight_gram < FEEDER_LOW_WEIGHT_G) &&
            ((Feeder_has_fed == 0U) ||
             ((HAL_GetTick() - Feeder_last_finish_tick) >=
              FEEDER_COOLDOWN_TIME_MS)))
        {
          Servo_status = Servo_OpenFeeder();
          Servo_angle = Servo_GetAngle();
          if (Servo_status == SERVO_STATUS_OK)
          {
            Feeder_portions = 1U;
            Feeder_state = FEEDER_STATE_DISPENSING;
            Feeder_state_tick = HAL_GetTick();
          }
        }
      }
      else if (Feeder_state == FEEDER_STATE_DISPENSING)
      {
        if ((HAL_GetTick() - Feeder_state_tick) >= FEEDER_OPEN_TIME_MS)
        {
          Servo_status = Servo_CloseFeeder();
          Servo_angle = Servo_GetAngle();
          if (Servo_status == SERVO_STATUS_OK)
          {
            Feeder_state = FEEDER_STATE_SETTLING;
            Feeder_state_tick = HAL_GetTick();
          }
        }
      }
      else if (Feeder_state == FEEDER_STATE_SETTLING)
      {
        if ((HAL_GetTick() - Feeder_state_tick) >= FEEDER_SETTLE_TIME_MS)
        {
          if ((TCRT5000_detected != 0U) &&
              (HX711_weight_gram < FEEDER_TARGET_WEIGHT_G) &&
              (Feeder_portions < FEEDER_MAX_PORTIONS))
          {
            Servo_status = Servo_OpenFeeder();
            Servo_angle = Servo_GetAngle();
            if (Servo_status == SERVO_STATUS_OK)
            {
              Feeder_portions++;
              Feeder_state = FEEDER_STATE_DISPENSING;
              Feeder_state_tick = HAL_GetTick();
            }
          }
          else
          {
            Feeder_state = FEEDER_STATE_COOLDOWN;
            Feeder_last_finish_tick = HAL_GetTick();
            Feeder_has_fed = 1U;
          }
        }
      }
      else if (Feeder_state == FEEDER_STATE_COOLDOWN)
      {
        if ((HAL_GetTick() - Feeder_last_finish_tick) >=
            FEEDER_COOLDOWN_TIME_MS)
        {
          Feeder_state = FEEDER_STATE_IDLE;
          Feeder_portions = 0U;
        }
      }
      else
      {
        Feeder_state = FEEDER_STATE_IDLE;
      }
    }

    if ((HAL_GetTick() - DHT11_last_read_tick) >= 2000U)
    {
      DHT11_last_read_tick = HAL_GetTick();
      DHT11_status = DHT11_Read(&DHT11_temperature, &DHT11_humidity);
    }

    if (OLED_ok)
    {
      OLED_Clear();

#if HX711_CALIBRATION_MODE
      if (HX711_status == HX711_STATUS_OK)
      {
        snprintf(OLED_text, sizeof(OLED_text),
                 "N:%ld", (long)HX711_net);
        OLED_ShowString(0, 0, OLED_text, 16);

        snprintf(OLED_text, sizeof(OLED_text),
                 "W:%ldg", (long)HX711_total_weight_gram);
        OLED_ShowString(0, 20, OLED_text, 16);

        snprintf(OLED_text, sizeof(OLED_text),
                 "D:%ld", (long)HX711_difference);
        OLED_ShowString(0, 40, OLED_text, 16);
      }
      else
      {
        OLED_ShowString(0, 20, "HX711 ERROR", 16);
      }
#else
      snprintf(OLED_text, sizeof(OLED_text),
               "PET:%s",
               (TCRT5000_detected != 0U) ? "YES" : "NO");
      OLED_ShowString(0, 0, OLED_text, 16);

      if (HX711_status == HX711_STATUS_OK)
      {
        snprintf(OLED_text, sizeof(OLED_text),
                 "FOOD:%ldg", (long)HX711_weight_gram);
      }
      else
      {
        snprintf(OLED_text, sizeof(OLED_text), "FOOD:ERR");
      }
      OLED_ShowString(0, 16, OLED_text, 16);

      if (DHT11_status == DHT11_STATUS_OK)
      {
        snprintf(OLED_text, sizeof(OLED_text),
                 "T:%uC H:%u%%",
                 (unsigned int)DHT11_temperature,
                 (unsigned int)DHT11_humidity);
      }
      else
      {
        snprintf(OLED_text, sizeof(OLED_text), "T:ERR H:ERR");
      }
      OLED_ShowString(0, 32, OLED_text, 16);

      if (LightSensor_status == LIGHT_SENSOR_STATUS_OK)
      {
        snprintf(OLED_text, sizeof(OLED_text),
                 "LIGHT:%u%%", (unsigned int)LightSensor_percent);
      }
      else
      {
        snprintf(OLED_text, sizeof(OLED_text), "LIGHT:ERR");
      }
      OLED_ShowString(0, 48, OLED_text, 16);
#endif

      OLED_Refresh();
    }

    HAL_Delay(100);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
}

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
