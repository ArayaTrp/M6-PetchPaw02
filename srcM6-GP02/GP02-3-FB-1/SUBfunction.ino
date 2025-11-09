void CHECKandLOG() {
  if (squeezeCount_set == 10) {
    setCount_set++;
    // อัปเดตค่าก่อนส่ง
    // maxPressure_set = maxPressure_kPa;
    // minPressure_set = minPressure_kPa;
    // avrPressure_set = avrPressure_kPa;

    // ส่งข้อมูลไป firebase
    // FIREBASEsend();



    // แสดงผลใน serial monitor
    SERIALrealtimedisplay();
    // รีเซ็ตค่า
    squeezeCount_set = 0;
    avrPressure_set = 0;
    maxPressure_set = -999;
    minPressure_set = 999;
    CountA_set = 0;
    CountB_set = 0;
    CountC_set = 0;
  }
}

void FIREBASEsend() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 300 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    // ส่งข้อมูลขึ้น Firebase
    Firebase.RTDB.setFloat(&fbdo, "GP02/1-SqueezeCount", squeezeCount);
    Firebase.RTDB.setFloat(&fbdo, "GP02/2-SetCount", setCount_set);
    Firebase.RTDB.setFloat(&fbdo, "GP02/3-MinPressureSet", minPressure_set);
    Firebase.RTDB.setFloat(&fbdo, "GP02/4-MaxPressureSet", maxPressure_set);
    Firebase.RTDB.setFloat(&fbdo, "GP02/5-AvrSet", avrPressure_set);

    Firebase.RTDB.setFloat(&fbdo, "GP02/6-LEVEL1", CountA_set);
    Firebase.RTDB.setFloat(&fbdo, "GP02/7-LEVEL2", CountB_set);
    Firebase.RTDB.setFloat(&fbdo, "GP02/8-LEVEL3", CountC_set);

    Serial.println("----- ส่งข้อมูลขึ้น Firebase สำเร็จ! -----");
    Serial.println();
  }
}

void handleRead() {
  // แสดงข้อมูลจำนวนครั้งทั้งหมด
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