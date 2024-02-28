#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
#include <Servo.h>
#include <EEPROM.h>

#define CE_PIN 9
#define CSN_PIN 10
#define IRQ_PIN 2
#define INA_PIN A0
#define INB_PIN 7
#define PWM_BACK_PIN 3
#define PWM_FRONT_PIN 6
#define LED_LFRONT_PIN A5
#define LED_RFRONT_PIN A2
#define LED_LBACK_PIN 4
#define LED_RBACK_PIN 8
#define LED_BLUE_PIN A4
#define LED_RED_PIN A3
#define LED_PIN A1
const uint8_t OUTPUT_PIN[11] = {INA_PIN,INB_PIN,PWM_BACK_PIN,
                              PWM_FRONT_PIN,LED_LFRONT_PIN,LED_RFRONT_PIN,
                              LED_LBACK_PIN,LED_RBACK_PIN,LED_BLUE_PIN,LED_RED_PIN,LED_PIN};

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

uint32_t time_blinkBR = 0;
uint32_t time_delayBR = 200;
int8_t is_red = 0;

uint8_t turn_value = 30;
uint8_t base_postion = 40;
uint32_t time_blink_LED = 0;
