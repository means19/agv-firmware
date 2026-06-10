/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_HEARTBEAT_Pin GPIO_PIN_13
#define LED_HEARTBEAT_GPIO_Port GPIOC
#define HC_SR04_Trig_Pin GPIO_PIN_5
#define HC_SR04_Trig_GPIO_Port GPIOA
#define HCSR04_Echo_Pin GPIO_PIN_11
#define HCSR04_Echo_GPIO_Port GPIOB
#define HCSR04_Echo_EXTI_IRQn EXTI15_10_IRQn
#define Motor_L_IN1_Pin GPIO_PIN_12
#define Motor_L_IN1_GPIO_Port GPIOB
#define Motor_L_IN2_Pin GPIO_PIN_13
#define Motor_L_IN2_GPIO_Port GPIOB
#define Motor_R_IN1_Pin GPIO_PIN_14
#define Motor_R_IN1_GPIO_Port GPIOB
#define Motor_R_IN2_Pin GPIO_PIN_15
#define Motor_R_IN2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
