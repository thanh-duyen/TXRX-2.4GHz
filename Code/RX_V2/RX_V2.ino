#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"

RF24 radio(CE_PIN, CSN_PIN);

#define CE_PIN 12
#define CSN_PIN 13
#define IRQ_PIN 4
#define BL_A_PIN 2
#define BL_B_PIN 16
#define BR_A_PIN 5
#define BR_B_PIN 17
#define FL_A_PIN 32
#define FL_B_PIN 33
#define FR_A_PIN 22
#define FR_B_PIN 21
#define LED_PIN 14
#define OUTPUT_PIN_AMOUNT 8
const uint8_t OUTPUT_PIN[OUTPUT_PIN_AMOUNT] = {BL_A_PIN,BL_B_PIN,
                                               BR_A_PIN,BR_B_PIN,
                                               FL_A_PIN,FL_B_PIN,
                                               FR_A_PIN,FR_B_PIN};
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
enum eDirection{
  emBackward = 0,
  emForward
};
enum eMotor{
  emBL = 0,
  emBR = 1,
  emFL = 2,
  emFR = 3
};
sData rfdata;
bool is_nRF_ok;

void motor_run(eMotor motor, eDirection dir, uint8_t pwm){
  if(dir == emBackward){
    ledcWrite(motor*2+1, 0);
    ledcWrite(motor*2, pwm);
  }
  else{
    ledcWrite(motor*2+1, 255);
    ledcWrite(motor*2, 255 - pwm);
  }
}
void motor_stop(eMotor motor){
  ledcWrite(motor*2, 0);
  ledcWrite(motor*2+1, 0);
}
void data_handler(){
  static uint32_t t = 0;
  if(millis() >= t){
    int16_t pwm_FL, pwm_FR, pwm_BL, pwm_BR;
    eDirection dir_FL, dir_FR, dir_BL, dir_BR;
    
    pwm_FL = pwm_FR = pwm_BL = pwm_BR = abs(rfdata.joys_leftY);
    if(rfdata.joys_leftY >= 0){
      dir_FL = dir_FR = dir_BL = dir_BR = emBackward;
    }
    else{
      dir_FL = dir_FR = dir_BL = dir_BR = emForward;
    }

    if(pwm_FL < 15 && abs(rfdata.joys_rightX) > 10){
      pwm_FL = pwm_FR = pwm_BL = pwm_BR = abs(rfdata.joys_rightX);
      if(rfdata.joys_rightX >= 0){
        dir_FL = dir_BR = emForward;
        dir_BL = dir_FR = emBackward;
      }
      else{
        dir_FL = dir_BR = emBackward;
        dir_BL = dir_FR = emForward;
      }
    }
    else if(pwm_FL >= 15 && abs(rfdata.joys_rightX) > 10){
      if(dir_FL == emForward){
        if(rfdata.joys_rightX >= 0){
          pwm_FR -= abs(rfdata.joys_rightX);
          if(pwm_FR < 0) pwm_FR = 0;
          pwm_BL = pwm_FR;
        }
        else{
          pwm_FL -= abs(rfdata.joys_rightX);
          if(pwm_FL < 0) pwm_FL = 0;
          pwm_BR = pwm_FL;
        }
      }
      else{
        if(rfdata.joys_rightX >= 0){
          pwm_FL -= abs(rfdata.joys_rightX);
          if(pwm_FL < 0) pwm_FL = 0;
          pwm_BR = pwm_FL;
        }
        else{
          pwm_FR -= abs(rfdata.joys_rightX);
          if(pwm_FR < 0) pwm_FR = 0;
          pwm_BL = pwm_FR;
        }
      }
    }
    
    if(rfdata.button[9] == 1){
      pwm_FL = pwm_FR = pwm_BL = pwm_BR = abs(rfdata.joys_rightX);
      if(rfdata.joys_leftX >= 0){
        dir_FL = dir_BL = emForward;
        dir_BR = dir_FR = emBackward;
      }
      else{
        dir_BL = dir_FL = emBackward;
        dir_BR = dir_FR = emForward;
      }
    }
    motor_run(emBL,dir_BL,pwm_BL);
    motor_run(emBR,dir_BR,pwm_BR);
    motor_run(emFL,dir_FL,pwm_FL);
    motor_run(emFR,dir_FR,pwm_FR);
    
    if(rfdata.button[11] == 1)
      digitalWrite(LED_PIN,HIGH);
    else
      digitalWrite(LED_PIN,LOW);
    t = millis() + 50;
  }
}
void parse_data(char *data){
  /* byte 0  |byte 1  |byte 2  |byte 3  |byte 4       byte 5    byte 6
   * 01234567|01234567|01234567|01234567|0123    |45670 |123456701234|56  |7
   * 01234567|89012345|67890123|45678901|0123    |45678 |901234567890|12  |3(7 bytes = 56 bits)
   * joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|en_pos|button[0:10]|mode|reserved
   */
  if(strlen(data) == 0)
    return;
  rfdata.time_lastRecive = millis();
  rfdata.joys_leftX = (uint8_t)data[0];
  rfdata.joys_leftY = (uint8_t)data[1];
  rfdata.joys_rightX = (uint8_t)data[2];
  rfdata.joys_rightY = (uint8_t)data[3];
  if((((int)data[4] >> 0) & 0x1) == 0) rfdata.joys_leftX *= -1;
  if((((int)data[4] >> 1) & 0x1) == 1) rfdata.joys_leftY *= -1;
  if((((int)data[4] >> 2) & 0x1) == 0) rfdata.joys_rightX *= -1;
  if((((int)data[4] >> 3) & 0x1) == 1) rfdata.joys_rightY *= -1;
  for(uint8_t i = 1; i < 8; i++){
    rfdata.button[i-1] = ((int)data[5] >> i) & 0x1;
  }
  for(uint8_t i = 0; i < 5; i++){
    rfdata.button[i+7] = ((int)data[6] >> i) & 0x1;
  }
}
bool send_rf(String data, uint8_t times){
  radio.stopListening();
  char text[32] = {NULL};
  data.toCharArray(text,data.length()+1);
  while(--times >= 0){
    if(radio.write(&text, sizeof(text))){
      radio.startListening();
      return true;
    }
    delay(1);
  }
  radio.startListening();
  return false;
}
void read_nRF(){
  if(radio.available()){
    char text[8] = "";
    radio.read(&text, sizeof(text));
    parse_data(text);                                                                                                                                             
  }
}
void setup(){
  Serial.begin(115200);
  delay(100);
  Serial.println("Start");
  //printf_begin();
  
  if(radio.begin()){
    is_nRF_ok = true;
    Serial.println("nRF is OK!");
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_2MBPS);
    radio.openWritingPipe(0xEEEEECCCCC);
    radio.openReadingPipe(1, 0xEEEEECCCCC);
    radio.setPALevel(RF24_PA_MAX);
    //radio.printDetails();
    radio.startListening();
  }
  else{
    is_nRF_ok = false;
    Serial.println("nRF is BAD!");
  }
  for(uint8_t i = 0; i < OUTPUT_PIN_AMOUNT; i++){
    ledcAttachPin(OUTPUT_PIN[i], i);
    ledcSetup(i, 700, 8);
    ledcWrite(i, 0);
  }
  pinMode(LED_PIN,OUTPUT);
}
void loop(){
  if(is_nRF_ok){
    read_nRF();
  }
  data_handler();
}
