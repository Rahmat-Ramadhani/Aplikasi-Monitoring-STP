#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SPI.h> 
#include <MFRC522.h>

//RFID Komponen
#define SS_PIN 5  
#define RST_PIN 25

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Gasoline"
#define WIFI_PASSWORD "gasoline"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCICKlgTMN4XAf-SWKrbm1SXFV8DKpoQig"

//1 : AIzaSyCICKlgTMN4XAf-SWKrbm1SXFV8DKpoQig 
//2 : AIzaSyDwr9QjdBVU9-BaKGSsST3XT8Nuiv0Z9sw

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://esp32-s-e5ba8-default-rtdb.asia-southeast1.firebasedatabase.app/" 


//1 : https://esp32-s-e5ba8-default-rtdb.asia-southeast1.firebasedatabase.app/
//2 : https://location-checker-6fd25-default-rtdb.asia-southeast1.firebasedatabase.app/

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// Choose two Arduino pins to use for software serial
int RXPin = 12;
int TXPin = 13;

int GPSBaud = 9600;

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Create a software serial port called "gpsSerial"
SoftwareSerial gpsSerial(RXPin, TXPin);

MFRC522 mfrc522(SS_PIN, RST_PIN);


void setup()
{
  // Start the Arduino hardware serial port at 9600 baud
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);
}

void loop()
{
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      displayInfo();
      

  // If 5000 milliseconds pass and there are no characters coming in
  // over the software serial port, show a "No GPS detected" error
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS detected");
    while(true);
  }
}


void displayInfo()
{
  
  if (gps.location.isValid())
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());

      if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setDouble(&fbdo, "location/latitude", gps.location.lat())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //--------------------------
        // Write an Float number on the database path test/float
    if (Firebase.RTDB.setDouble(&fbdo, "location/longitude", gps.location.lng())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
  }
  }
  else
  {
    Serial.println("Location: Not Available");
  }
//====================================RFID===========================

  if ( ! mfrc522.PICC_IsNewCardPresent())   {
    return; 
  }
  if ( ! mfrc522.PICC_ReadCardSerial())   {
    return;
  }
  //Menampilkan UID ke monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();

  Serial.println(content.substring(1));

  if (Firebase.ready() && signupOK){
    
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setString(&fbdo, "RFID/UID", content.substring(1))){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

//===================================================================
  
  Serial.print("Date: ");
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  }
  else
  {
    Serial.println("Not Available");
  }



  Serial.println();
  Serial.println();
  delay(1000);
}
