#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f1xx_hal.h"

#define SERVO_MIN_ANGLE_DEG       0U
#define SERVO_MAX_ANGLE_DEG       180U
#define SERVO_MIN_PULSE_US        500U
#define SERVO_MAX_PULSE_US        2500U
#define SERVO_FEEDER_CLOSED_ANGLE 0U
#define SERVO_FEEDER_OPEN_ANGLE   90U

typedef enum
{
    SERVO_STATUS_OK = 0,
    SERVO_STATUS_BAD_ARGUMENT,
    SERVO_STATUS_HAL_ERROR
} Servo_Status;

Servo_Status Servo_Init(void);
Servo_Status Servo_SetAngle(uint8_t angle_deg);
Servo_Status Servo_OpenFeeder(void);
Servo_Status Servo_CloseFeeder(void);
uint8_t Servo_GetAngle(void);

#endif
