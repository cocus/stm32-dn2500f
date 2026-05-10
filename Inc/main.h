/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define MIDI_IN_PORTS_NUM   0x01 // Specify input ports number of your device
#define MIDI_OUT_PORTS_NUM  0x01 // Specify output ports number of your device
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DBG_LED_Pin GPIO_PIN_13
#define DBG_LED_GPIO_Port GPIOC
#define RDRV0_Pin GPIO_PIN_14
#define RDRV0_GPIO_Port GPIOC
#define RDRV1_Pin GPIO_PIN_15
#define RDRV1_GPIO_Port GPIOC
#define ADC_LEFT_Pin GPIO_PIN_0
#define ADC_LEFT_GPIO_Port GPIOA
#define ADC_RIGHT_Pin GPIO_PIN_1
#define ADC_RIGHT_GPIO_Port GPIOA
#define RDRV2_Pin GPIO_PIN_2
#define RDRV2_GPIO_Port GPIOA
#define LCD2_CE_Pin GPIO_PIN_3
#define LCD2_CE_GPIO_Port GPIOA
#define LDRV3_Pin GPIO_PIN_4
#define LDRV3_GPIO_Port GPIOA
#define RDRV3_Pin GPIO_PIN_6
#define RDRV3_GPIO_Port GPIOA
#define LDRV2_Pin GPIO_PIN_0
#define LDRV2_GPIO_Port GPIOB
#define LDRV1_Pin GPIO_PIN_1
#define LDRV1_GPIO_Port GPIOB
#define LCD_CE_Pin GPIO_PIN_10
#define LCD_CE_GPIO_Port GPIOB
#define LDRV0_Pin GPIO_PIN_11
#define LDRV0_GPIO_Port GPIOB
#define K0_Pin GPIO_PIN_12
#define K0_GPIO_Port GPIOB
#define K1_Pin GPIO_PIN_13
#define K1_GPIO_Port GPIOB
#define K2_Pin GPIO_PIN_14
#define K2_GPIO_Port GPIOB
#define K3_Pin GPIO_PIN_15
#define K3_GPIO_Port GPIOB
#define K4_Pin GPIO_PIN_8
#define K4_GPIO_Port GPIOA
#define K5_Pin GPIO_PIN_9
#define K5_GPIO_Port GPIOA
#define K6_Pin GPIO_PIN_10
#define K6_GPIO_Port GPIOA
#define K7_Pin GPIO_PIN_15
#define K7_GPIO_Port GPIOA
#define LEDS0_Pin GPIO_PIN_3
#define LEDS0_GPIO_Port GPIOB
#define LEDS1_Pin GPIO_PIN_4
#define LEDS1_GPIO_Port GPIOB
#define LEDS2_Pin GPIO_PIN_5
#define LEDS2_GPIO_Port GPIOB
#define LEDS3_Pin GPIO_PIN_6
#define LEDS3_GPIO_Port GPIOB
#define LEDS4_Pin GPIO_PIN_7
#define LEDS4_GPIO_Port GPIOB
#define LDRV4_Pin GPIO_PIN_8
#define LDRV4_GPIO_Port GPIOB
#define LDRV5_Pin GPIO_PIN_9
#define LDRV5_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
