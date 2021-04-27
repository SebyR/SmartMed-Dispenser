//SmartMed Dispenser

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>    
#include "RTClib.h"             
#include <DFMiniMp3.h>
#include "EEPROM.h"
#include "ESP32_MailClient.h"

const char* ssid = "XXXX";
const char* password = "xxxxxxxxxx";

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define EEPROM_SIZE 64

#define emailSenderAccount    "xxxxxxxxxxxxxx@gmail.com"    
#define emailSenderPassword   "xxxxxxxxx"
#define emailRecipient        "xxxxxxxxxxxxx@yahoo.com"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "ALERTA"

const int dir = 19;
const int viteza = 18;                // step dir ptr motor
const int box = 25;                   // detectie o cutie
const int cup = 26;                   //detectie pahar
const int pinOk = 35;
const int pinUp = 33;
const int pinDn = 32;
const int batt = A0;
int stare = 0;
int pahar = 0;
int cont1 = 0;
int cont2 = 0;
int cont3 = 0;
int obs = 0;
int fault = 0;
int faults = 0;
int mail = 0;
int addr;
int meniu = 0;
int poz = 1;
int val = 30;
int load = 0;
int level = 0;
int radio = 0;
int pas = 0;
int bat = 0;

SMTPData smtpData;
void sendCallback(SendStatus info);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;                   


class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
  }
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print("Play finished for #");
    Serial.println(track);  
  }
  static void OnPlaySourceOnline(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};
DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial2);

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  Serial.println();
  Serial.println();
  
  pinMode(dir, OUTPUT);
  pinMode(viteza, OUTPUT);
  pinMode(box, INPUT);
  pinMode(cup, INPUT);
  pinMode(pinOk, INPUT);
  pinMode(pinUp, INPUT);
  pinMode(pinDn, INPUT);
  pinMode(batt, INPUT);
  digitalWrite(dir, LOW);
  digitalWrite(viteza, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  EEPROM.begin(EEPROM_SIZE);
  display.display();
  mp3.begin();
  
     
  display.clearDisplay();
  display.display();
  mp3.setVolume(30);
  mp3.setEq(DfMp3_Eq_Rock);
  display.invertDisplay(false);
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("SmartMed"));
  display.println(F("Dispenser"));
  display.display();
  delay(2000);

 addr = EEPROM.read(60);
 
}

void wireless(){
  WiFi.begin(ssid, password);   
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  radio = 0; 
}

void trimiteMail(){
 smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
 smtpData.setSender("URGENT", emailSenderAccount);
 smtpData.setPriority("High");
 smtpData.setSubject(emailSubject);
 smtpData.setMessage("XXXXXXXXX nu si-a luat medicamentele", true);
 smtpData.addRecipient(emailRecipient);
 smtpData.setSendCallback(sendCallback);
 
 if (!MailClient.sendMail(smtpData))
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  //Clear all data from Email object to free memory
  smtpData.empty();
  mail = 0; 
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    mp3.loop(); 
    delay(1);
  }
}

void msgIaPill(){
  digitalWrite(viteza, LOW);                          // ding si mesaj "ia pastilele"
  mp3.playFolderTrack(1, 5);
  waitMilliseconds(1000);
  mp3.playFolderTrack(1, 1);
  waitMilliseconds(5000);
}

void msgNuCup(){
   mp3.playFolderTrack(1, 5);
   waitMilliseconds(1000);
   mp3.playFolderTrack(1, 4);
   waitMilliseconds(5000); 
}

void msgRogPill(){
  mp3.playFolderTrack(1, 5);
  waitMilliseconds(1000);
  mp3.playFolderTrack(1, 2);
  waitMilliseconds(5000); 
}

void msgThx(){
  mp3.playFolderTrack(1, 3);
  waitMilliseconds(1000); 
}

