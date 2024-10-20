#include "data.h"

Adafruit_ST7735 tft = Adafruit_ST7735(CS_LCD_PIN, DC_LCD_PIN, RST_LCD_PIN);
RF24 radio(CE_PIN, CSN_PIN);

void read_battery(bool trigger = false){
  static uint32_t time_readBattery = 0;
  if(trigger) time_readBattery = millis();
  if (time_readBattery > millis())
    return;
  time_readBattery = millis() + 2*MINUTE;
  float volt = read_adc(VOLT_PIN,5)*7.25/4095;
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
  tft.fillRect(0,0,40,15,ST77XX_BLACK);
  
  uint8_t percent = (volt-3.2)*100;
  if(last_batPercent > percent)
    last_batPercent = percent;
  tft.print("Bat: "+String(last_batPercent)+"%");
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
  tft.print("ReciverAddr: ");
  tft.setCursor(6,30);
  tft.print("0x"+String(addnRF_value[addnRF_index]));
  tft.setCursor(0,45);
  tft.print("ReciverBa:Unk");
}
void merge_rfdata(sData *data, char *output){
  /* byte 0                      |byte 1  |byte 2  |byte 3  |byte 4  |byte 5       byte 6
   * 0       |12345 |6   |7      |01234567|01234567|01234567|01234567|0123    |456701234567
   * 0       |12345 |6   |7      |89012345|67890123|45678901|01234567|8901    |234567890123
   * is valid|en_pos|mode|bat_req|joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|button[0:10]
   */
  output[0] = 0x1; // is valid to 1
  output[0] = (char)output[0] | (data->encoder_postion << 1);
  if(data->mode_sel1 == 1)
    output[0] = (char)output[0] | (1<<6);
  if(data->batter_request)
    output[0] = (char)output[0] | (1<<7);
  
  output[5] = '\0';
  for(uint8_t i = 0; i < JOYSTICK_NUM; i++){
    output[i+1] = (char)abs(data->joys[i]) & 0xFF;
    if(data->joys[i] >= 0){
      output[5] = (char)output[5] | (1<<i);
    }
  }
  
  for(uint8_t i = 4; i < 8; i++){
    if(data->button[i-4] == 1){
      output[5] = (char)output[5] | (1 << i);
    }
  }
  output[6] = '\0';
  for(uint8_t i = 0; i < 8; i++){
    if(data->button[i+4] == 1){
      output[6] = (char)output[6] | (1 << i);
    }
  }
  output[7] = '\0';
}
void send_rfData(bool is_reset = false){
  /*
   * Input not change -> send every 50ms
   * Input change -> min delay 10ms
   */
  static uint32_t time_run = 0;
  static int32_t times_check = 0;
  static bool is_connected = false;
  static bool last_status = false;
  if(is_reset){
    times_check = 0;
    is_connected = false;
    last_status = false;
    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.fillRect(0,75,80,15,ST77XX_BLACK);
    tft.setCursor(0,60); tft.print("Status:");
    tft.setCursor(6,75); tft.print("UnConnect");
    return;
  }
  if(!is_nRF_ok)
    return;
  if(time_run > millis())
    return;
  time_run = millis() + fresh_time;
  char text[9] = {NULL};
  merge_rfdata(&rfdata, text);
  if(radio.write(&text, sizeof(text))){
    if((text[0] >> 7) & 0x1){
      radio.startListening();
      rfdata.batter_request = 0;
      uint32_t timeout = millis() + fresh_time;
      String str = "Unk";
      while(millis() < timeout){
        if(radio.available()){
          char text[32] = "";
          radio.read(&text, sizeof(text));
          str = String(text);
          uint8_t bat = str.toInt();
          if(bat < 100)
            str = add_zero(bat,2) + "%";
          else
            str = String(bat);
        }
      }
      tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
      tft.setCursor(60,45);
      tft.print(str);
      radio.stopListening();
    }
    if(is_connected == false) times_check = 0;
    is_connected = true;
    if(++times_check >= 20){
      if(last_status == false){
        tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
        tft.fillRect(0,75,80,15,ST77XX_BLACK);
        tft.setCursor(6,75); tft.print("Connected");
        last_status = true;
      }
    }
  }
  else{
    if(is_connected == true) times_check = 0;
    is_connected = false;
    if(++times_check >= 5){
      if(last_status == true){
        tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
        tft.fillRect(0,75,80,15,ST77XX_BLACK);
        tft.setCursor(6,75); tft.print("UnConnect");
        last_status = false;
      }
    }
  }
}
String send_rfMesage(String data, eComand cmd, int16_t times, uint32_t timeout = 2000){
  /* byte 0               |byte 1 to byte 31
   * 0        |12345  |67 |
   * must be 0|mes_len|cmd|data
   */
  char text[32] = "";
  char first_byte = (char)(data.length() << 1) | (int)cmd << 6;
  data = (String)first_byte + data;
  if(data.length() >= 32)
    data.toCharArray(text,32);
  else 
    data.toCharArray(text,data.length()+1);
  bool is_ok = false;
  while(--times >= 0){
    if(radio.write(&text, sizeof(text))){
      if(cmd != emGetCmd)
        return "OK";
      radio.startListening();
      timeout = millis() + timeout;
      while(millis() < timeout){
        if(radio.available()){
          radio.read(&text, sizeof(text));
          radio.stopListening();
          return String(text);
        }
      }
    }
  }
  radio.stopListening();
  return "";
}
void print_display(ePage page){
  if(page == emSetting){
    cursor_x = 60;
    cursor_y = 15+0*15+10;
    cursor_width = 17;
    /* base_postion turn_value is_reserve */
    String data_str = send_rfMesage("$A",emGetCmd,1,1000);
    int32_t data_int[3] = {-1, -1, -1};
    uint8_t i = 0, j = 0;
    String buff_str = "";
    while(i < data_str.length() && i < 32 && j < 3){
      if(data_str.charAt(i) != ' '){
        buff_str += data_str.charAt(i);
      }
      else{
        data_int[j++] = buff_str.toInt();
        buff_str = "";
      }
      i++;
    }
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.print("ReciverSettin");
    tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
    tft.setCursor(6,15*1);
    if(data_int[0] != -1){
      data_setting.base_postion = data_int[0];
      if(data_setting.base_postion > 999) data_setting.base_postion = 999;
      tft.print("BasePos :" + add_zero(data_setting.base_postion,3));
    }
    else{
      data_setting.base_postion = ERROR_VALUE;
      tft.print("BasePos :Err");
    }
    tft.setCursor(6,15*2);
    if(data_int[1] != -1){
      data_setting.turn_value = data_int[1];
      if(data_setting.turn_value > 999) data_setting.turn_value = 999;
      tft.print("TurnVal :" + add_zero(data_setting.turn_value,3));
    }
    else{
      data_setting.turn_value = ERROR_VALUE;
      tft.print("TurnVal :Err");
    }
    tft.setCursor(6,15*3);
    if(data_int[2] != -1){
      data_setting.is_reserve = data_int[2];
      if(data_setting.is_reserve > 1) data_setting.is_reserve = 1;
      tft.print("Reserve :" + String(data_setting.is_reserve ? "On ":"Off"));
    }
    else{
      data_setting.turn_value = ERROR_VALUE;
      tft.print("Reserve :Err");
    }

    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.setCursor(0,15*4);
    tft.print("SenderSetting");
    tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
    tft.setCursor(6,15*5);
    tft.print("Fresh :" + add_zero(fresh_time,3) + "ms");
    tft.setCursor(6,15*6);
    tft.print("Rate  :" + String(data_rate + 1) + "MBPS");
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
  for(uint8_t i = 0; i < ROW_OUTPUT_NUM; i++)
    digitalWrite(ROW_OUTPUT_PIN[i],LOW);
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
      if(page != emHome && button_status[row_output][i] == 1){
        uint8_t button = row_output*COL_INPUT_NUM+i;
        if(button == 1 || button == 3 || button == 4 || button == 5){
          uint16_t _cursor_x = cursor_x, _cursor_y = cursor_y;
          uint8_t old_width = cursor_width;
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
            if(button == 1){ // U
              if(cur_index == 4){
                cursor_x = 60;
                cursor_width = 17;
                cur_index -= 2;
                cursor_y -= 30;
              }
              else if(cur_index > 0){
                cur_index--;
                cursor_y -= 15;
                if(cur_index == 3){
                  cursor_x = 6*8;
                  cursor_width = 5;
                }
              }
            }
            else if(button == 4){ // D
              if(cur_index == 2){
                cur_index += 2;
                cursor_y += 30;
                cursor_x = 6*8;
                cursor_width = 5;
              }
              else if(cur_index < 5){
                cur_index++;
                cursor_y += 15;
                if(cur_index == 5) cursor_x = 6*8;
              }
            }
            else if(cur_index == 4){
              if(button == 3 && cursor_x < 60){ // R
                cursor_x = cursor_x + 6;
              }
              else if(button == 5 && cursor_x > 42){ // L
                cursor_x = cursor_x - 6;
              }
            }
            tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
            tft.setCursor(0,15+old_index*15); tft.print(" ");
            tft.setCursor(0,15+cur_index*15); tft.print(">");
          }
          tft.drawLine(_cursor_x,_cursor_y,_cursor_x+old_width,_cursor_y,ST77XX_BLACK);
          tft.drawLine(_cursor_x,_cursor_y+1,_cursor_x+old_width,_cursor_y+1,ST77XX_BLACK);
          tft.drawLine(cursor_x,cursor_y,cursor_x+cursor_width,cursor_y,ST77XX_WHITE);
          tft.drawLine(cursor_x,cursor_y+1,cursor_x+cursor_width,cursor_y+1,ST77XX_WHITE);
        }
        else if(button == 7 || button == 8){ // U2 || D2
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
              if(data_setting.base_postion != ERROR_VALUE){
                data_setting.base_postion += value;
                tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
                tft.setCursor(6,15*1);
                if(send_rfMesage("#0" + String(data_setting.base_postion),emSetCmd,1) != "OK"){
                  data_setting.base_postion -= value;
                  delay(100);
                }
                tft.setCursor(6,15*1);
                tft.print("BasePos :" + add_zero(data_setting.base_postion,3));
              }
            }
            else if(cursor_y == 2*15+10){
              if(data_setting.turn_value != ERROR_VALUE){
                data_setting.turn_value += value;
                tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
                tft.setCursor(6,15*2);
                if(send_rfMesage("#1" + String(data_setting.turn_value),emSetCmd,1) != "OK"){
                  data_setting.turn_value -= value;
                  delay(100);
                }
                tft.setCursor(6,15*2);
                tft.print("TurnVal :" + add_zero(data_setting.turn_value,3));
              }
            }
            else if(cursor_y == 3*15+10){
              if(data_setting.is_reserve != ERROR_VALUE){
                data_setting.is_reserve += value;
                if(data_setting.is_reserve == 1 || data_setting.is_reserve == 0){
                  tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
                  tft.setCursor(6,15*3);
                  if(send_rfMesage("#2" + String(data_setting.is_reserve),emSetCmd,1) != "OK"){
                    data_setting.is_reserve -= value;
                    delay(100);
                  }
                  tft.setCursor(6,15*3);
                  tft.print("Reserve :" + String(data_setting.is_reserve ? "On ":"Off"));
                }
                else{
                  data_setting.is_reserve -= value;
                }
              }
            }
            else if(cursor_y == 5*15+10){
              if((cursor_x-48)/6 == 0)
                fresh_time = fresh_time + value*100;
              else if((cursor_x-48)/6 == 1)
                fresh_time = fresh_time + value*10;
              else
                fresh_time = fresh_time + value;
              if(fresh_time > 999) fresh_time = 999;
              else if(fresh_time < 10) fresh_time = 10;
              tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
              tft.setCursor(6,15*4);
              tft.print("Fresh :" + add_zero(fresh_time,3) + "ms");
            }
            else if(cursor_y == 6*15+10){
              data_rate += value;
              if(data_rate > 1) data_rate = 1;
              else if(data_rate < 0) data_rate = 0;
              tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
              tft.setCursor(6,15*5);
              tft.print("Rate  :" + String(data_rate + 1) + "MBPS");
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
      if(rfdata.mode_sel2 == 1 && page == emHome){
        delay(fresh_time);
        String respond = send_rfMesage("$$",emGetCmd,1,fresh_time);
        if(respond != "KO"){
          page = emSelector;
          print_display(page);
        }
        else{
          tft.setTextColor(ST77XX_YELLOW,ST77XX_BLACK);
          tft.setCursor(0,105); tft.print("Can't setting");
        }
      }
      else if(rfdata.mode_sel2 == 0 && page == emHome){
        tft.fillRect(0,105,80,15,ST77XX_BLACK);
      }
      else if(rfdata.mode_sel2 == 0 && page != emHome){
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
        EEPROM.write(50,fresh_time/255);
        EEPROM.write(51,fresh_time%255);
        EEPROM.write(52,data_rate);
        EEPROM.commit();
        uint8_t address[6] = "abcde";
        for(int8_t i = 4; i >= 0; i--){
          address[4-i] = EEPROM.read(1+addnRF_index*5+i);
        }
        radio.setDataRate((rf24_datarate_e)data_rate);
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
          if(data_setting.base_postion != ERROR_VALUE){
            data_setting.base_postion += value;
            tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
            tft.setCursor(6,15*1);
            if(send_rfMesage("#0" + String(data_setting.base_postion),emSetCmd,1) != "OK"){
              data_setting.base_postion -= value;
              delay(100);
            }
            tft.setCursor(6,15*1);
            tft.print("BasePos :" + add_zero(data_setting.base_postion,3));
          }
        }
        else if(cursor_y == 2*15+10){
          if(data_setting.turn_value != ERROR_VALUE){
            data_setting.turn_value += value;
            tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
            tft.setCursor(6,15*2);
            if(send_rfMesage("#1" + String(data_setting.turn_value),emSetCmd,1) != "OK"){
              data_setting.turn_value -= value;
              delay(100);
            }
            tft.setCursor(6,15*2);
            tft.print("TurnVal :" + add_zero(data_setting.turn_value,3));
          }
        }
        else if(cursor_y == 3*15+10){
          if(data_setting.is_reserve != ERROR_VALUE){
            data_setting.is_reserve += value;
            if(data_setting.is_reserve == 1 || data_setting.is_reserve == 0){
              tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
              tft.setCursor(6,15*3);
              if(send_rfMesage("#2" + String(data_setting.is_reserve),emSetCmd,1) != "OK"){
                data_setting.is_reserve -= value;
                delay(100);
              }
              tft.setCursor(6,15*3);
              tft.print("Reserve :" + String(data_setting.is_reserve ? "On ":"Off"));
            }
            else{
              data_setting.is_reserve -= value;
            }
          }
        }
        else if(cursor_y == 4*15+10){
          if((cursor_x-48)/6 == 0)
            fresh_time = fresh_time + value*100;
          else if((cursor_x-48)/6 == 1)
            fresh_time = fresh_time + value*10;
          else
            fresh_time = fresh_time + value;
          if(fresh_time > 999) fresh_time = 999;
          else if(fresh_time < 10) fresh_time = 10;
          tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
          tft.setCursor(6,15*4);
          tft.print("Fresh :" + add_zero(fresh_time,3) + "ms");
        }
        else if(cursor_y == 5*15+10){
          data_rate += value;
          if(data_rate > 1) data_rate = 1;
          else if(data_rate < 0) data_rate = 0;
          tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
          tft.setCursor(6,15*5);
          tft.print("Rate  :" + String(data_rate + 1) + "MBPS");
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
  time_run = millis() + 20;
  uint16_t average[JOYSTICK_NUM];
  for(uint8_t i = 0; i < JOYSTICK_NUM; i++){
    average[i] = read_adc(JOYS_PIN[i],3);
    if(average[i] < rfdata.joys_base[i])
      average[i] = map(average[i],0,rfdata.joys_base[i],255,-1);
    else
      average[i] = map(average[i],rfdata.joys_base[i],4095,0,-256);
    if(average[i] != rfdata.joys[i]){
      rfdata.joys[i] = average[i];
    }
  }
}
void read_terminal(){
  if(Serial.available()){
    delay(10);
    String str = "";
    while(Serial.available()){
      str += (char)Serial.read();
    }
    str.replace("\r","");
    str.replace("\n","");
    if(send_rfMesage(str, emMessage, 3) == "OK"){
      Serial.println("Sended: " + str);
      delay(100); // delay for a while for reciver handle
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
  if(EEPROM.read(50)*255 + EEPROM.read(51) > 999 || EEPROM.read(50)*255 + EEPROM.read(51) < 10){
    EEPROM.write(50,0);
    EEPROM.write(51,50);
    EEPROM.commit();
  }
  fresh_time = EEPROM.read(50)*255 + EEPROM.read(51);
  if(EEPROM.read(52) > 1){
    EEPROM.write(52,1);
    EEPROM.commit();
  }
  data_rate = EEPROM.read(52);
  if(radio.begin()){
    is_nRF_ok = true;
    radio.setPayloadSize(32);
    radio.setDataRate((rf24_datarate_e)data_rate);
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
  send_rfData(true);
}
void loop(){
  if(millis() >= time_req_bat){
    rfdata.batter_request = 1;
    time_req_bat = millis() + 1*MINUTE;
  }
  if(page == emHome){
    read_joystick();
    send_rfData();
    read_battery();
  }
  read_button();
  read_encoder();
  read_switch();
  read_terminal();
}
