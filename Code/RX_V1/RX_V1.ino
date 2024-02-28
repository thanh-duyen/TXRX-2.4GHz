#include "data.h"
RF24 radio(CE_PIN, CSN_PIN);
Servo front;
Servo back;

void parse_data(char *data, uint8_t length){
  /* byte 0  |byte 1  |byte 2  |byte 3  |byte 4       byte 5    byte 6
   * 01234567|01234567|01234567|01234567|0123    |45670 |123456701234|56  |7
   * 01234567|89012345|67890123|45678901|0123    |45678 |901234567890|12  |3(7 bytes = 56 bits)
   * joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|en_pos|button[0:10]|mode|reserved
   */
  rfdata.time_lastRecive = millis();
  rfdata.joys_leftX = (uint8_t)data[0];
  rfdata.joys_leftY = (uint8_t)data[1];
  rfdata.joys_rightX = (uint8_t)data[2];
  rfdata.joys_rightY = (uint8_t)data[3];
  if((((int)data[4] >> 0) & 0x1) == 0) rfdata.joys_leftX *= -1;
  if((((int)data[4] >> 1) & 0x1) == 1) rfdata.joys_leftY *= -1;
  if((((int)data[4] >> 2) & 0x1) == 0) rfdata.joys_rightX *= -1;
  if((((int)data[4] >> 3) & 0x1) == 1) rfdata.joys_rightY *= -1;
  
  uint8_t value = map(rfdata.joys_leftY,-255,255,0,180);
  back.write(value);
  value = map(rfdata.joys_rightX,-255,255,base_postion+turn_value,base_postion-turn_value);
  front.write(value);
  
  int8_t old_postion = rfdata.encoder_postion;
  rfdata.encoder_postion = ((((int)data[4] >> 4) & 0xF) << 1) | ((int)data[5] & 0x1);
  if(rfdata.encoder_postion != old_postion){
    int8_t value_change = 0;
    if(abs(rfdata.encoder_postion - old_postion) <= 1){
      value_change = 1;
      if(old_postion > rfdata.encoder_postion)
        value_change = -1;
    }
    else{
      value_change = 20 - (old_postion - rfdata.encoder_postion);
      if(old_postion < rfdata.encoder_postion){
        value_change = -value_change;
      }
    }
    time_delayBR += value_change;
  }
  
  for(uint8_t i = 1; i < 8; i++){
    rfdata.button[i-1] = ((int)data[5] >> i) & 0x1;
  }
  for(uint8_t i = 0; i < 5; i++){
    rfdata.button[i+7] = ((int)data[6] >> i) & 0x1;
  }
  rfdata.mode_sel1 = ((int)data[6] >> 5) & 0x1;
  rfdata.mode_sel2 = ((int)data[6] >> 6) & 0x1;
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
    String str = String(text);
    if(str.indexOf("$$$") == 0){
      uint8_t value = uint8_t(str.charAt(3))-48;
      value = EEPROM.read(value);
      delay(50);
      bool is_ok = send_rf(String(value),10);
    }
    else if(str.indexOf("###") == 0){
      String buff = "";
      uint8_t i = 4;
      while(i < str.length()){
        buff += str.charAt(i);
        i++;
      }
      EEPROM.write(uint8_t(str.charAt(3))-48, buff.toInt());
      if(str.charAt(3) == '0'){
        front.write(buff.toInt());
        base_postion = buff.toInt();
      }
      else if(str.charAt(3) == '1'){
        turn_value = buff.toInt();
      }
    }
    else{
      parse_data(text,8);
    }
  }
}
void leds_handler(){
  if(rfdata.mode_sel1 == 1){
    if(millis() >= time_blinkBR){
      is_red = !is_red;
      if(is_red){
        digitalWrite(LED_RED_PIN,HIGH);
        digitalWrite(LED_BLUE_PIN,LOW);
      }
      else{
        digitalWrite(LED_RED_PIN,LOW);
        digitalWrite(LED_BLUE_PIN,HIGH);
      }
      time_blinkBR = millis()+time_delayBR;
    }
  }
  else if(is_red != -1){
    digitalWrite(LED_RED_PIN,LOW);
    digitalWrite(LED_BLUE_PIN,LOW);
    is_red = -1;
  }
  digitalWrite(LED_LFRONT_PIN,rfdata.button[9]);
  digitalWrite(LED_RFRONT_PIN,rfdata.button[9]);

  digitalWrite(LED_LBACK_PIN,rfdata.button[11]);
  digitalWrite(LED_RBACK_PIN,rfdata.button[11]);
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
    radio.openWritingPipe(0xECECECECEC);
    radio.openReadingPipe(1, 0xECECECECEC);
    radio.setPALevel(RF24_PA_MAX);
    //radio.printDetails();
    radio.startListening();
  }
  else{
    is_nRF_ok = false;
    Serial.println("nRF is BAD!");
  }
  for(uint8_t i = 0;i < 10; i++){
    pinMode(OUTPUT_PIN[i],OUTPUT);
  }
  uint8_t first_blink_pin[6] = {LED_LFRONT_PIN,LED_RFRONT_PIN,LED_LBACK_PIN,LED_RBACK_PIN,LED_BLUE_PIN,LED_RED_PIN};
  digitalWrite(first_blink_pin[0],HIGH);
  delay(200);
  for(uint8_t i = 1; i < 6; i++){
    digitalWrite(first_blink_pin[i-1],LOW);
    digitalWrite(first_blink_pin[i],HIGH);
    delay(200);
  }
  digitalWrite(first_blink_pin[5],LOW);
  
  base_postion = EEPROM.read(0);
  if(base_postion > 180){
    base_postion = 90;
  }
  turn_value = EEPROM.read(1);
  if(turn_value > 90){
    turn_value = 45;
  }
  front.attach(PWM_FRONT_PIN);
  front.write(base_postion);
  back.attach(PWM_BACK_PIN);
  back.write(90);
}
void loop(){
  if(is_nRF_ok){
    read_nRF();
    if(millis() >= time_blink_LED){
      time_blink_LED = millis() + digitalRead(LED_PIN)*1000+100;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  }
  leds_handler();
}
