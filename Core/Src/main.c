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
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "HX711.h"
#include "DHT11.h"
#include "Servo.h"
#include "oled.h"
#include <stdio.h> //sprintf,把整数转换成可以显示的字符串

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HX711_READ_TIMEOUT_MS 1000U
#define HX711_AVERAGE_SAMPLES 10U
#define HX711_WARMUP_MS        5000U
#define SERVO_SELF_TEST_DELAY_MS 1500U
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
volatile uint8_t Servo_self_test_state = 0U;
uint32_t Servo_self_test_tick = 0U;

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
  /* USER CODE BEGIN 2 */
  OLED_ok = OLED_Init();
  HX711_Init();
  DHT11_Init();
  Servo_status = Servo_Init();
  Servo_angle = Servo_GetAngle();

  if (OLED_ok)
  {
    OLED_Clear();
    OLED_ShowString(0, 0, "HX711 HAL TEST", 16);
    OLED_ShowString(0, 20, "KEEP EMPTY", 16);
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
    OLED_ShowString(0, 20, "KEEP EMPTY", 16);
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

  /* Start the one-time servo check after HX711 warm-up and tare finish. */
  Servo_self_test_tick = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* One-time non-blocking check: closed -> open -> closed. */
    if ((Servo_status == SERVO_STATUS_OK) &&
        ((HAL_GetTick() - Servo_self_test_tick) >= SERVO_SELF_TEST_DELAY_MS))
    {
      if (Servo_self_test_state == 0U)
      {
        Servo_status = Servo_OpenFeeder();
        Servo_angle = Servo_GetAngle();
        Servo_self_test_state = 1U;
        Servo_self_test_tick = HAL_GetTick();
      }
      else if (Servo_self_test_state == 1U)
      {
        Servo_status = Servo_CloseFeeder();
        Servo_angle = Servo_GetAngle();
        Servo_self_test_state = 2U;
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
        HX711_weight_gram = HX711_GetWeightGram(HX711_net);
        HX711_read_count++;
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

      if (HX711_status == HX711_STATUS_OK)
      {
        snprintf(OLED_text, sizeof(OLED_text),
                 "WT:%ldg", (long)HX711_weight_gram);
        OLED_ShowString(0, 0, OLED_text, 16);

        if (DHT11_status == DHT11_STATUS_OK)
        {
          snprintf(OLED_text, sizeof(OLED_text),
                   "T:%uC H:%u%%", (unsigned int)DHT11_temperature,
                   (unsigned int)DHT11_humidity);
        }
        else
        {
          snprintf(OLED_text, sizeof(OLED_text),
                   "DHT ERR:%u", (unsigned int)DHT11_status);
        }
        OLED_ShowString(0, 20, OLED_text, 16);

        snprintf(OLED_text, sizeof(OLED_text),
                 "NET:%ld", (long)HX711_net);
        OLED_ShowString(0, 40, OLED_text, 16);
      }
      else if (HX711_status == HX711_STATUS_TIMEOUT)
      {
        OLED_ShowString(0, 20, "TIMEOUT", 16);
        OLED_ShowString(0, 40, "CHECK DT/SCK", 16);
      }
      else
      {
        OLED_ShowString(0, 20, "BAD PARAM", 16);
      }

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
