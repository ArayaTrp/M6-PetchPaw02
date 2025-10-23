#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h> // สื่อสาร I2C
#include <LiquidCrystal_I2C.h>
#include <time.h>

#include <Firebase_ESP_Client.h> 
// Provide the token generation process info.
#include <addons/TokenHelper.h> 
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#define SENSOR_PIN 34
#define buzzer 26

LiquidCrystal_I2C lcd(0x27, 16, 2);  // address, cols, rows 

WebServer server(80); // Global scope 

// WiFi credentials
// const char* ssid = "NT";
// const char* password = "14021997";
// const char* ssid = "STP-WiFi (2.4G)";
// const char* password = "petcharik217";
const char* ssid = "araya_2.4G";
const char* password = "0993797578"; 
// Firebase config
#define API_KEY "AIzaSyBUwx7bHTBzMP3Jg2mznD3qhuzukksSejI"
#define DATABASE_URL "https://gp02-web-3-default-rtdb.asia-southeast1.firebasedatabase.app/"
// #define USER_EMAIL "numtan26012550@gmail.com"
// #define USER_PASSWORD "araya26012550"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

String status = ""; // green, yellow, red
String statuslight = "";

// --- สำหรับค่าเฉลี่ย ---
float sumPeak_kPa = 0;
int squeezeCount = 0; // นับเฉพาะรอบที่ "จบจริง"
float avrPressure_kPa = 0;

int adcValue = 0;
float pressure_kPa = 0;
float voltage_V = 0;

// --- Peak แต่ละรอบ ---
float peakPressure_kPa = 0;
float peakVoltage_V = 0;
int   peakADCValue = 0;

// --- ค่าสูงสุดต่ำสุดตลอดการใช้งาน ---
float maxPressure_kPa = -9999;
float minPressure_kPa =  9999;
float maxVoltage_V = -9999;
float minVoltage_V =  9999;

// --- สถานะ ---
bool recordedThisSqueeze = false; 
bool wasWaitingHys = true;  // ก่อนหน้านี้ อยู่ในสถานะ "waiting" หรือไม่ true = ยังไม่ได้บีบ

// --- Threshold สำหรับ hysteresis ---
const float START_THR = 0.20;     // เริ่มบีบ
const float END_THR   = 0.10;     // จบการบีบ


char buffer[16]; // arrays
int buzzerTones[3] = { 500, 600, 700 };  // buzzer index = 0 1 2


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
  for (int i = 5; i >= 0; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Press: Offline ");
    lcd.setCursor(0, 1);
    // เลขนับถอยหลัง คำนวณตำแหน่งให้อยู่ตรงกลาง
    int len = String(i).length();
    int col = (16 - len) / 2;  // ตำแหน่งเริ่มพิมพ์
    lcd.setCursor(col, 1);
    lcd.print(i);
    // ตรวจว่ากดปุ่มไหม
    if (digitalRead(15) == LOW) {
      goOffline = true;
      break;
    }
    delay(1000);
  } 

  if (goOffline) {
    offlineMode();  // เรียกฟังก์ชันโหมดออฟไลน์
  } else {
    // Online Mode
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Web Connecting ");
    lcd.setCursor(1, 1);
    lcd.print("Project Ready!");
    delay(2000);

    // เริ่มเชื่อมต่อ WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    unsigned long startAttemptTime = millis();
    bool wifiConnected = false;

    // กำหนด timeout = 10 วินาที
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

      // ตั้งค่าเวลา NTP
      configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");  
      // ↑ 7*3600 = GMT+7 (เวลาไทย)  ถ้าอยากได้ UTC ใช้ 0,0
      struct tm timeinfo;
      int retry = 0, maxRetries = 30; // รอได้สูงสุด ~30 วินาที
      while (!getLocalTime(&timeinfo) && retry < maxRetries) {
        Serial.println("Waiting for NTP time sync...");
        retry++;
        delay(1000);
      }
      if (retry < maxRetries) {
        Serial.println(&timeinfo, "Time set: %Y-%m-%d %H:%M:%S");
      } else {
        Serial.println("Failed to obtain time from NTP");
      }

      server.on("/read", handleRead);
      server.begin();

      // ตั้งค่า Firebase
      // config.api_key = API_KEY;
      // config.database_url = DATABASE_URL;

      // if (Firebase.signUp(&config, &auth, "", "")) {
      //   Serial.println("signUp OK");
      //   signupOK = true;
      // } else {
      //   Serial.printf("%s\n", config.signer.signupError.message.c_str());
      // } 
      // config.token_status_callback = tokenStatusCallback;
      // Firebase.begin(&config, &auth); 
      // lcd.setCursor(0,0);
      // lcd.print("Firebase Ready!");
      // delay(1000);
      
    } else {
      // ถ้าไม่สามารถเชื่อมต่อ WiFi ได้
      Serial.println("\nWiFi connect FAILED! Switching to offline mode...");
      offlineMode();
    }
  }

  
}

