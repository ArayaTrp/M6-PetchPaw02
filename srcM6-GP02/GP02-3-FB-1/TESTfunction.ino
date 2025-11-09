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