void SERIALrealtimedisplay() {
  Serial.print("จำนวนครั้งทั้งหมด : ");
  Serial.println(squeezeCount);
  Serial.print("จำนวนรอบที่สำเร็จแล้ว : ");
  Serial.println(setCount_set);
  Serial.print("ค่าต่ำสุดในรอบนี้ : ");
  Serial.println(minPressure_set);
  Serial.print("ค่าสูงสุดในรอบนี้ : ");
  Serial.println(maxPressure_set);
  Serial.print("ค่าเฉลี่ยในรอบนี้ : ");
  Serial.println(avrPressure_set);
  Serial.print("🟢 : ");
  Serial.println(String(CountA_set) + " ครั้ง");
  Serial.print("🟡 : ");
  Serial.println(String(CountB_set) + " ครั้ง");
  Serial.print("🔴 : ");
  Serial.println(String(CountC_set) + " ครั้ง");
  Serial.println(" "); 
}
void LCDrealtimedisplay() {
  // Display on LCD
  if (pressure_kPa < 10.0)
    sprintf(buffer, "0%.1f", pressure_kPa);
  else
    sprintf(buffer, "%.1f", pressure_kPa);
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
    lcd.print("MID   ");
    digitalWrite(27, LOW); digitalWrite(18, HIGH); digitalWrite(19, LOW);
  } else {
    status = "เยี่ยมมาก! 🚀";
    statuslight = "🔴";  
    lcd.print("HIGH   ");
    digitalWrite(27, LOW); digitalWrite(18, LOW); digitalWrite(19, HIGH);
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
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

void Countdown5s() {
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
}