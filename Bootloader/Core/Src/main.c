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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bl_jump.h"
#include "bl_ota.h"
#include <string.h>
#include "ota_image.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//#define OTA_SIZE_ADDR  0x08007FFC
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart5;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint32_t mem_base = 0;
static uint32_t mem_cursor = 0;
static uint32_t ota_image_total_size = 16;
uint8_t setRCmd[6] = {0xC0, 0x00, 0x10, 0x1A, 0x07, 0x44};
GPIO_PinState auxStateR;
uint32_t image_size;
uint8_t image[32];
uint32_t received = 0;
uint32_t remaining;

void ota_mem_set_total_size(uint32_t size)
{
	ota_image_total_size = size;
}

int ota_read_mem(uint8_t *buf, uint32_t len)
{
	if(mem_cursor + len > ota_image_total_size)
		return -1;

	memcpy(buf, &ota_image_bin[mem_base + mem_cursor], len);

	mem_cursor += len;
	return 0;
}


void SleepRMs(void){
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);
	HAL_Delay(50);
}

void WakeUpRMs(void){
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);
	HAL_Delay(50);
}

void SetRModuleParameters(void) {

    auxStateR = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
    if (auxStateR == GPIO_PIN_SET) {

        HAL_UART_Transmit(&huart5, setRCmd, 6, 100);
        HAL_Delay(50);
    }
}

void ReceiveImage(uint32_t size)
{
	HAL_UART_Receive(&huart5, image, size, 100);
}

void ReceiveImageSize()
{
	HAL_UART_Receive(&huart5, (uint8_t*)&image_size, sizeof(image_size), 100);

}

void SendImageSizeNACK(void)
{
	uint8_t nack = 0x00;
	HAL_UART_Transmit(&huart5, &nack, 1, 100);
}

void SendImageSizeACK(void)
{
	uint8_t ack = 0x01;
	HAL_UART_Transmit(&huart5, &ack, 1, 100);
}

void SendChunkACK(void)
{
	uint8_t ack = 0x01;
	HAL_UART_Transmit(&huart5, &ack, 1, 100);
}

void SendEnableOTA(void)
{
	uint8_t ack = 0x01;
	HAL_UART_Transmit(&huart5, &ack, 1, 1000);
}


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
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  //SleepRMs();
  //SetRModuleParameters();
  //WakeUpRMs();

  HAL_Delay(100);


  while (1)
  {
      uint8_t test = 0x55;

      HAL_StatusTypeDef status=  HAL_UART_Transmit(
          &huart5,
          &test,
          1,
          1000
      );
      if (status == HAL_OK)
    	  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
      HAL_Delay(1000);
  }



  if (check_ota_request() == 0)
  {
	  SendEnableOTA();

	  ReceiveImageSize();

	  if (image_size == 0 || image_size > OTA_MAX_SIZE)
	  {
		  SendImageSizeNACK();
		  while (1)
		  {
			  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
			  HAL_Delay(500);

		  }
	  }

	  ota_image_bin_len = image_size;
	  received = 0;

	  SendImageSizeACK();

	  HAL_Delay(100);


	  //if flash memory almost full, these algorithm could be problem.

	  while(received < image_size)
	  {
		  remaining = image_size - received;

		  uint32_t len = (remaining > sizeof(image)) ? sizeof(image) : remaining;

		  ReceiveImage(len);

		  memcpy(&ota_image_bin[received], image, len);

		  received += len;

		  SendChunkACK();
	  }

	  HAL_Delay(100);

	  ota_stream_t stream =
	  {
			  .read = ota_read_mem,
			  .set_total_size = ota_mem_set_total_size
	  };

	  bl_ota_ctx_t ctx;

	  if (bl_ota_run(&ctx, &stream) != 0)
	  {
		  while(1)
		  {
			  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
			  HAL_Delay(100);
		  }
	  } else{
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
	  }
  }

  if (bootloader_is_app_valid() != 0)
  {
	  while (1)
	  {
		  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
		  HAL_Delay(500);
	  }
  }

  JumpToApplication();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	  HAL_Delay(500);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 9600;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PE14 PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PB14 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
