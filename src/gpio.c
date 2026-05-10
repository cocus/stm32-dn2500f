/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DBG_LED_Pin|RDRV0_Pin|RDRV1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RDRV2_Pin|LCD2_CE_Pin|LDRV3_Pin|RDRV3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LDRV2_Pin|LDRV1_Pin|LCD_CE_Pin|LDRV0_Pin
                          |LEDS0_Pin|LEDS1_Pin|LEDS2_Pin|LEDS3_Pin
                          |LEDS4_Pin|LDRV4_Pin|LDRV5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DBG_LED_Pin RDRV0_Pin RDRV1_Pin */
  GPIO_InitStruct.Pin = DBG_LED_Pin|RDRV0_Pin|RDRV1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : RDRV2_Pin LCD2_CE_Pin LDRV3_Pin RDRV3_Pin */
  GPIO_InitStruct.Pin = RDRV2_Pin|LCD2_CE_Pin|LDRV3_Pin|RDRV3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LDRV2_Pin LDRV1_Pin LCD_CE_Pin LDRV0_Pin
                           LEDS0_Pin LEDS1_Pin LEDS2_Pin LEDS3_Pin
                           LEDS4_Pin LDRV4_Pin LDRV5_Pin */
  GPIO_InitStruct.Pin = LDRV2_Pin|LDRV1_Pin|LCD_CE_Pin|LDRV0_Pin
                          |LEDS0_Pin|LEDS1_Pin|LEDS2_Pin|LEDS3_Pin
                          |LEDS4_Pin|LDRV4_Pin|LDRV5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : K0_Pin K1_Pin K2_Pin K3_Pin */
  GPIO_InitStruct.Pin = K0_Pin|K1_Pin|K2_Pin|K3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : K4_Pin K5_Pin K6_Pin K7_Pin */
  GPIO_InitStruct.Pin = K4_Pin|K5_Pin|K6_Pin|K7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
