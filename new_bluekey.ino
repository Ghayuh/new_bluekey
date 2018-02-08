#include "Arduino.h"
#include "SoftwareSerial.h"
#include "ESP8266WiFi.h"
#include "ESP8266WiFiMulti.h"
#include "ESP8266HTTPClient.h"
#include "ArduinoJson.h"
#include <U8g2lib.h>
#include <Wire.h>

#define TxD         D6
#define RxD         D5
#define SCL         D2
#define SCA         D1
#define selenoid    D0
#define openButton  D7

#define STATE_INIT              0
#define STATE_SEND_COMMAND      1
#define STATE_BLE_PARSER        2
#define STATE_GET_DATA_JSON     3
#define STATE_PROCESSING_DATA   4
#define STATE_END_DATA          5

#define USSD  "Trojan"
#define PASS  "99g4FeyTaKb1P$A"

const char* domain_get = "https://5-dot-inno-cloud.appspot.com/blukey/ble";

String jsonData;
const char* android_id
const char* user_name;
uint8_t jsonObjectCounter;

u8g2_uint_t offset;
u8g2_uint_t width;
const char *text_verify = "verifying.. ";

char receivedData[100];
static uint8_t ndx = 0;
char inputChar = ' ';
static bool recvInProgress = false;
char state = STATE_INIT;
unsigned long currentMillis, scanMillis, prevMillis;
unsigned long retryInterval = 100;
bool f_open = false;
bool newScan = false;

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);

SoftwareSerial bluekeySerial(RxD, TxD);

void setup()
{
    Serial.begin(115200);
    bluekeySerial.begin(115200);
    WiFi.begin(USSD, PASS);
    u8g2.begin();
    u8g2.setFont(u8g2_font_inb30_mr);
    width = u8g2.getUTF8Width(text_verify);
    u8g2.setFontMode(0);

    pinMode(openButton, INPUT);
    attachInterrupt(digitalPinToInterrupt(openButton), interruptButton, RISING);

    pinMode(selenoid, OUTPUT);
    digitalWrite(selenoid, HIGH);

    state = STATE_INIT;
    currentMillis = millis();
    prevMillis = currentMillis;
}

void loop(){
    switch(state)
    {
        case STATE_INIT:
                newScan = true;
                state = STATE_SEND_COMMAND;
                detachButtonIntterupt();
            break;
        case STATE_SEND_COMMAND:
                scanMillis = millis();
                if(newScan)
                {
                    prevMillis = scanMillis;
                    bluekeySerial.print("AT+DISA?");
                    newScan= false;
                }
                else
                {
                    if((scanMillis - prevMillis) >= retryInterval)
                    {
                        prevMillis += retryInterval;
                        bluekeySerial.print("AT+DISA?");
                    }
                }
            break;
        case STATE_BLE_PARSER:
                if(bluekeySerial.available())
                {
                    inputChar = bleParser();
                    if(inputChar == 1)
                    {
                        recvInProgress = false;
                        ndx = 0;
                        //for loop to get data id
                        state = STATE_GET_DATA_JSON;
                    }
                    else if(inputChar == 2)
                    {
                        state = STATE_END_DATA;
                    }
                }
            break;
        case STATE_GET_DATA_JSON:
            break;
        case STATE_PROCESSING_DATA:
            break;
        case STATE_END_DATA:
                state = STATE_INIT;
            break;
        default:
            break;
    }
}

void interruptButton()
{
    //getData
    //compare data
    //desicion to open door
}

char bleParser()
{
  char _startSeparator = ':';
  char _endSeparator = 'O';
  char _endData = 'E';
  char _dataReceive;
  char result = 0;

  _dataReceive = bluekeySerial.read();

  if(recvInProgress == true){   
    if(_dataReceive != _endSeparator)
    {
      receivedData[ndx] = _dataReceive;
      ndx++;
    }
    else
    {
      receivedData[ndx] = '\0';
      result = 1;
    }
  }
  else if (_dataReceive == _startSeparator)
  {
    recvInProgress = true;
  }  
  else if (_dataReceive == _endData)
  {
    result = 2;
  }
  
  return result;
}

void getData()
{
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;

      http.begin(domain_get);
      int httpCode = http.GET();

      if(httpCode > 0){
         jsonData = http.getString();
        }
      http.end();
    }
}

void parseJSON()
{
  StaticJsonBuffer<1200> jsonBuffer;
  JsonArray& root = jsonBuffer.parseArray(jsonData);

  if(!root.success())
  {
    return;
  }

  for(jsonObjectCounter=0;jsonObjectCounter <= 10;jsonObjectCounter++){
      JsonObject& a = root[jsonObjectCounter];

      android_id = a["android_id"];
      user_name = a["user_name"];
    }
}

void advertiseTextAuth()
{
  u8g2_uint_t x;

  u8g2.firstPage();
  do{
      x = offset;
      u8g2.setFont(u8g2_font_logisoso18_tr);
      do{
          u8g2.drawUTF8(x, 26, text_verify);
          x += width;
        } while( x < u8g2.getDisplayWidth() );
    } while( u8g2.nextPage() );

  offset-=1;
  if ( (u8g2_uint_t)offset < (u8g2_uint_t)-width )  
    offset = 0;
    
  delay(10);
}

void advertiseTextBegin()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,18,"<<>> IDEMIA");
  u8g2.sendBuffer();
  delay(1000); 
}

void advertiseTextVerify()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,18, "User Verified");
  u8g2.sendBuffer();
  delay(1000); 
}

void detachButtonIntterupt()
{
  if (f_open)
    {
        f_open = false;
        detachInterrupt(openButton);
        delay(8000);
        digitalWrite(selenoid, HIGH);
        attachInterrupt(digitalPinToInterrupt(openButton), interruptButton, RISING);
    }  
}
