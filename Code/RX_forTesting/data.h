#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include <JPEGDecoder.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"

#define LED_LCD_PIN 27
#define POWER_EN_PIN 16
#define BUZZER_PIN 17
#define VOLT_EN 15
#define VOLT_PIN 14
#define CE_PIN 12
#define CSN_PIN 13
#define IRQ_PIN 35
#define NONE_VALUE 2147483648

#define BUTTON_NUM 11
struct sData{
  sData(){
    joys_leftX = joys_leftY = joys_rightX = joys_rightY = 0;
    mode_sel1 = mode_sel2 = 0;
    time_lastRecive = 0;
    for(uint8_t i = 0; i < BUTTON_NUM; i++){
      button[i] = 0;
    }
    is_triggered = false;
  }
  int16_t joys_leftX, joys_leftY, joys_rightX, joys_rightY;
  bool button[BUTTON_NUM];
  int8_t encoder_value;
  int8_t encoder_postion;
  bool mode_sel1, mode_sel2;
  uint32_t time_lastRecive;
  uint32_t time_lastTransmit;
  bool is_triggered;
};
sData rfdata_new;
sData rfdata_old;

TFT_eSPI_Button button[BUTTON_NUM];

bool is_initDisplay = false;