void loop() {
  Serial.print(cont1);
  Serial.print("  ");
  Serial.print(cont2);
  Serial.print("  ");
  Serial.print(cont3);
  Serial.println("  ");
  Serial.println(mail);
  level = analogRead(batt);
  level = map(level, 1900, 2350, 0, 9);

  DateTime now = rtc.now();
  stare = digitalRead(box);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(now.day(), DEC);         
  display.print(F("/"));
  display.print(now.month(), DEC);
  if (WiFi.status() == WL_CONNECTED){ 
   display.print(F("**"));        
  }
  if (WiFi.status() != WL_CONNECTED){
  display.print(F("  "));
  }

  display.print(level);
  display.println(F("0%"));         
 
  display.setTextSize(4);             
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,18);
  display.print(now.hour(), DEC);         
  display.print(F(":"));
  display.println(now.minute(), DEC);
  
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);         
  display.print(F("interval:"));
  display.println(addr);         
  display.display();

 if(digitalRead(pinDn) == LOW && digitalRead(pinUp) == LOW && meniu == 0){
  meniu = 1;
  delay(1000);
 }

 while(meniu == 1){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
    display.print(poz);
    display.print(F(": "));
    display.println(val);
    display.display();
    if(digitalRead(pinUp) == LOW){
      val++;
      delay(200);
      if(val >= 60){val = 60;}
    }
    if(digitalRead(pinDn) == LOW){
      val--;
      delay(200);
      if(val <= 0){val = 0;}
    }
     if (digitalRead(pinOk) == LOW){
      addr = poz;
      EEPROM.write(addr, val);
      EEPROM.commit();
      delay(1000);
//      val = 0;
      poz++;
      if(poz > 4){poz = 1;}
  }
  if(digitalRead(pinUp) == LOW && digitalRead(pinDn) == LOW){
  meniu = 0;
  addr = 1;
  delay(1000);
   }
 }

 if(mail == 2){
  if(radio == 0){
    wireless();
    radio = 1;
  }
  trimiteMail();
 }
  if (now.minute() == byte(EEPROM.read(addr)) && now.second() == 0){    //totul este ok
    pahar = 1;
      if(digitalRead(cup) == LOW){
          digitalWrite(viteza, HIGH);
          stare = LOW;
          delay(100);
              while (stare == 0) {
                digitalWrite(viteza, HIGH);
                stare = digitalRead(box);
                delay(5);
                }
                  msgIaPill();  
                cont2 = 0;
                pahar = 0;
                obs = 1;
                mail = 0;
        }       
    }
        if(digitalRead(cup) == HIGH && pahar == 1){                 //s-a facut timpul si "nu am paharul in aparat"
            cont1++;
        }
              if(cont1 == 1000 && pahar == 1){
                  msgNuCup(); 
                cont1 = 0;
                mail++;
              }
  if(digitalRead(cup) == LOW && pahar == 1 && obs == 0){              //se executa dupa ce a fost pus paharul "ia pastilele"
     digitalWrite(viteza, HIGH);
     stare = LOW;
     delay(100);
        while (stare == 0) {
            digitalWrite(viteza, HIGH);
            stare = digitalRead(box);
            delay(5);
        }
              digitalWrite(viteza, LOW);
                msgIaPill();
            cont1 = 0;
            pahar = 0;
            obs = 1;
            mail = 0;
  }
        if (digitalRead(cup) == LOW && pahar == 0 ){        //pahar ok dar nu a luat pastilele "te rog ia pastilele"
          cont2++;
        }
            if(cont2 == 1000 && obs == 1){
               msgRogPill(); 
            cont2 = 0;
            mail++;
            }
        if (digitalRead(cup) == HIGH && pahar == 0 && obs == 1){          //a ridicat paharul din aparat
          pahar = 1;  
        }
            if (digitalRead(cup) == LOW && pahar == 1 && obs == 1){       //a pus paharul si "multumesc" si next program
                msgThx();
             cont1 = 0;
             cont2 = 0;
             mail = 0;
             pahar = 0;
             obs = 0;
             addr++;  
            }
EEPROM.write(60, addr);                                                   // salveaza status in memorie
EEPROM.commit();

/*  if(digitalRead(cup) == HIGH && cont1 == 0 && fault == 0){                // nu are pahar in aparat
       cont3++;
       faults = 1; 
  }
     if(cont3 == 1000 && faults == 1){                                     //mesaj "pune pahar in aparat"
          msgNuCup();
      cont3 = 0;
      mail++;
      }
  if(digitalRead(cup) == LOW && faults == 1){                             //a pus paharul in aparat "multumesc"
      msgThx(); 
    cont3 = 0;
    mail = 0;
    pahar = 0;
    obs = 0;
    fault = 0;
    faults = 0;  
  }
  */
if(addr > 4){addr = 1;}                                                                 //reia programul

  if(digitalRead(pinOk) == LOW && stare == 0) {                                         //load cartus
        digitalWrite(viteza, HIGH);
        stare = digitalRead(box);
        delay(10); 
  }
            load = 1;
            digitalWrite(viteza, LOW);
            delay(10);
}

void sendCallback(SendStatus msg) {
      Serial.println(msg.info());
          if (msg.success()) {
                Serial.println("----------------");
          }
}