void loop() {
  toggleLCDbacklight(); // เช็คการกดสวิตช์เพื่อพักจอ 

  adcValue = analogRead(SENSOR_PIN);                        // 0–4095
  voltage_V = (adcValue / 4095.0) * 3.3;                // แปลงเป็นแรงดัน (หลังผ่าน divider)
  float sensorVoltage = (voltage_V * ((4.7 + 10.0) / 10.0));  // แปลงกลับไปเป็นแรงดันจริงจากเซ็
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

    if (peakVoltage_V > maxVoltage_V) maxVoltage_V = peakVoltage_V;
    if (peakVoltage_V < minVoltage_V) minVoltage_V = peakVoltage_V;
  }

  // จบการบีบ: จาก active -> waiting
  if (!wasWaitingHys && isWaitingNow) {
    if (!recordedThisSqueeze) {
      sumPeak_kPa += peakPressure_kPa;
      squeezeCount++;
      avrPressure_kPa = sumPeak_kPa / squeezeCount;

      recordedThisSqueeze = true;
      // Serial.print("squeeze Count :"); 
      // Serial.print(squeezeCount);
      // Serial.print("| Peak pressure =");
      // Serial.print(peakPressure_kPa, 2);
      // Serial.print(" kPa | avg=");
      // Serial.println(avrPressure_kPa, 2);
    }
    wasWaitingHys = true;
  }


  // Display on LCD
  if (pressure_kPa < 10.0)
    sprintf(buffer, "0%.1f", pressure_kPa);
  else
    sprintf(buffer, "%.1f", pressure_kPa);


  // if(squeezed){
  //   Serial.print("Squeeze Count: ");
  //   Serial.println(squeezeCount);

  //   Serial.print("maxPressure : ");
  
  //   Serial.print("minPressure : ");
  //   Serial.println(minPressure_kPa); 
  //   Serial.print("maxVoltage  : ");
  //   Serial.print(maxVoltage_V); 
  //   Serial.print("minVoltage  : ");
  //   Serial.println(minVoltage_V);  
  //   Serial.println("- - - - - - - - - - - - - - - - - - - - - - - -");
  // }

  server.handleClient();
  // Serial.print(maxPressure_kPa); 

  // LCD row 1
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  lcd.print(" kPa ");
  lcd.print(voltage_V, 2);
  lcd.print(" V ");
  // LCD row 2
  lcd.setCursor(0, 1);
  lcd.print("Force Lv : ");
  if (pressure_kPa == 0) {
    status = "มาเริ่มบีบกันเลย! ✨"; 
    lcd.print("Wait   ");
  } else if (peakADCValue <= 1365) {
    status = "ลองบีบแรงขึ้นอีกนะ 💪";
    statuslight = "🟢";
    lcd.print("LOW   ");
    digitalWrite(27, HIGH); digitalWrite(18, LOW); digitalWrite(19, LOW);
  } else if (peakADCValue <= 2730) {
    status = "ดีขึ้นแล้ว! ✨";
    statuslight = "🟡";
    digitalWrite(27, LOW); digitalWrite(18, HIGH); digitalWrite(19, LOW);
  } else {
    status = "เยี่ยมมาก! 🚀";
    statuslight = "🔴";
    lcd.print("HIGH   ");
    digitalWrite(27, LOW); digitalWrite(18, LOW); digitalWrite(19, HIGH);
  }

  // if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 300 || sendDataPrevMillis == 0)){
  //   sendDataPrevMillis = millis();
  //   // ส่งข้อมูล adcValue ขึ้น Firebase
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/1-pressure", pressure_kPa);
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/2-pressurepeak", peakPressure_kPa);
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/3-pressuremax", maxPressure_kPa);
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/4-pressuremin", minPressure_kPa);

  //   Firebase.RTDB.setFloat(&fbdo, "GP02/1-voltage", voltage);
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/2-voltagepeak", peakVoltage_V);
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/3-voltagemax", maxVoltage_V);
  //   Firebase.RTDB.setFloat(&fbdo, "GP02/4-voltagemin", minVoltage_V);

  //   Firebase.RTDB.setString(&fbdo, "GP02/status", status); 
  // }
  // Serial.println(peakADCValue);
}

