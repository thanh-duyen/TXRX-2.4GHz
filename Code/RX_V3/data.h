#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
#include <Servo.h>
#include <EEPROM.h>

#define SECOND 1000
#define CE_PIN 12
#define CSN_PIN 13

#define IN1_PIN 32
#define IN2_PIN 33
#define IN3_PIN 25
#define IN4_PIN 26

#define FRONT_LED_PIN 16
#define BACK_LED_PIN 4
#define LEFT_LED_PIN 5
#define RIGHT_LED_PIN 2
const uint8_t LED_PIN[4] = {FRONT_LED_PIN, BACK_LED_PIN, LEFT_LED_PIN, RIGHT_LED_PIN};

/* Not change these define */
const uint8_t MOTOR_PIN[4] = {IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN};
typedef enum eMotor{
  emMotor12 = 1,
  emMotor34 = 3
};
/* end */

#define SERVO_PIN 14
#define VOLT_PIN 35

#define BUTTON_NUM 12
struct sData{
  sData(){
    joys_leftX = joys_leftY = joys_rightX = joys_rightY = 0;
    mode_sel1 = mode_sel2 = 0;
    time_lastRecive = 0;
    for(uint8_t i = 0; i < BUTTON_NUM; i++){
      button[i] = 0;
    }
  }
  int16_t joys_leftX, joys_leftY, joys_rightX, joys_rightY;
  bool button[BUTTON_NUM];
  int8_t encoder_postion; // 0 to 19 (encoder 20 PPR)
  bool mode_sel1, mode_sel2;
  uint32_t time_lastRecive;
};
sData rfdata;
bool is_nRF_ok;

typedef enum eDir{
  emForward,
  emBackward
};

uint32_t time_blink = 0;
uint32_t time_delay = 200;
int8_t is_left_on = 0;

uint8_t turn_value = 30;
uint8_t base_postion = 40;
uint8_t is_reserve = 0;
uint8_t battery_percent = 100;
