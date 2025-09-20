#include <Wire.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Adafruit_SHTC3.h>
#include "background.h"
#include "fonts.h"
#include "esp_sleep.h"
#include "PCF85063A-SOLDERED.h" // Soldered PCF85063A RTC library

// ----------- EPD pins (ESP32-S3) -----------
#define EPD_DC     10
#define EPD_CS     11
#define EPD_SCK    12
#define EPD_MOSI   13
#define EPD_RST     9
#define EPD_BUSY    8
#define EPD_PWR     6    // ACTIVE-LOW (ON = LOW)
#define VBAT_PWR    17   // rail enable (ON = HIGH)

// ----------- I2C pins -----------
#define I2C_SDA     47
#define I2C_SCL     48

// ----------- Settings -----------
#define SPI_CLOCK_HZ   4000000
#define SLEEP_SECONDS  60

// ----------- EPD (1.54" D67) -----------
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ----------- Sensors / RTC -----------
Adafruit_SHTC3 shtc3;
PCF85063A rtc;

int h,m=0;  // hour, minute
int voltageSegments=0;
String hourStr,minStr,tmp,hum2,tt,dateString; // temperature, humidity
String days[7]={"SU","MO","TU","WE","TH","FR","SA"};
// ----------- EPD full refresh  -----------



 void epdDraw()
{
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.epd2.selectSPI(SPI, SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));

  display.init(115200);
  display.setRotation(0);
  //display.setFont(&DSEG7_Classic_Bold_60);
 
  display.setFullWindow();

  display.firstPage();
  do {
    
    display.drawBitmap(0, 0, backImage, 200, 200, GxEPD_BLACK);

    //LINE BELOW TIME
    display.fillRect(60,137,124,5,GxEPD_BLACK);
    display.fillRect(120,82,60,2,GxEPD_BLACK);
    display.fillRect(10,42,3,129,GxEPD_BLACK);


    //baterry
    display.drawRect(150,8,40,16,GxEPD_BLACK);
    display.drawRect(151,9,38,14,GxEPD_BLACK);
    display.fillRect(190,12,3,7,GxEPD_BLACK);
    for(int i=0;i<voltageSegments;i++)
    display.fillRect(154+(i*7),12,4,8,GxEPD_BLACK);

    //date
     display.fillRoundRect(20,40,95,45,5,GxEPD_BLACK);
     

     //SENSR READINGS temperature symbol
      display.fillRoundRect(35,143,15,40,8,GxEPD_BLACK);
      display.fillCircle(42,173,10,GxEPD_BLACK);
      display.fillRoundRect(37,145,11,36,8,GxEPD_WHITE);
      display.fillCircle(42,173,8,GxEPD_WHITE);
      display.fillRoundRect(40,153,5,25,2,GxEPD_BLACK);
      display.fillCircle(42,173,5,GxEPD_BLACK);

      //SENSR READINGS humidity symbol
      for(int i=0;i<6;i++)
      display.fillCircle(122,170-(i*3),6-i,GxEPD_BLACK);

     
     //week day
    display.fillRoundRect(152,94,30,22,4,GxEPD_BLACK);
   

     //draw time
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&DSEG7_Classic_Bold_36);
    display.setCursor(18, 130);  display.print(tt);

    
    
    display.setFont(&DejaVu_Sans_Condensed_Bold_15);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(27, 57);  display.print("DATE");
  
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(60, 161);  display.print("TEMP");
    display.setCursor(60, 177);  display.print(tmp);
    display.setCursor(135, 161);  display.print("HUM");
    display.setCursor(135, 177);  display.print(hum2);
    display.setCursor(120, 78);  display.print("SHTC3");

    display.setTextColor(GxEPD_WHITE);
    display.setCursor(156, 110);  display.print(days[rtc.getWeekday()]);
    
   
    display.setFont(&DejaVu_Sans_Condensed_Bold_18);
    display.setCursor(27, 76);  display.print(dateString);
     
     display.setTextColor(GxEPD_BLACK);
    display.setFont(&DejaVu_Sans_Condensed_Bold_23);
    display.setCursor(120, 62);  display.print("TIME");
  
  } while (display.nextPage());
}


void setup() {

  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)VBAT_PWR);
  gpio_hold_dis((gpio_num_t)EPD_PWR);
 
  // turn on batery
  pinMode(VBAT_PWR, OUTPUT);
  digitalWrite(VBAT_PWR,  HIGH);

  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR,LOW);

  pinMode(3, OUTPUT);
  digitalWrite(3,  HIGH);
  
  delay(10);

  Wire.begin(I2C_SDA, I2C_SCL);
  rtc.begin();

  // set rtc
  //rtc.setDate(5, 19, 9, 2025);                     // NOTE: weekday first
  //rtc.setTime(19, 57, 15);

  h = rtc.getHour();
  m = rtc.getMinute();
  if(h<10) hourStr="0"+String(h); else hourStr=String(h);
  if(m<10) minStr="0"+String(m);  else minStr=String(m);
  tt=hourStr+":"+minStr; // tt is string that contain time in 00:00 format 
  
  dateString=String(rtc.getDay())+"/"+String(rtc.getMonth())+"/"+String(rtc.getYear()-2000);

  // SHTC3 temperature sensor begin
  shtc3.begin();
  
  // read sensor
  sensors_event_t hum, temp;
  bool ok = shtc3.getEvent(&hum, &temp);
  if (!ok) {  delay(5); ok = shtc3.getEvent(&hum, &temp); }

  tmp=String(temp.temperature);
  hum2=String(hum.relative_humidity);
  
  voltageSegments=map(readBatteryVoltage(),3100,4200,0,5);
  
  //draw content on screen
  epdDraw();

  // lets prepare to sleep for 60 sec
  display.hibernate();
  digitalWrite(EPD_PWR,1);
  gpio_hold_en((gpio_num_t)VBAT_PWR);   // drži EPD_PWR u OFF stanju
  gpio_hold_en((gpio_num_t)EPD_PWR); 
  gpio_deep_sleep_hold_en();
  delay(5);
 
  // MCU deep sleep 
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

int readBatteryVoltage()
{
  int pin = 4;                                // ADC pin
  analogReadResolution(12);                   // 0..4095
  analogSetPinAttenuation(pin, ADC_11db);     // do ~3.3 V na ADC-u (dovoljno za Vbat/2)
  int mv = analogReadMilliVolts(pin);         // mV na ADC pinu (kalibrirano)
  // Vbat = Vpin * (Rtop+Rbottom)/Rbottom  → 200k/200k = *2.0
  return 1000*(mv / 1000.0f) * ((200000.0f + 200000.0f) / 200000.0f);
}

void loop() { /* not used */ }
