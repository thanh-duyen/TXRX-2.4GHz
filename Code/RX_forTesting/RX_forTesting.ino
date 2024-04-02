#include "data.h"

TFT_eSPI tft = TFT_eSPI();
RF24 radio(CE_PIN, CSN_PIN);

void draw_data(){
  tft.setFreeFont(&FreeMono9pt7b);
  if(!is_initDisplay){
    tft.setTextColor(TFT_BLACK,TFT_WHITE);
    tft.drawCentreString("nRF24L01 reciver testing",240,5,4);
    button[0].initButton(&tft, 90, 176, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "A", 1);
    button[1].initButton(&tft, 110, 210, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "U", 1);
    button[2].initButton(&tft, 150, 250, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "R", 1);
    button[3].initButton(&tft, 110, 290, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "D", 1);
    button[4].initButton(&tft, 70, 250, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "L", 1);
    button[5].initButton(&tft, 390, 176, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "B", 1);
    button[6].initButton(&tft, 410, 210, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "U2", 1);
    button[7].initButton(&tft, 410, 290, 30, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "D2", 1);
    button[8].initButton(&tft, 50, 50, 50, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "1", 1);
    button[9].initButton(&tft, 480-50-70, 50, 50, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "2", 1);
    button[10].initButton(&tft, 480-50, 50, 50, 30, TFT_BLACK, TFT_CYAN, TFT_BLACK, "3", 1);
    
    tft.drawCentreString("Sel 1",140,35,2);
    tft.drawRect(110,50,60,30,TFT_BLACK);
    tft.drawCentreString("Sel 2",210,35,2);
    tft.drawRect(180,50,60,30,TFT_BLACK);

    tft.drawRoundRect(90-40,120-40,81,81,15,TFT_BLACK);
    tft.drawRoundRect(390-40,120-40,81,81,15,TFT_BLACK);
    tft.fillCircle(360,250,30,TFT_CYAN);
  }
  for(int8_t i = 0; i < BUTTON_NUM; i++){
    if(rfdata_new.button[i] != rfdata_old.button[i] || !is_initDisplay){
      button[i].drawButton(rfdata_new.button[i]);
      rfdata_old.button[i] = rfdata_new.button[i];
    }
  }
  if(rfdata_new.mode_sel1 != rfdata_old.mode_sel1 || !is_initDisplay){
    tft.fillRect(110+1+rfdata_old.mode_sel1*29,50+1,29,28,TFT_WHITE);
    tft.fillRect(110+1+rfdata_new.mode_sel1*29,50+1,29,28,TFT_BLACK);
    rfdata_old.mode_sel1 = rfdata_new.mode_sel1;
  }
  if(rfdata_new.mode_sel2 != rfdata_old.mode_sel2 || !is_initDisplay){
    tft.fillRect(180+1+rfdata_old.mode_sel2*29,50+1,29,28,TFT_WHITE);
    tft.fillRect(180+1+rfdata_new.mode_sel2*29,50+1,29,28,TFT_BLACK);
    rfdata_old.mode_sel2 = rfdata_new.mode_sel2;
  }

  if(rfdata_new.joys_leftX != rfdata_old.joys_leftX || rfdata_new.joys_leftY != rfdata_old.joys_leftY || !is_initDisplay){
    tft.fillCircle(rfdata_old.joys_leftX+90,rfdata_old.joys_leftY+120,7,TFT_WHITE);
    tft.fillCircle(rfdata_new.joys_leftX+90,rfdata_new.joys_leftY+120,7,TFT_BLACK);
    rfdata_old.joys_leftX = rfdata_new.joys_leftX;
    rfdata_old.joys_leftY = rfdata_new.joys_leftY;
  }
  if(rfdata_new.joys_rightX != rfdata_old.joys_rightX || rfdata_new.joys_rightY != rfdata_old.joys_rightY || !is_initDisplay){
    tft.fillCircle(rfdata_old.joys_rightX+390,rfdata_old.joys_rightY+120,7,TFT_WHITE);
    tft.fillCircle(rfdata_new.joys_rightX+390,rfdata_new.joys_rightY+120,7,TFT_BLACK);
    rfdata_old.joys_rightX = rfdata_new.joys_rightX;
    rfdata_old.joys_rightY = rfdata_new.joys_rightY;
  }
  if(rfdata_new.encoder_value != 0 || !is_initDisplay){
    float theta = PI*rfdata_new.encoder_postion*18/180.0;
    uint16_t x = 360 + cos(theta)*35;
    uint16_t y = 250 + sin(theta)*35;
    tft.fillCircle(x,y,3,TFT_WHITE);
    rfdata_new.encoder_postion += rfdata_new.encoder_value;
    while(rfdata_new.encoder_postion >= 20){
      rfdata_new.encoder_postion -= 20;
    }
    theta = PI*rfdata_new.encoder_postion*18/180.0;
    x = 360 + cos(theta)*35;
    y = 250 + sin(theta)*35;
    tft.fillCircle(x,y,3,TFT_BLACK);
    rfdata_new.encoder_value = 0;
  }
  is_initDisplay = true;
}
void parse_data(char *data, uint8_t length){
  /* byte 0  |byte 1  |byte 2  |byte 3  |byte 4                |byte 5  byte 6
   * 01234567|01234567|01234567|01234567|0123    |4     |567   |012345670123|45  |67
   * 01234567|89012345|67890123|45678901|0123    |4     |567   |890123456789|01  |23 (7 bytes = 56 bits)
   * joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|en_dir|en_val|button[0:10]|mode|reserved
   */
  rfdata_new.joys_leftX = map((int)data[0],0,255,0,32);
  rfdata_new.joys_leftY = map((int)data[1],0,255,0,32);
  rfdata_new.joys_rightX = map((int)data[2],0,255,0,32);
  rfdata_new.joys_rightY = map((int)data[3],0,255,0,32);
  if((((int)data[4] >> 0) & 0x1) == 0) rfdata_new.joys_leftX *= -1;
  if((((int)data[4] >> 1) & 0x1) == 1) rfdata_new.joys_leftY *= -1;
  if((((int)data[4] >> 2) & 0x1) == 0) rfdata_new.joys_rightX *= -1;
  if((((int)data[4] >> 3) & 0x1) == 1) rfdata_new.joys_rightY *= -1;
  int8_t encoder_dir = (((int)data[4] >> 4) & 0x1) ? 1 : -1;
  rfdata_new.encoder_value = (((int)data[4] >> 5) & 0x7) * encoder_dir;
  rfdata_new.mode_sel1 = ((int)data[6] >> 4) & 0x1;
  rfdata_new.mode_sel2 = ((int)data[6] >> 5) & 0x1;
  for(uint8_t i = 0; i < 2; i++){
    rfdata_new.button[i] = ((int)data[5] >> i) & 0x1;
  }
  for(uint8_t i = 3; i < 8; i++){
    rfdata_new.button[i-1] = ((int)data[5] >> i) & 0x1;
  }
  for(uint8_t i = 0; i < 4; i++){
    rfdata_new.button[i+7] = ((int)data[6] >> i) & 0x1;
  }
  draw_data();
}
void read_nRF(){
  if(radio.available()){
    char text[8] = "";
    radio.read(&text, sizeof(text));
    parse_data(text,8);
  }
}
void setup(){
  Serial.begin(115200);
  printf_begin();
  pinMode(LED_LCD_PIN,OUTPUT);
  pinMode(POWER_EN_PIN,OUTPUT);
  pinMode(4,INPUT_PULLUP);
  digitalWrite(LED_LCD_PIN,HIGH);
  digitalWrite(POWER_EN_PIN,HIGH);
  delay(10);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  
  if(radio.begin()){
    Serial.println("nRF is OK!");
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_2MBPS);
    radio.openWritingPipe(0x55555EEEEE);
    radio.openReadingPipe(1, 0x55555EEEEE);
    radio.setPALevel(RF24_PA_MAX);
    radio.printDetails();
    radio.startListening();
  }
  else{
    Serial.println("nRF is BAD!");
  }
  draw_data();
}
void loop(){
  read_nRF();
}
