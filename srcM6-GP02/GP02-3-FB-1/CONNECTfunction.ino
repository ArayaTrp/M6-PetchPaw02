void Wifi_NTP_fBstart() {
  // เริ่มเชื่อมต่อ WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");

  int timeout = 0;  // ตัวนับเวลา
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    // รอ 10 วินาที (20 x 500ms)
    delay(500);
    Serial.print(".");
    timeout++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

    // NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();

    // Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // Web server
    server.on("/read", handleRead);
    server.begin();
    
    if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("signUp OK");
      signupOK = true;
    } else {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }
    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);

    lcd.setCursor(0, 0);
    lcd.print("Firebase Ready!");
    delay(1000);

  } else {
    Serial.println("");
    Serial.println("WiFi connection failed!");
    // เปลี่ยนไป OFFLINE MODE
    offlineMode();
  }
}