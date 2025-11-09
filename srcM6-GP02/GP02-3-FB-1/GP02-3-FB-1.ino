#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h> // สื่อสาร I2C
#include <LiquidCrystal_I2C.h>
#include "time.h"

#include <Firebase_ESP_Client.h> 
// Provide the token generation process info.
#include <addons/TokenHelper.h> 
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#define SENSOR_PIN 34
#define buzzer 26

LiquidCrystal_I2C lcd(0x27, 16, 2);  // address, cols, rows

WebServer server(80); // Global scope 

const char* ssid = "araya_2.4G";
const char* password = "0993797578"; 

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;   // GMT+7 สำหรับประเทศไทย
const int   daylightOffset_sec = 0;     // ไทยไม่มีเวลาออมแสง

// Firebase config
#define API_KEY "AIzaSyBUwx7bHTBzMP3Jg2mznD3qhuzukksSejI"
#define DATABASE_URL "https://gp02-web-3-default-rtdb.asia-southeast1.firebasedatabase.app/"
// #define USER_EMAIL "numtan26012550@gmail.com"
// #define USER_PASSWORD "araya26012550"
// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ใช้ตอนจะส่งค่าไป firebase
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

String status = ""; // green, yellow, red
String statuslight = "";

// ค่าเฉลี่ย (ทั้งหมด ไม่แยกเซ็ต)
float sumPeak_kPa = 0;
int squeezeCount = 0;  // จำนวนครั้งที่บีบทั้งหมด
float avrPressure_kPa = 0;

int adcValue = 0; 
float pressure_kPa = 0;
float voltage_V = 0;
float sensorVoltage;  

// Peak (ค่าสูงสุดใน 1 ครั้งที่บีบ)
float peakPressure_kPa = 0;
float peakVoltage_V = 0;
int   peakADCValue = 0;

// ค่าสูงสุดต่ำสุด (ทั้งหมด ไม่แยกเซ็ต)
float maxPressure_kPa = -9999;
float minPressure_kPa =  9999;
float maxVoltage_V = -9999;
float minVoltage_V =  9999;

// ข้อมูลรายเซ็ต (1 รอบ)
/* อยากรู้ว่า ใน 1 รอบ 
- มีค่าแรงสูงสุดเท่าไหร่
- มีค่าแรงต่ำสุดเท่าไหร่
- มีค่าแรงเฉลี่ยเท่าไหร่
- บีบได้แรงระดับ A(1) กี่ครั้ง
- บีบได้แรงระดับ B(2) กี่ครั้ง
- บีบได้แรงระดับ C(3) กี่ครั้ง*/
int setCount_set = 0; // จำนวนรอบ
int squeezeCount_set = 0; // จำนวนครั้งในรอบนั้น
int maxPressure_set = -9999; 
int minPressure_set = 9999; 
float avrPressure_set = 0;
int CountA_set = 0; // done
int CountB_set = 0; // done
int CountC_set = 0; // done

// สถานะบีบ - ปล่อย
bool recordedThisSqueeze = false; 
bool wasWaitingHys = true;  // ก่อนหน้านี้ อยู่ในสถานะ "waiting" หรือไม่ true = ยังไม่ได้บีบ

// Threshold สำหรับ hysteresis
const float START_THR = 0.20;     // เริ่มบีบ
const float END_THR   = 0.10;     // จบการบีบ 

char buffer[16]; // arrays
int buzzerTones[3] = { 500, 600, 700 };  // buzzer index = 0 1 2

bool squeezed = false; // flag ป้องกันการนับซ้ำ  

