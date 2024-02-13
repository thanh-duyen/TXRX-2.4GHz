#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define MODE_SELECT_PIN 36
#define JOYS_LEFT_X_PIN 32
#define JOYS_LEFT_Y_PIN 35
#define JOYS_RIGHT_X_PIN 34
#define JOYS_RIGHT_Y_PIN 39
const uint8_t JOYS_PIN[] = {32,35,34,39};
#define VOLT_PIN 33
#define EN_A_PIN 27
#define EN_B_PIN 14
#define CE_PIN 12
#define CSN_PIN 13
#define CS_LCD_PIN 16
#define RST_LCD_PIN 17
#define DC_LCD_PIN 21

#define ROW_OUTPUT_NUM 4
#define COL_INPUT_NUM 3
const uint8_t ROW_OUTPUT_PIN[ROW_OUTPUT_NUM] = {5,2,15,22};
const uint8_t COL_INPUT_PIN[COL_INPUT_NUM] = {26,25,4};
bool button_status[ROW_OUTPUT_NUM][COL_INPUT_NUM] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
uint8_t row_output = 0;
#define TIMES_JUDGEMENT_BUTTON 10

#define JOYSTICK_NUM 4
struct sData{
  sData(){
    joys[0] = joys[1] = joys[2] = joys[3] = 0;
    for(uint8_t i = 0; i < ROW_OUTPUT_NUM*COL_INPUT_NUM; i++){
      button[i] = 0;
    }
    encoder_value = 0;
    encoder_postion = 0;
    mode_sel1 = mode_sel2 = 0;
    is_triggered = false;
  }
  uint16_t joys_base[JOYSTICK_NUM];
  int16_t joys[JOYSTICK_NUM];
  bool button[ROW_OUTPUT_NUM*COL_INPUT_NUM];
  int8_t encoder_value;
  int8_t encoder_postion;
  bool mode_sel1, mode_sel2;
  bool is_triggered;
};
sData rfdata;
bool is_nRF_ok;
uint16_t read_adc(uint8_t pin, uint8_t times){
  uint16_t average = analogRead(pin);
  for(uint8_t j = 0; j < times; j++){
    average = (average+analogRead(pin))/2;
  }
  return average;
}
