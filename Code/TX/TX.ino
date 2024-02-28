#include "data.h"

Adafruit_ST7735 tft = Adafruit_ST7735(CS_LCD_PIN, DC_LCD_PIN, RST_LCD_PIN);
RF24 radio(CE_PIN, CSN_PIN);

void read_battery(bool trigger = false){
  static uint32_t time_readBattery = 0;
  if(trigger) time_readBattery = millis();
  if (time_readBattery > millis())
    return;
  time_readBattery = millis() + 15000;
  float volt = read_adc(VOLT_PIN,3)*6.6/4095.0;
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
  tft.fillRect(0,0,40,15,ST77XX_BLACK);
  uint8_t percent = (volt-3.2)*100;
  tft.print("Bat: "+String(percent)+"%");
}
void print_info(){
  String tmp = "";
  for(uint8_t j = 0; j < 5; j++){
    if(EEPROM.read(addnRF_index*5+1+j) < 0x10) tmp += "0";
      tmp += String(EEPROM.read(addnRF_index*5+1+j),HEX);
  }
  tmp.toUpperCase();
  tmp.toCharArray(addnRF_value[addnRF_index],11);
  
  tft.setCursor(0,15);
  tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
  tft.print("Reciver addr:");
  tft.setCursor(6,30);
  tft.print("0x"+String(addnRF_value[addnRF_index]));
}
void merge_rfdata(sData *data, char *output){
  /* byte 0  |byte 1  |byte 2  |byte 3  |byte 4       byte 5    byte 6
   * 01234567|01234567|01234567|01234567|0123    |45670 |123456701234|56  |7
   * 01234567|89012345|67890123|45678901|0123    |45678 |901234567890|12  |3(7 bytes = 56 bits)
   * joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|en_pos|button[0:10]|mode|reserved
   */
  output[4] = '\0';
  for(uint8_t i = 0; i < JOYSTICK_NUM; i++){
    output[i] = (char)abs(data->joys[i]) & 0xFF;
//    tft.setCursor(0,20*i);
//    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
//    tft.fillRect(0,i*20,80,20,ST77XX_BLACK);
//    tft.print(data->joys[i]);
    if(data->joys[i] > 0){
      output[4] = (char)output[4] | (1<<i);
    }
  }  
  output[4] = (char)output[4] | ((data->encoder_postion >> 1) << 4);
  output[5] = data->encoder_postion & 0x1;
  for(uint8_t i = 0; i < 8; i++){
    if(data->button[i-1] == 1){
      output[5] = (char)output[5] | (1 << i);
    }
  }
  output[6] = '\0';
  for(uint8_t i = 1; i < 5; i++){
    if(data->button[i+7] == 1){
      output[6] = (char)output[6] | (1 << i);
    }
  }
  if(data->mode_sel1 == 1){
    output[6] = (char)output[6] | (1<<5);
  }
  if(data->mode_sel2 == 1){
    output[6] = (char)output[6] | (1<<6);
  }
  output[7] = '\0';
}
void send_rfData(bool is_reset = false){
  /*
   * Input not change -> send every 50ms
   * Input change -> min delay 10ms
   */
  static uint32_t time_run = 0;
  static uint32_t time_last = 0;
  static int32_t times_unsend = 20;
  static bool is_connected = false;
  if(is_reset){
    times_unsend = 20;
    is_connected = false;
    return;
  }
  if(!is_nRF_ok)
    return;
  if((time_run > millis() && !rfdata.is_triggered) || (rfdata.is_triggered && (time_last + 10) >= millis()))
    return;
  time_run = millis()+50;
  time_last = millis();
  char text[9] = {NULL};
  merge_rfdata(&rfdata, text);
  if(radio.write(&text, sizeof(text))){
    rfdata.is_triggered = false;
    if(times_unsend == 1){
      if(is_connected == false){
        tft.setCursor(0,45);
        tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
        tft.fillRect(0,45,80,30,ST77XX_BLACK);
        tft.print("Status:");
        tft.setCursor(6,60);
        tft.print("Connected");
        is_connected = true;
      }
      times_unsend = 0;
    }
    else if(times_unsend > 1){
      times_unsend--;
    }
  }
  else{
    if(times_unsend == 20){
      tft.setCursor(0,45);
      tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
      tft.fillRect(0,45,80,30,ST77XX_BLACK);
      tft.print("Status:");
      tft.setCursor(6,60);
      tft.print("UnConnect");
      times_unsend = 21;
      is_connected = false;
    }
    else if(times_unsend < 20){
      times_unsend++;
    }
  }
}
bool send_rf(String data, uint8_t times){
  char text[32] = {NULL};
  data.toCharArray(text,data.length()+1);
  while(--times >= 0){
    if(radio.write(&text, sizeof(text)))
      return true;
    delay(1);
  }
  return false;
}
String trigger_rf(String data, uint8_t times, uint32_t timeout){
  char text[32] = {NULL};
  data.toCharArray(text,data.length()+1);
  bool is_ok = false;
  while(--times >= 0){
    if(radio.write(&text, sizeof(text))){
      is_ok = true;
      radio.startListening();
      break;
    }
    delay(1);
  }
  if(is_ok == true){
    timeout = millis() + timeout;
    while(millis() < timeout){
      if(radio.available()){
        char text[32] = "";
        radio.read(&text, sizeof(text));
        radio.stopListening();
        return String(text);
      }
    }
  }
  else
    return "Err01";
  radio.stopListening();
  return "Err02";
}
void read_nRF(){
  if(radio.available()){
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
  }
}
void print_display(ePage page){
  if(page == emSetting){
    cursor_x = 60;
    cursor_y = 15+0*15+10;
    cursor_width = 17;
    String para = trigger_rf("$$$0",5,1000);
    data_setting.base_postion = para.toInt();
    para = trigger_rf("$$$1",5,1000);
    data_setting.turn_value = para.toInt();
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.print("Setting");
    tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
    tft.setCursor(6,15*1);
    tft.print("BasePos :" + add_zero(data_setting.base_postion,3));
    tft.setCursor(6,15*2);
    tft.print("TurnVal :" + add_zero(data_setting.turn_value,3));
    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.setCursor(0,cursor_y - 10);
    tft.print(">");
    tft.drawLine(cursor_x,cursor_y,cursor_x+cursor_width,cursor_y,ST77XX_WHITE);
    tft.drawLine(cursor_x,cursor_y+1,cursor_x+cursor_width,cursor_y+1,ST77XX_WHITE);
  }
  else if(page == emSelector){
    cursor_x = 18;
    cursor_y = 15 + addnRF_index*15+10;
    cursor_width = 5;
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.print("ReciverSelect");
    tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
    for(uint8_t i = 0; i < 9; i++){
      tft.setCursor(0,15+i*15);
      String tmp = "";
      for(uint8_t j = 0; j < 5; j++){
        if(EEPROM.read(i*5+1+j) < 0x10) tmp += "0";
        tmp += String(EEPROM.read(i*5+1+j),HEX);
      }
      tmp.toUpperCase();
      tmp.toCharArray(addnRF_value[i],11);
      if(addnRF_index == i){
        tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
        tft.print(">");
        tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
        tft.print(String(i+1)+":"+tmp);
      }
      else tft.print(" "+String(i+1)+":"+tmp);
    }
    tft.drawLine(cursor_x,cursor_y,cursor_x+cursor_width,cursor_y,ST77XX_WHITE);
    tft.drawLine(cursor_x,cursor_y+1,cursor_x+cursor_width,cursor_y+1,ST77XX_WHITE);
  }
}
void read_button(){
  for(uint8_t i = 0; i < ROW_OUTPUT_NUM; i++){
    digitalWrite(ROW_OUTPUT_PIN[i],LOW);
  }
  digitalWrite(ROW_OUTPUT_PIN[row_output],HIGH);
  for(uint8_t i = 0; i < COL_INPUT_NUM; i++){
    uint8_t times = 0;
    while(times < TIMES_JUDGEMENT_BUTTON && digitalRead(COL_INPUT_PIN[i]) != button_status[row_output][i]){
      times++;
      delayMicroseconds(200);
    }
    if(times >= TIMES_JUDGEMENT_BUTTON){
      button_status[row_output][i] = !button_status[row_output][i];
      rfdata.button[row_output*COL_INPUT_NUM+i] = button_status[row_output][i];
      rfdata.is_triggered = true;
      if(page != emHome && button_status[row_output][i] == 1){
        uint8_t button = row_output*COL_INPUT_NUM+i;
        if(button == 1 || button == 3 || button == 4 || button == 5){
          uint16_t _cursor_x = cursor_x, _cursor_y = cursor_y;
          if(page == emSelector){
            uint8_t _addnRF_index = addnRF_index;
            if(button == 1 && addnRF_index > 0){ // U
              addnRF_index--;
              cursor_y = 15 + addnRF_index*15+10;
            }
            else if(button == 4 && addnRF_index < 8){ // D
              addnRF_index++;
              cursor_y = 15 + addnRF_index*15+10;
            }
            else if(button == 3 && cursor_x < 72){ // R
              cursor_x = cursor_x + 6;
            }
            else if(button == 5 && cursor_x > 18){ // L
              cursor_x = cursor_x - 6;
            }
            tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
            tft.setCursor(0,15+_addnRF_index*15); tft.print(" ");
            tft.setCursor(0,15+addnRF_index*15); tft.print(">");
          }
          else{
            uint8_t cur_index = (cursor_y-15-10)/15;
            uint8_t old_index = cur_index;
            if(button == 1 && cur_index > 0){ // U
              cur_index--;
              cursor_y -= 15;
            }
            else if(button == 4 && cur_index < 1){ // D
              cur_index++;
              cursor_y += 15;
            }
            tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
            tft.setCursor(0,15+old_index*15); tft.print(" ");
            tft.setCursor(0,15+cur_index*15); tft.print(">");
          }
          tft.drawLine(_cursor_x,_cursor_y,_cursor_x+cursor_width,_cursor_y,ST77XX_BLACK);
          tft.drawLine(_cursor_x,_cursor_y+1,_cursor_x+cursor_width,_cursor_y+1,ST77XX_BLACK);
          tft.drawLine(cursor_x,cursor_y,cursor_x+cursor_width,cursor_y,ST77XX_WHITE);
          tft.drawLine(cursor_x,cursor_y+1,cursor_x+cursor_width,cursor_y+1,ST77XX_WHITE);
        }
        else if(button == 7 || button == 8){
          int8_t value = 1;
          if(button == 8) value = -1; // D2
          if(page == emSelector){
            addnRF_value[addnRF_index][(cursor_x-18)/6] = (int)addnRF_value[addnRF_index][(cursor_x-18)/6] + value;
            if(addnRF_value[addnRF_index][(cursor_x-18)/6] == '@') addnRF_value[addnRF_index][(cursor_x-18)/6] = '9';
            else if(addnRF_value[addnRF_index][(cursor_x-18)/6] == '/') addnRF_value[addnRF_index][(cursor_x-18)/6] = 'F';
            else if(addnRF_value[addnRF_index][(cursor_x-18)/6] == ':') addnRF_value[addnRF_index][(cursor_x-18)/6] = 'A';
            else if(addnRF_value[addnRF_index][(cursor_x-18)/6] == 'G') addnRF_value[addnRF_index][(cursor_x-18)/6] = '0';
            addnRF_value[addnRF_index][10] = '\0';
            tft.setCursor(0,15+addnRF_index*15);
            tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
            tft.print(">");
            tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
            tft.print(String(addnRF_index+1)+":"+String(addnRF_value[addnRF_index]));
          }
          else{
            if(cursor_y == 1*15+10){
              data_setting.base_postion += value;
              tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
              tft.setCursor(6,15*1);
              tft.print("BasePos :" + add_zero(data_setting.base_postion,3));
              send_rf("###0" + String(data_setting.base_postion),10);
            }
            else if(cursor_y == 2*15+10){
              data_setting.turn_value += value;
              tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
              tft.setCursor(6,15*2);
              tft.print("TurnVal :" + add_zero(data_setting.turn_value,3));
              send_rf("###1" + String(data_setting.turn_value),10);
            }
          }
        }
        else if(button == 9){
          if(page == emSetting){
            page = emSelector;
            print_display(page);
          }
          else{
            page = emSetting;
            print_display(page);
          }
        }
      }
    }
  }
  if(++row_output >= ROW_OUTPUT_NUM){
    row_output = 0;
  }
}
void read_switch(){
  static uint32_t time_run = 0;
  static uint8_t times = 0;
  if(time_run > millis())
    return;
  time_run = millis()+25;
  uint16_t average = analogRead(MODE_SELECT_PIN);
  for(uint8_t i = 0; i < 5; i++){
    average = (analogRead(MODE_SELECT_PIN)+average)/2;
  }
  bool sel_1 = 0, sel_2 = 0;
  if(average > 2550 && average < 2650)
    sel_1 = 1;
  else if(average > 150 && average < 250)
    sel_2 = 1;
  else if(average > 500 && average < 550){
    sel_1 = 1;
    sel_2 = 1;
  }
  if(sel_1 != rfdata.mode_sel1 || sel_2 != rfdata.mode_sel2){
    if(++times >= 20){
      rfdata.mode_sel1 = sel_1;
      rfdata.mode_sel2 = sel_2;
      rfdata.is_triggered = true;
      //Serial.println(String(rfdata.mode_sel1)+String(rfdata.mode_sel2));
      if(rfdata.mode_sel1 == 0 && rfdata.mode_sel2 == 1 && page == emHome){
        page = emSelector;
        print_display(page);
      }
      else if(rfdata.mode_sel1 == 0 && rfdata.mode_sel2 == 0 && page != emHome){
        char temp[3] = "\0\0";
        for(uint8_t a = 0; a < 9; a++){
          for(uint8_t i = 0, k = 0; i < 10 && k < 5; i++, k++){
            temp[0] = addnRF_value[a][i];
            i++;
            temp[1] = addnRF_value[a][i];
            EEPROM.write(1+a*5+k,strtol(temp, 0, 16));
          }
        }
        EEPROM.write(0,addnRF_index);
        EEPROM.commit();
        uint8_t address[6] = "abcde";
        for(int8_t i = 4; i >= 0; i--){
          address[4-i] = EEPROM.read(1+addnRF_index*5+i);
        }
        radio.openWritingPipe(address);
        radio.openReadingPipe(1, address);
        //radio.printDetails();
        page = emHome;
        tft.fillScreen(ST77XX_BLACK);
        print_info();
        read_battery(true);
        send_rfData(true);
      }
    }
  }
  else{
    times = 0;
  }
}
void read_encoder(){
  static byte lastState = 3;
  static uint32_t time_run = 0;
  if(time_run > millis())
    return;
  time_run = millis()+2;
  byte currentState = (digitalRead(EN_A_PIN) << 1) | digitalRead(EN_B_PIN);
  if(currentState != lastState){
    int8_t value = 0;
    if(currentState == 0b00 && lastState == 0b10){
      if(page == emHome){
        rfdata.encoder_postion--;
        if(rfdata.encoder_postion < 0){
          rfdata.encoder_postion = 19;
        }
      }
      value--;
    }
    else if(currentState == 0b10 && lastState == 0b00){
      if(page == emHome){
        rfdata.encoder_postion++;
        if(rfdata.encoder_postion >= 20){
          rfdata.encoder_postion = 0;
        }
      }
      value++;
    }
    if(value != 0){
      if(page == emSelector){
        addnRF_value[addnRF_index][(cursor_x-18)/6] = (int)addnRF_value[addnRF_index][(cursor_x-18)/6] + value;
        if(addnRF_value[addnRF_index][(cursor_x-18)/6] == '@') addnRF_value[addnRF_index][(cursor_x-18)/6] = '9';
        else if(addnRF_value[addnRF_index][(cursor_x-18)/6] == '/') addnRF_value[addnRF_index][(cursor_x-18)/6] = 'F';
        else if(addnRF_value[addnRF_index][(cursor_x-18)/6] == ':') addnRF_value[addnRF_index][(cursor_x-18)/6] = 'A';
        else if(addnRF_value[addnRF_index][(cursor_x-18)/6] == 'G') addnRF_value[addnRF_index][(cursor_x-18)/6] = '0';
        addnRF_value[addnRF_index][10] = '\0';
        tft.setCursor(0,15+addnRF_index*15);
        tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
        tft.print(">");
        tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
        tft.print(String(addnRF_index+1)+":"+String(addnRF_value[addnRF_index]));
      }
      else if(page == emSetting){
        if(cursor_y == 1*15+10){
          data_setting.base_postion += value;
          tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
          tft.setCursor(6,15*1);
          tft.print("BasePos :" + add_zero(data_setting.base_postion,3));
          send_rf("###0" + String(data_setting.base_postion),10);
        }
        else if(cursor_y == 2*15+10){
          data_setting.turn_value += value;
          tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
          tft.setCursor(6,15*2);
          tft.print("TurnVal :" + add_zero(data_setting.turn_value,3));
          send_rf("###1" + String(data_setting.turn_value),10);
        }
      }
    }
    
    lastState = currentState;
  }
}
void read_joystick(){
  static uint32_t time_run = millis();
  if(time_run > millis())
    return;
  time_run = millis() + 50;
  uint16_t average[JOYSTICK_NUM];
  for(uint8_t i = 0; i < JOYSTICK_NUM; i++){
    average[i] = read_adc(JOYS_PIN[i],3);
    if(average[i] < rfdata.joys_base[i])
      average[i] = map(average[i],0,rfdata.joys_base[i],255,-1);
    else
      average[i] = map(average[i],rfdata.joys_base[i],4095,0,-256);
    if(average[i] != rfdata.joys[i]){
      rfdata.joys[i] = average[i];
      //rfdata.is_triggered = true;
    }
  }
}
void setup(){
  Serial.begin(115200);
  EEPROM.begin(100);
  printf_begin();
  tft.initR(INITR_MINI160x80_PLUGIN);
  tft.setSPISpeed(40000000);
  tft.fillScreen(ST77XX_BLACK);

  if(EEPROM.read(0) > 8){
    EEPROM.write(0,0);
    EEPROM.commit();
  }
  addnRF_index = EEPROM.read(0);
  if(radio.begin()){
    is_nRF_ok = true;
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_2MBPS);
    uint8_t address[6] = "abcde";
    for(int8_t i = 4; i >= 0; i--){
      address[4-i] = EEPROM.read(1+addnRF_index*5+i);
    }
    radio.openWritingPipe(address);
    radio.openReadingPipe(1, address);
    radio.setPALevel(RF24_PA_MAX);
    //radio.printDetails();
    radio.stopListening();
  }
  else{
    is_nRF_ok = false;
    tft.setCursor(0,15);
    tft.setTextColor(ST77XX_WHITE,ST77XX_RED);
    tft.print("nRF error");
  }
  for(uint8_t i = 0; i < ROW_OUTPUT_NUM; i++){
    pinMode(ROW_OUTPUT_PIN[i],OUTPUT);
    digitalWrite(ROW_OUTPUT_PIN[i],LOW);
  }
  for(uint8_t i = 0; i < COL_INPUT_NUM; i++){
    pinMode(COL_INPUT_PIN[i],INPUT);
  }
  pinMode(EN_A_PIN,INPUT);
  pinMode(EN_B_PIN,INPUT);
  
  for(uint8_t i = 0; i < JOYSTICK_NUM; i++){
    rfdata.joys_base[i] = analogRead(JOYS_PIN[i]);
    for(uint8_t j = 0; j < 3; j++){
      rfdata.joys_base[i] = (rfdata.joys_base[i]+analogRead(JOYS_PIN[i]))/2;
    }
  }
  print_info();
}
void loop(){
  if(page == emHome){
    read_joystick();
    send_rfData();
    read_battery();
  }
  read_button();
  read_encoder();
  read_switch();
}