void offlineMode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  OFFLINE MODE  ");
  lcd.setCursor(1, 1);
  lcd.print("Ready to Start!");
  Serial.println(" OFFLINE MODE ");
  delay(2000);

  while (true) {  
    toggleLCDbacklight(); // เช็คการกดสวิตช์เพื่อพักจอ 
    adcValue = analogRead(SENSOR_PIN);                        // 0–4095
    voltage_V = (adcValue / 4095.0) * 3.3;                 // แปลงเป็นแรงดัน (หลังผ่าน divider)
    float sensorVoltage = (voltage_V * ((4.7 + 10.0) / 10.0));  // แปลงกลับไปเป็นแรงดันจริงจากเซ็นเซอร์
    float pressure_kPa = ((sensorVoltage)-0.2) / 0.45;  // Datasheet: Pressure (kPa) = (Vout - 0.2) / 0.45

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

      if (peakVoltage_V > maxVoltage_V) maxVoltage_V = peakVoltage_V;
      if (peakVoltage_V < minVoltage_V) minVoltage_V = peakVoltage_V;
    }

    // จบการบีบ: จาก active -> waiting
    if (!wasWaitingHys && isWaitingNow) {
      if (!recordedThisSqueeze) {
        sumPeak_kPa += peakPressure_kPa;
        squeezeCount++;
        avrPressure_kPa = sumPeak_kPa / squeezeCount;

        recordedThisSqueeze = true;
        Serial.print("END squeeze | peak=");
        Serial.print(peakPressure_kPa, 2);
        Serial.print(" kPa | avg=");
        Serial.println(avrPressure_kPa, 2);
      }
      wasWaitingHys = true;
    }
    if (pressure_kPa < 10.0) {  // 01 - 10 kPa
      sprintf(buffer, "0%.1f", pressure_kPa);
    } else {
      sprintf(buffer, "%.1f", pressure_kPa);
    }

    // if(squeezed){
    //   Serial.print("Squeeze Count: ");
    //   Serial.println(squeezeCount);

    //   Serial.print("maxPressure : ");
    //   Serial.print(maxPressure_kPa); 
    //   Serial.print("minPressure : ");
    //   Serial.println(minPressure_kPa); 
    //   Serial.print("maxVoltage  : ");
    //   Serial.print(maxVoltage_V); 
    //   Serial.print("minVoltage  : ");
    //   Serial.println(minVoltage_V);  
    //   Serial.println("- - - - - - - - - - - - - - - - - - - - - - - -");
    // }

    // แสดงผลจอ LCD row1
    lcd.setCursor(0, 0);
    lcd.print(buffer);
    lcd.print(" ");
    lcd.print("kPa");

    lcd.print("  ");
    lcd.print(voltage_V, 2);  // ทศนิยม 2 ตำแหน่ง
    lcd.print(" ");
    lcd.print("V");

    // แสดงผลจอ LCD row2
    lcd.setCursor(0, 1);
    lcd.print("Force Lv : ");

    if (pressure_kPa == 0) {
      status = "Wait"; 
      lcd.print("Wait   ");
    } else if (peakADCValue <= 1365) {
      status = "LOW";
      lcd.print("LOW   ");
      digitalWrite(27, HIGH);
      digitalWrite(18, LOW);
      digitalWrite(19, LOW);
    } else if (peakADCValue <= 2730) {
      status = "MID";
      lcd.print("MID   ");
      digitalWrite(27, LOW);
      digitalWrite(18, HIGH);
      digitalWrite(19, LOW);
    } else {
      status = "HIGH";
      lcd.print("HIGH   ");
      digitalWrite(27, LOW);
      digitalWrite(18, LOW);
      digitalWrite(19, HIGH);
    }
    delay(200);
    // Serial monitor
    // Serial.print("ADC: ");
    // Serial.println(adcValue);
    // Serial.print(" | Voltage: ");
    // Serial.print(voltage, 3);
    // Serial.print(" mV | Pressure: ");
    // Serial.print(pressure_kPa, 2);
    // Serial.println(" kPa"); 
  }
}

