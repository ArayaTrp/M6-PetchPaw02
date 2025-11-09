void offlineMode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  OFFLINE MODE  ");
  lcd.setCursor(1, 1);
  lcd.print("Ready to Start!");
  // Serial.println(" OFFLINE MODE ");
  delay(2000);

  while (true) {  
    toggleLCDbacklight(); // เช็คการกดสวิตช์เพื่อพักจอ 

    if (squeezeCount_set >= 10) {
      squeezeCount_set = 0;
    } 

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
        squeezeCount_set++;
        avrPressure_kPa = sumPeak_kPa / squeezeCount;

        recordedThisSqueeze = true;
        // Serial.print("END squeeze | peak=");
        // Serial.print(peakPressure_kPa, 2);
        // Serial.print(" kPa | avg=");
        // Serial.println(avrPressure_kPa, 2);
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