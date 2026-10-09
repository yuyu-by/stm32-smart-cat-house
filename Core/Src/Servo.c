#include "Servo.h"
#include "tim.h"

static uint8_t servo_current_angle = SERVO_FEEDER_CLOSED_ANGLE;

static uint16_t Servo_AngleToPulseUs(uint8_t angle_deg)
{
    uint32_t pulse_range = SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US;

    return (uint16_t)(SERVO_MIN_PULSE_US +
                      ((uint32_t)angle_deg * pulse_range) /
                      SERVO_MAX_ANGLE_DEG);
}

Servo_Status Servo_Init(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1,
                          Servo_AngleToPulseUs(SERVO_FEEDER_CLOSED_ANGLE));

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
        return SERVO_STATUS_HAL_ERROR;
    }

    servo_current_angle = SERVO_FEEDER_CLOSED_ANGLE;
    return SERVO_STATUS_OK;
}

Servo_Status Servo_SetAngle(uint8_t angle_deg)
{
    if (angle_deg > SERVO_MAX_ANGLE_DEG)
    {
        return SERVO_STATUS_BAD_ARGUMENT;
    }

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1,
                          Servo_AngleToPulseUs(angle_deg));
    servo_current_angle = angle_deg;
    return SERVO_STATUS_OK;
}

Servo_Status Servo_OpenFeeder(void)
{
    return Servo_SetAngle(SERVO_FEEDER_OPEN_ANGLE);
}

Servo_Status Servo_CloseFeeder(void)
{
    return Servo_SetAngle(SERVO_FEEDER_CLOSED_ANGLE);
}

uint8_t Servo_GetAngle(void)
{
    return servo_current_angle;
}