void TestingMode() {
  lcd.setCursor(0, 0);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("  Testing Mode  ");
  delay(2000);
  playBuzzer(1);
  testled();
  playBuzzer(1);
  delay(1000);
  testbuzzer();
  playBuzzer(1);
  testbacklight();
  playBuzzer(1);
  testSW();
  playBuzzer(1);
  lcd.clear();
  while (1) {  // ไม่ให้ออกจาก if 
    lcd.setCursor(0, 0);  // เว้นด้านหน้า 2 ช่อง
    lcd.print("Testing Complete");
    lcd.setCursor(0, 1);  // เว้นด้านหน้า 2 ช่อง
    lcd.print(" Please Turn Off");
    delay(2000);
  }
}

void HelloFunction() {
  lcd.setCursor(0, 0);  // index of (column,row)
  lcd.print("<< PETCH PAW >> ");
  lcd.setCursor(0, 1);
  lcd.print("Project Ready!");
  delay(1000);

  digitalWrite(27, HIGH);
  // playBuzzer(0);
  delay(1000);
  digitalWrite(18, HIGH);
  // playBuzzer(1);
  delay(1000);
  digitalWrite(19, HIGH);
  // playBuzzer(2);
  delay(1000);
  digitalWrite(27, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  delay(1000);
}

// BUZZER
void playBuzzer(int index) {
  tone(buzzer, buzzerTones[index]);  // inde = 0 1 2
  delay(300);
  noTone(buzzer);
}

void testSW() {
  lcd.setCursor(0, 0);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("  SWITCH TEST   "); 
 
  lcd.setCursor(0, 1);  // เว้น 1 ช่องเพื่อให้กึ่งกลาง
  lcd.print("    Press  1    ");
  bool x = true;
  while(x == true){
    if (digitalRead(15) == LOW) {
      playBuzzer(1);
      digitalWrite(27,HIGH);
      digitalWrite(18,LOW);
      digitalWrite(19,LOW);  
      x = false;
    } 
  }
   
  lcd.setCursor(0, 1);  // เว้น 1 ช่องเพื่อให้กึ่งกลาง
  lcd.print("    Press  2    ");
  bool y = true;
  while(y == true){
    if (digitalRead(15) == LOW) {
      playBuzzer(1);
      digitalWrite(27,HIGH);
      digitalWrite(18,HIGH);
      digitalWrite(19,LOW);  
      y = false;
    } 
  }
 
  lcd.setCursor(0, 1);  // เว้น 1 ช่องเพื่อให้กึ่งกลาง
  lcd.print("    Press  3    ");
  bool z = true;
  while(z == true){
    if (digitalRead(15) == LOW) {
      playBuzzer(1);
      digitalWrite(27,HIGH);
      digitalWrite(18,HIGH);
      digitalWrite(19,HIGH);  
      z = false;
    } 
  }

  lcd.setCursor(0, 1);  // เว้น 1 ช่องเพื่อให้กึ่งกลาง
  lcd.print("  Press Button  ");
  bool all = true;
  while(all == true){
    if (digitalRead(15) == LOW) {
      playBuzzer(1);
      delay(500);
      digitalWrite(27,LOW);
      digitalWrite(18,LOW);
      digitalWrite(19,LOW); 
      delay(500); 
      digitalWrite(27,HIGH);
      digitalWrite(18,HIGH);
      digitalWrite(19,HIGH); 
      delay(500); 
      digitalWrite(27,LOW);
      digitalWrite(18,LOW);
      digitalWrite(19,LOW); 
      delay(500); 
      digitalWrite(27,HIGH);
      digitalWrite(18,HIGH);
      digitalWrite(19,HIGH); 
      delay(500); 
      digitalWrite(27,LOW);
      digitalWrite(18,LOW);
      digitalWrite(19,LOW); 
      delay(500); 
      all = false;
    } 
  }

  lcd.setCursor(0, 1);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("  << FINISH >>  ");
  delay(1000);
}

void toggleLCDbacklight() {
  bool currentlcdSW = digitalRead(15); 
  // ตรวจการเปลี่ยนจาก HIGH → LOW (คือเพิ่งกด)
  if (lastlcdSW == HIGH && currentlcdSW == LOW) {
    // toggle ไฟ backlight
    lcdBacklightState = !lcdBacklightState;
    if (lcdBacklightState) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
    delay(200); // ป้องกันกดซ้ำไวเกินไป
  } 
  lastlcdSW = currentlcdSW;
}

void testled() {
  lcd.clear();
  lcd.setCursor(0, 0);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("    LED TEST    ");

  for (int i = 0; i <= 2; i++) { 
    digitalWrite(27, HIGH);
    delay(300);
    digitalWrite(18, HIGH);
    delay(300);
    digitalWrite(19, HIGH);
    delay(300);
    digitalWrite(27, LOW); 
    digitalWrite(18, LOW); 
    digitalWrite(19, LOW);
    delay(300); 
     
    digitalWrite(19, HIGH);
    delay(300);
    digitalWrite(18, HIGH);
    delay(300);
    digitalWrite(27, HIGH);
    delay(300);
    digitalWrite(19, LOW); 
    digitalWrite(18, LOW); 
    digitalWrite(27, LOW);
    delay(300); 
  }

  int z = 0;
  while (z <= 2) {
    digitalWrite(18, HIGH);
    digitalWrite(19, HIGH);
    digitalWrite(27, HIGH);
    delay(800);
    digitalWrite(18, LOW);
    digitalWrite(19, LOW);
    digitalWrite(27, LOW);
    delay(800);
    z++;
  } 
  lcd.setCursor(0, 1);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("  << FINISH >>  ");
  delay(1000);
}

void testbuzzer() {
  lcd.clear();
  lcd.setCursor(0, 0);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("   SOUND TEST   ");
  for (int i = 0; i <= 2; i++) {
    for (int i = 0; i <= 2; i++) {
      playBuzzer(i);
    }
    for (int i = 2; i >= 0; i--) {
      playBuzzer(i);
    }
  }

  lcd.setCursor(0, 1);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("  << FINISH >>  ");
  delay(1000);
}

void testbacklight(){
  lcd.clear();
  lcd.setCursor(0, 0);  // เว้นด้านหน้า 2 ช่อง
  lcd.print(" BACKLIGHT TEST ");
  delay(1000);

  lcd.noBacklight();
  delay(1000);
  lcd.backlight();
  delay(1000);
  lcd.noBacklight();
  delay(1000);
  lcd.backlight();
  delay(1000);

  lcd.setCursor(0, 1);  // เว้นด้านหน้า 2 ช่อง
  lcd.print("  << FINISH >>  ");
  delay(1000);
}

void handleRead() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"pressure_kPa\":" + String(pressure_kPa, 2) + ",";
  json += "\"peakPressure_kPa\":" + String(peakPressure_kPa, 2) + ",";
  json += "\"maxPressure_kPa\":" + String(maxPressure_kPa, 2) + ",";
  json += "\"minPressure_kPa\":" + String(minPressure_kPa, 2) + ",";
  // json += "\"voltage_V\":" + String(voltage_V, 2) + ",";
  // json += "\"peakVoltage_V\":" + String(peakVoltage_V, 2) + ",";
  // json += "\"maxVoltage_V\":" + String(maxVoltage_V, 2) + ",";
  // json += "\"minVoltage_V\":" + String(minVoltage_V, 2) + ",";
  json += "\"status\":\"" + status + "\",";
  json += "\"statuslight\":\"" + statuslight + "\",";
  json += "\"squeezeCount\":" + String(squeezeCount) + ",";
  json += "\"avrPressure_kPa\":" + String(avrPressure_kPa, 2);
  json += "}";

  server.send(200, "application/json", json);
}
