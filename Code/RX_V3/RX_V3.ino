#include "data.h"
RF24 radio(CE_PIN, CSN_PIN);
Servo servo;

void read_battery(){
  static uint32_t time_read = 0;
  static uint8_t last_battery = 100;
  if(millis() < time_read)
    return;
  time_read = millis() + 30*SECOND;
  uint16_t average = analogRead(VOLT_PIN);
  for(uint8_t i = 0; i < 5; i++){
    average = (average + analogRead(VOLT_PIN))/2;
  }
  /* Bat: 7.78, readed: 1.82 */
  battery_percent = ((average*3.263/4096)*4.27 - 6.4)/2*100;
  if(battery_percent > last_battery)
    battery_percent = last_battery;
  last_battery = battery_percent;
}
void motor_control(eMotor motor, eDir dir, uint8_t pwm){
  if(motor != emMotor12 && motor != emMotor34)
    return;
  if(dir == emForward){
    digitalWrite(MOTOR_PIN[motor], LOW);
    ledcWrite(motor, pwm);
  }
  else{
    digitalWrite(MOTOR_PIN[motor], pwm ? HIGH:LOW);
    ledcWrite(motor, pwm);
  }
}
bool send_rf(String data, int16_t times){
  radio.stopListening();
  char text[32] = {"\0"};
  data.toCharArray(text,data.length()+1);
  while(--times >= 0){
    if(radio.write(&text, sizeof(text))){
      radio.startListening();
      return true;
    }
  }
  radio.startListening();
  return false;
}
void print_rfdata(){
  Serial.printf("XL=%d\tYL=%d\tXR=%d\tYR=%d\tMode=%d\tButton=%d%d%d%d%d%d%d%d%d%d%d%d\tEncoder=%d\n",
    rfdata.joys_leftX, rfdata.joys_leftY, rfdata.joys_rightX, rfdata.joys_rightY,
    rfdata.mode_sel1, rfdata.button[0], rfdata.button[1], rfdata.button[2], rfdata.button[3], 
    rfdata.button[4], rfdata.button[5], rfdata.button[6], rfdata.button[7], rfdata.button[8], 
    rfdata.button[9], rfdata.button[10], rfdata.button[11], rfdata.encoder_postion);
}
void parse_data(char *data){
  /* byte 0                      |byte 1  |byte 2  |byte 3  |byte 4  |byte 5       |byte 6
   * 0       |12345 |6   |7      |01234567|01234567|01234567|01234567|0123    |456701234567
   * 0       |12345 |6   |7      |89012345|67890123|45678901|01234567|8901    |234567890123
   * is valid|en_pos|mode|bat_req|joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|button[0:10]
   */
  if((int)data[0]  & 0x1 == 0)
    return;
  rfdata.time_lastRecive = millis();
  rfdata.encoder_postion = (((int)data[0] >> 1) & 0x1F);
  rfdata.mode_sel1 = ((int)data[0] >> 6) & 0x1;
  if(((int)data[0] >> 7) & 0x1){
    send_rf(String(battery_percent) + "%", 1);
  }
  rfdata.joys_leftX = (uint8_t)data[1];
  rfdata.joys_leftY = (uint8_t)data[2];
  rfdata.joys_rightX = (uint8_t)data[3];
  rfdata.joys_rightY = (uint8_t)data[4];
  if((((int)data[5] >> 0) & 0x1) == 0) rfdata.joys_leftX *= -1;
  if((((int)data[5] >> 1) & 0x1) == 0) rfdata.joys_leftY *= -1;
  if((((int)data[5] >> 2) & 0x1) == 0) rfdata.joys_rightX *= -1;
  if((((int)data[5] >> 3) & 0x1) == 0) rfdata.joys_rightY *= -1;

  for(uint8_t i = 4; i < 8; i++){
    rfdata.button[i-4] = ((int)data[5] >> i) & 0x1;
  }
  for(uint8_t i = 0; i < 8; i++){
    rfdata.button[i+4] = ((int)data[6] >> i) & 0x1;
  }

  motor_control(emMotor12, rfdata.joys_leftY > 0 ? emForward:emBackward, rfdata.joys_leftY);
  int8_t temp = is_reserve ? 1:-1;
  int16_t value = map(rfdata.joys_rightX,-255,255,base_postion-turn_value*temp,base_postion+turn_value*temp);
  servo.write(value);
//  print_rfdata();
}
void read_nRF(){
  if(radio.available()){
    char text[8] = "";
    radio.read(&text, sizeof(text));
    String str = String(text);
    if(text[0] & 0x1){
      parse_data(text);
    }
    else{
      if(str.indexOf("$") >= 0){ // tx reads parameter
        if(str.indexOf("$A") >= 0){
          String data = String(EEPROM.read(0))+" "+String(EEPROM.read(1))+" "+String(EEPROM.read(2))+" ";
          delay(50); bool is_ok = send_rf(data,10);
        }
        else{
          uint8_t value = uint8_t(str.charAt(str.indexOf("$") + 1))-48;
          value = EEPROM.read(value); delay(50);
          bool is_ok = send_rf(String(value),10);
        }
      }
      else if(str.indexOf("#") >= 0){ // tx sets parameter
        String buff = "";
        uint8_t i = str.indexOf("#") + 2;
        while(i < str.length()){
          buff += str.charAt(i);
          i++;
        }
        EEPROM.write(uint8_t(str.charAt(str.indexOf("#")+1))-48, buff.toInt());
        EEPROM.commit();
        if(str.charAt(str.indexOf("#") + 1) == '0'){
          servo.write(buff.toInt());
          base_postion = buff.toInt();
        }
        else if(str.charAt(str.indexOf("#") + 1) == '1'){
          turn_value = buff.toInt();
        }
        else if(str.charAt(str.indexOf("#") + 1) == '2'){
          is_reserve = buff.toInt();
        }
      }
      else{
        Serial.printf("Recived an unknow command: %s\n", str);
      }
    }
  }
}
void leds_handler(){
  if(millis() >= rfdata.time_lastRecive + 1000){
    if(millis() >= time_blink){
      time_blink = millis() + 200 + is_left_on*800;
      is_left_on = !is_left_on;
      digitalWrite(LEFT_LED_PIN, is_left_on);
      digitalWrite(RIGHT_LED_PIN, is_left_on);
      motor_control(emMotor12, emBackward, 0); // stop motor
      servo.write(base_postion);
    }
  }
  else if(rfdata.mode_sel1 == 1){
    if(millis() >= time_blink){
      is_left_on = !is_left_on;
      if(is_left_on){
        digitalWrite(LEFT_LED_PIN,HIGH);
        digitalWrite(RIGHT_LED_PIN,LOW);
      }
      else{
        digitalWrite(LEFT_LED_PIN,LOW);
        digitalWrite(RIGHT_LED_PIN,HIGH);
      }
      time_blink = millis() + time_delay;
    }
  }
  else {
    digitalWrite(LEFT_LED_PIN, rfdata.button[10]);
    digitalWrite(RIGHT_LED_PIN, rfdata.button[10]);
  }
  
  digitalWrite(FRONT_LED_PIN,rfdata.button[9]);
  digitalWrite(BACK_LED_PIN,rfdata.button[11]);
}
void setup(){
  Serial.begin(115200);
  EEPROM.begin(50);
  if(radio.begin()){
    is_nRF_ok = true;
    Serial.println("nRF is OK!");
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_2MBPS);
    radio.openWritingPipe(0xECECECECEC);
    radio.openReadingPipe(1, 0xECECECECEC);
    radio.setPALevel(RF24_PA_MAX);
    radio.startListening();
  }
  else{
    is_nRF_ok = false;
    Serial.println("nRF is BAD!");
  }
  for(uint8_t i = 0;i < 4; i++){
    pinMode(MOTOR_PIN[i],OUTPUT);
    pinMode(LED_PIN[i],OUTPUT);
  }
  
  base_postion = EEPROM.read(0);
  if(base_postion > 180){
    base_postion = 90;
    EEPROM.write(0, 90);
    EEPROM.commit();
  }
  turn_value = EEPROM.read(1);
  if(turn_value > 90){
    turn_value = 45;
    EEPROM.write(1, 45);
    EEPROM.commit();
  }
  is_reserve = EEPROM.read(2);
  if(is_reserve > 1){
    is_reserve = 1;
    EEPROM.write(2, 1);
    EEPROM.commit();
  }
  servo.attach(SERVO_PIN, 5);
  servo.write(base_postion);
  
  ledcAttachPin(IN1_PIN, emMotor12);
  ledcSetup(emMotor12, 12000, 8);
  ledcWrite(emMotor12, 0);
  ledcAttachPin(IN3_PIN, emMotor34);
  ledcSetup(emMotor34, 12000, 8);
  ledcWrite(emMotor34, 0);
}
void loop(){
  if(is_nRF_ok){
    read_nRF();
  }
  leds_handler();
  read_battery();
}