// backlight
bool lcdBacklightState = true;
bool lastlcdSW = HIGH; // การกดสวิตช์เพื่อปิดไฟล์พื้นหลัง lcd
bool goOffline = false; 

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // I2C for LCD
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(27, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(15, INPUT_PULLUP); // สวิตช์

  // กดค้าง 1.5 วิ ก่อนเริ่ม เพื่อเข้า TestingMode();
  if (digitalRead(15) == LOW) {
    delay(1500); // กดค้าง 1.5 วิ
    if (digitalRead(15) == LOW) {
      TestingMode();
    }
  }
  // ถ้าไม่มีการกดค้าง 1.5 วิ > แสดงข้อความทักทาย
  HelloFunction();
  // นับถอยหลัง 5 วิ กดเพื่อเข้าโหมด offline
  Countdown5s();
  // เชื่อมไวไฟ เวลา firebase
  Wifi_NTP_fBstart();
}

void loop() {
  toggleLCDbacklight(); // เช็คการกดสวิตช์เพื่อพักจอ 

  adcValue = analogRead(SENSOR_PIN); // 0–4095
  voltage_V = (adcValue / 4095.0) * 3.3; // แปลงเป็นแรงดัน (หลังผ่าน divider)
  sensorVoltage = (voltage_V * ((4.7 + 10.0) / 10.0)); // แปลงกลับไปเป็นแรงดันจริงจากเซ็
  pressure_kPa = (sensorVoltage - 0.2) / 0.45;

  if (pressure_kPa > 10) pressure_kPa = 10; 
  if (voltage_V > 3.3) voltage_V = 3.3;
  if (pressure_kPa < 0) pressure_kPa = 0; 

  bool isWaitingNow = (pressure_kPa <= END_THR); // จบการบีบแล้ว ( < 0.10 )

  // เริ่มบีบ: จาก waiting -> active
  if (wasWaitingHys && pressure_kPa >= START_THR) { 
    // reset ค่า peak ของรอบใหม่
    peakPressure_kPa = 0;
    peakVoltage_V    = 0;
    peakADCValue     = 0;

    recordedThisSqueeze = false;  
    wasWaitingHys = false;
  }

  // ระหว่างบีบ: อัปเดต peak
  if (!wasWaitingHys) {
    if (pressure_kPa > peakPressure_kPa) peakPressure_kPa = pressure_kPa;
    if (voltage_V    > peakVoltage_V)    peakVoltage_V    = voltage_V;
    if (adcValue     > peakADCValue)     peakADCValue     = adcValue;

    if (peakPressure_kPa > maxPressure_kPa) maxPressure_kPa = peakPressure_kPa;
    if (peakPressure_kPa < minPressure_kPa) minPressure_kPa = peakPressure_kPa; 

    if (peakPressure_kPa > maxPressure_set) maxPressure_set = peakPressure_kPa;
    if (peakPressure_kPa < minPressure_set) minPressure_set = peakPressure_kPa;
  }

  // จบการบีบ: จาก active -> waiting
  if (!wasWaitingHys && isWaitingNow) {
    if (!recordedThisSqueeze) {
      squeezeCount++;
      sumPeak_kPa += peakPressure_kPa; 
      avrPressure_kPa = sumPeak_kPa / squeezeCount;

      squeezeCount_set++;
      avrPressure_set = sumPeak_kPa / squeezeCount_set;

      if (peakADCValue <= 1365) { //(pressure_kPa == 0 && peakADCValue <= 1365)
        CountA_set++;
      } else if (peakADCValue <= 2730) {
        CountB_set++;
      } else { 
        CountC_set++; 
      }

      recordedThisSqueeze = true;
      
    }
    wasWaitingHys = true; 

    
    // เช็คจำนวนครั้งและส่งไป firebase ถ้าครบรอบ
    CHECKandLOG();
  }
  // ตรวจสอบว่ามี คำขอ HTTP (เช่น GET / หรือ POST /led) เข้ามาไหม ถ้ามีจะเรียกฟังก์ชัน server.on("/read", handleRead); ที่เรากำหนดไว้ 
  server.handleClient(); 
  // แสดงผลเรียลไทม์บนชุดอุปกรณ์
  LCDrealtimedisplay(); 
  

}


