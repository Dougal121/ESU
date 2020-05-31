/*
    This is the SCADA for my first ESU built at Farm 121

    The code is designed to work with/in multiple configurations and hardware based around the ESP8266.

    It was fabricated at code and electrical works of Plummer Software in Coomealla, Australia circa 2019
    Newer copies can be found at https://github.com/Dougal121/ESU_SCADA

    The idea is to provide experience and a bridging step between grid connected and off grid systems.

    Compiles for LOLIN D32 @ 80Mhz
*/
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiUDP.h>
#include <TimeLib.h>
#include <Wire.h>
#include <EEPROM.h>
#include <stdio.h>
#include "SSD1306.h"
//#include "SH1106.h"
#include "SH1106Wire.h"
#include "ds3231.h"
#include <Update.h>
#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "StaticPages.h"

#define BUFF_MAX 32
#define MYVER 0x01010111     // change this if you change the structures that hold data that way it will force a "backinthebox" to get safe and sane values from eeprom
#define LineText     0
#define Line        12

const int MAX_EEPROM = 2000 ;
const byte MAX_WIFI_TRIES = 45 ;
const int PROG_BASE = 256 ;   // where the irrigation valve setup and program information starts in eeprom
const int MAX_LOG = 96 ;

//SSD1306 display(0x3c, 5, 4);   // GPIO 5 = D1, GPIO 4 = D2   - onboard display 0.96"
SSD1306 display(0x3c, 21, 22);   // Arduino32 ESP boards   - onboard display 0.96"
//SH1106Wire display(0x3c, 4, 5);   // arse about ??? GPIO 5 = D1, GPIO 4 = D2  -- external ones 1.3"

typedef struct __attribute__((__packed__)) {     // eeprom stuff
  unsigned int localPort = 2390;          // 2 local port to listen for NTP UDP packets
  unsigned int localPortCtrl = 8666;      // 4 local port to listen for Control UDP packets
  unsigned int RemotePortCtrl = 8664;     // 6 local port to listen for Control UDP packets
  long lNodeAddress ;                     // 22
  float fTimeZone ;                       // 26
  IPAddress RCIP ;                        // (192,168,2,255)  30
  char NodeName[16] ;                     // 46
  char nssid[16] ;                        // 62
  char npassword[16] ;                    // 78
  time_t AutoOff_t ;                      // 82     auto off until time > this date
  uint8_t lDisplayOptions  ;              // 83
  uint8_t lNetworkOptions  ;              // 84
  uint8_t lSpare1  ;                      // 85
  uint8_t lSpare2  ;                      // 86
  char timeServer[24] ;                   // 110   = {"au.pool.ntp.org\0"}
  char cpassword[16] ;                    // 126
  long lVersion  ;                        // 130
  IPAddress IPStatic ;                    // (192,168,0,123)
  IPAddress IPGateway ;                   // (192,168,0,1)
  IPAddress IPMask ;                      // (255,255,255,0)
  IPAddress IPDNS ;                       // (192,168,0,15)
  struct ts tc;            //
  float ha ;
  float sunX ;
  float sunrise ;
  float sunset ;
  float latitude;          //
  float longitude;         //
  float tst ;
  float solar_az_deg;      //
  float solar_el_deg;      //
  int iDayNight ;          //
  float decl ;
  float eqtime ;
} general_housekeeping_stuff_t ;          // computer says it's  ??? is my maths crap ????

general_housekeeping_stuff_t ghks ;


typedef struct __attribute__((__packed__)) {             // permanent record
  bool      ActiveTimes[48];    // 48 half hour blocks
  uint16_t  iICtoInverter ;     // time delay in seconds between IC and inverter switching
  float     MinVoltage ;        // min battery pack voltage which shuts off inverter
  float     ResetVoltage ;      // Reset enable when battery pack voltage exceed this
  float     ExtraVoltage;       // whatever i forgot for later ?
  bool      bMasterEnable ;     // like it says on the box
  bool      bSolarEnable ;      // like it says on the box
  uint16_t  iOnTimeSunset ;     // man on minutes countdown
  uint16_t  iOnTimeSunrise ;    // man on minutes countdown
  uint8_t   InverterPIN ;       // Physical Output that its connected to
  uint8_t   IncommerPIN ;       //
  uint8_t   ADC_Voltage ;       //
  uint8_t   ADC_CurrentIn ;     //
  uint8_t   ADC_CurrentOut ;    //
  uint8_t   ADC_Extra ;         //
  float     ADC_Cal_Voltage ;
  float     ADC_Cal_CurrentIn ;     //
  float     ADC_Cal_CurrentOut ;    //
  float     ADC_Cal_Extra ;         //
  float     ADC_Cal_Ofs_Voltage ;
  float     ADC_Cal_Ofs_CurrentIn ;     //
  float     ADC_Cal_Ofs_CurrentOut ;    //
  float     ADC_Cal_Ofs_Extra ;         //
} control_t ;

typedef struct __attribute__((__packed__)) {             // 16 bytes per record
  time_t  RecTime ;           //    4
  float   Voltage ;           //    8   Pack volatge
  float   Current ;           //   12   Pack Current
  float   AmpHours ;          //   16   AH Balance
} log_rec_t ;

typedef struct __attribute__((__packed__)) {             // tempary record
  uint16_t  iOffTime ;      // man off minutes countdown
  uint16_t  iOnTime ;       // man on minutes countdown
  bool      bOnOffState ;
  bool      bVoltageEnable ;
  bool      bInverter  ;
  bool      bIncomer ;
  time_t    tOn ;
  time_t    tOff ;
  float     fTemp[4] ;
} internal_t ;

internal_t          esui ;      // internal volitoile states
control_t           esuc ;      // control data
log_rec_t           esul[MAX_LOG] ;  // 1536 bytes ??  last 24 hours at one sample per 15 min
uint16_t            current_ADC[4] ;

char cssid[32] = {"Configure_XXXXXXXX\0"} ;
char *host = "Control_00000000\0";                // overwrite these later with correct chip ID
char Toleo[10] = {"Ver 2.5\0"}  ;
char dayarray[8] = {'S', 'M', 'T', 'W', 'T', 'F', 'S', 'E'} ;

char buff[BUFF_MAX];

IPAddress MyIP ;
IPAddress MyIPC  ;

const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   //buffer to hold incoming and outgoing packets

byte rtc_sec ;
byte rtc_min ;
byte rtc_hour ;
byte rtc_fert_hour ;
float rtc_temp ;
long lScanCtr = 0 ;
long lScanLast = 0 ;
bool bConfig = false ;
bool hasRTC = false ;
bool hasSD = false;
long lRebootCode = 0 ;
uint8_t rtc_status ;
struct ts tc;
time_t tGoodTime ;   // try and remember the time if you root ;
unsigned long lTimeNext = 0 ;     // next network retry
int bSaveReq = 0 ;
int iUploadPos = 0 ;
bool bDoTimeUpdate = true ;
long  MyCheckSum ;
long  MyTestSum ;
long lTimePrev ;
long lTimePrev2 ;
long lLogIndex ; // which sample we is current one
long under_volt_ctr = 0 ;
uint64_t chipid;
long lMinUpTime = 0 ;
long lsst = 0 ;
long lsrt = 0 ;
float fVD[1440] ;   // volatge data once per minute
bool bPrevConnectionStatus = false;
bool bManSet = true ;
WiFiUDP ntpudp;

WebServer server(80);
//WebServer OTAWebServer(81);



void setup() {
  int i , k , j = 0;
  lMinUpTime = 0 ;
  esui.iOffTime = 0 ;
  esui.iOnTime = 0 ;
  esui.bOnOffState = false ;
  esui.bInverter = false  ;
  esui.bIncomer = false ;
  esui.tOn = 0 ;
  esui.tOff = 0 ;

  chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  lRebootCode = random(1, +2147483640) ; // want to change it straight away

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("");          // new line after the startup burst

  pinMode(2, OUTPUT);
  digitalWrite(2, !digitalRead(2)); // Built In LED
  digitalWrite(2, !digitalRead(2)); // Built In LED
  //  pinMode(BUILTIN_LED,OUTPUT);  //  D4 builtin LED
  //  pinMode(SETPMODE_PIN,INPUT_PULLUP);
  /*
    pinMode(15,OUTPUT);  //
    pinMode(33,OUTPUT);  //
    pinMode(32,OUTPUT);  //

    pinMode(18,OUTPUT);  // D14
    pinMode(19,OUTPUT);  // D13
    pinMode(23,OUTPUT);  // D12
    pinMode(5,OUTPUT);   // D11
    pinMode(13,OUTPUT);  // D10
    pinMode(12,OUTPUT);  // D9
  */
  pinMode(14, OUTPUT); // D8      setup 4 the outputs for relay shield
  pinMode(27, OUTPUT); // D7
  pinMode(16, OUTPUT); // D6
  pinMode(17, OUTPUT); // D5
  //  pinMode(25,OUTPUT);  // D4
  //  pinMode(26,OUTPUT);  // D3

  EEPROM.begin(MAX_EEPROM);
  LoadParamsFromEEPROM(true);

  display.init();
  if (( ghks.lDisplayOptions & 0x01 ) != 0 ) {  // if bit one on then flip the display
    display.flipScreenVertically();
  }


  display.clear();   //    show start screen
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(63, 0, "ESU");
  display.drawString(63, 16, "SCADA");
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 40, "Copyright (c) 2020");
  display.drawString(0, 50, "Dougal Plummer");
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(127, 50, String(Toleo));
  display.display();

  display.setFont(ArialMT_Plain_10);

  if ( MYVER != ghks.lVersion ) {
    //  if ( false ) {
    Serial.println("Loading memory defaults...");
    BackInTheBoxMemory();         // load defaults if blank memory detected but dont save user can still restore from eeprom
    Serial.println("Loaded memory defaults...");
    delay(2000);
  }

  WiFi.disconnect();
  Serial.println("Configuring soft access point...");
  WiFi.mode(WIFI_AP_STA);  // we are having our cake and eating it eee har
  sprintf(cssid, "Configure_%08X\0", chipid);
  if ( cssid[0] == 0 || cssid[1] == 0 ) {  // pick a default setup ssid if none
    sprintf(ghks.cpassword, "\0");
  }
  MyIPC = IPAddress (192, 168, 5 + ((uint8_t)chipid & 0x7f ) , 1);
  WiFi.softAPConfig(MyIPC, MyIPC, IPAddress (255, 255, 255 , 0));
  Serial.println("Starting access point...");
  Serial.print("SSID: ");
  Serial.println(cssid);
  Serial.print("Password: >");
  Serial.print(ghks.cpassword);
  Serial.println("< " + String(ghks.cpassword[0]));
  if (( ghks.cpassword[0] == 0 ) || ( ghks.cpassword[0] == 0xff)) {
    WiFi.softAP((char*)cssid);                   // no passowrd
  } else {
    WiFi.softAP((char*)cssid, (char*) ghks.cpassword);
  }
  MyIPC = WiFi.softAPIP();  // get back the address to verify what happened
  Serial.print("Soft AP IP address: ");
  snprintf(buff, BUFF_MAX, ">> IP %03u.%03u.%03u.%03u <<", MyIPC[0], MyIPC[1], MyIPC[2], MyIPC[3]);
  Serial.println(buff);

  bConfig = false ;   // are we in factory configuratin mode
  display.display();
  if ( ghks.lNetworkOptions != 0 ) {
    WiFi.config(ghks.IPStatic, ghks.IPGateway, ghks.IPMask, ghks.IPDNS );
  }
  if ( ghks.npassword[0] == 0 ) {
    WiFi.begin((char*)ghks.nssid);                    // connect to unencrypted access point
  } else {
    WiFi.begin((char*)ghks.nssid, (char*)ghks.npassword);  // connect to access point with encryption
  }
  while (( WiFi.status() != WL_CONNECTED ) && ( j < MAX_WIFI_TRIES )) {
    j = j + 1 ;
    delay(500);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "Chip ID " + String((uint32_t)chipid, HEX) );
    display.drawString(0, 9, String("SSID:") );
    display.drawString(0, 18, String("Password:") );
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128 , 0, String(WiFi.RSSI()));
    display.drawString(128, 9, String(ghks.nssid) );
    display.drawString(128, 18, String(ghks.npassword) );
    display.drawString(j * 4, 27 , String(">") );
    display.drawString(0, 36 , String(1.0 * j / 2) + String(" (s)" ));
    snprintf(buff, BUFF_MAX, ">>  IP %03u.%03u.%03u.%03u <<", MyIPC[0], MyIPC[1], MyIPC[2], MyIPC[3]);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(63 , 54 ,  String(buff) );
    display.display();
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  }
  if ( j >= MAX_WIFI_TRIES ) {
    bConfig = true ;
    WiFi.disconnect();
  } else {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    MyIP =  WiFi.localIP() ;
    snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
    Serial.println(buff);
    display.drawString(0 , 53 ,  String(buff) );
    display.display();
  }
  if (ghks.localPortCtrl == ghks.localPort ) {            // bump the NTP port up if they ar the same
    ghks.localPort++ ;
  }
  /*  ntpudp.begin(ghks.localPort);                      // this is the recieve on NTP port
    display.drawString(0, 44, "NTP UDP " );
    display.display();
    ctrludp.begin(ghks.localPortCtrl);                 // recieve on the control port
    display.drawString(64, 44, "CTRL UDP " );
    display.display();
                                                  // end of the normal setup

    sprintf(host,"Control_%08X\0",chipid);
    if (MDNS.begin(host)) {
      MDNS.addService("http", "tcp", 80);
    }
  */
  server.on("/", handleRoot);
  server.on("/setup", handleRoot);
  server.on("/vsss", handleRoot);
  server.on("/scan", i2cScan);
  server.on("/stime", handleRoot);
  server.on("/info", handleInfo);
  server.on("/log1", datalog1_page);
  server.on("/log2", datalog2_page);
  server.on("/chart1", chart1_page);
  server.on("/chart2", chart2_page);
  server.on("/eeprom", DisplayEEPROM);
  server.on("/backup", HTTP_GET , handleBackup);
  server.on("/backup.txt", HTTP_GET , handleBackup);
  server.on("/backup.txt", HTTP_POST,  handleRoot, handleFileUpload);
  server.on("/list", HTTP_GET, indexSDCard);
  server.on("/download", HTTP_GET, downloadFile);
  server.on("/chartfile", HTTP_GET, chartFile);
  
  server.on("/login", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
    Serial.printf("Display Login Page");
  });
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", updatePage);
  });
  server.on("/update", HTTP_POST, []() {   //handling uploading firmware file
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {

      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {   // flashing firmware to ESP
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.onNotFound(handleNotFound);
  server.begin();
  if ( year(now()) < 2020 ){
    if( year(tGoodTime) > 2019 ){
      setTime(tGoodTime) ;  // try and recuse a reasonable time stamp from memory just in case the NTP or network fails
    }
  }
  tc.mon = 0 ;
  tc.wday = 0 ;
  DS3231_init(DS3231_INTCN); // look for a rtc
  DS3231_get(&tc);
  rtc_status = DS3231_get_sreg();
  if (((tc.mon < 1 ) || (tc.mon > 12 )) && ((tc.wday > 8)||(tc.wday < 1))) { // no rtc to load off
    Serial.println("NO RTC ?");
  } else {
    setTime((int)tc.hour, (int)tc.min, (int)tc.sec, (int)tc.mday, (int)tc.mon, (int)tc.year ) ; // set the internal RTC
    hasRTC = true ;
    Serial.println("Has RTC ?");
    rtc_temp = DS3231_get_treg();
  }
  rtc_min = minute();
  rtc_sec = second();

  if (SD.begin(SS)) {
    Serial.println("SD Card initialized.");
    hasSD = true;
  }else{
    Serial.println("SD Card failed on startup.");
    hasSD = false;
  }
  randomSeed(now());                       // now we prolly have a good time setting use this to roll the dice for reboot code
  lRebootCode = random(1, +2147483640) ;
}


void loop() {
  long lTime ;
  long lRet ;
  int i , j , k  ;
  int x , y ;
  int board ;
  uint8_t OnPol ;
  uint8_t OffPol ;
  uint8_t OnPulse ;
  uint8_t OffPulse ;
  bool bSendCtrlPacket = false ;
  bool bDirty = false ;
  bool bDirty2 = false ;
  long lcct ;
  float fVoltage ;
  long lTD ;
  
  server.handleClient();
  //  OTAWebServer.handleClient();
  
  lTime = millis() ;
  if (esui.bOnOffState && (lTime > lTimePrev2)) {
    digitalWrite(2, !digitalRead(2)); // Built In LED
    lTimePrev2 = millis()+251;        // flashes extra one above
  }
  lScanCtr++ ;
  bSendCtrlPacket = false ;
  if ( rtc_sec != second()) {
    if( year(now()) > 2019 ){
      tGoodTime = now();    // update this if it looks remotely right
    }
    digitalWrite(2, !digitalRead(2)); // Built In LED
    lTimePrev2 = millis()+251;        // flashes extra one above
    switch (rtc_sec % 4) {
      case 0:
        current_ADC[0] = analogRead(esuc.ADC_Voltage) ;        // 3
        break;
      case 1:
        current_ADC[1] = analogRead(esuc.ADC_CurrentIn) ;      // 4
        break;
      case 2:
        current_ADC[2] = analogRead(esuc.ADC_CurrentOut) ;     // 5
        break;
      case 3:
        current_ADC[3] = analogRead(esuc.ADC_Extra) ;         // 6
        break;
    }
    display.clear();
    //      display.drawLine(minRow, 63, maxRow, 63);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(), month(), day() , hour(), minute(), second());
    display.drawString(0 , 0, String(buff) );
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(127 , 0, String(WiFi.RSSI()));
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    switch (rtc_sec & 0x03) {
      case 1:
        snprintf(buff, BUFF_MAX, "IP %03u.%03u.%03u.%03u", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
        break;
      case 2:
        snprintf(buff, BUFF_MAX, ">>  IP %03u.%03u.%03u.%03u <<", MyIPC[0], MyIPC[1], MyIPC[2], MyIPC[3]);
        break;
      default:
        snprintf(buff, BUFF_MAX, "%s", cssid );
        break;
    }
    display.drawString(64 , 53 ,  String(buff) );

    display.setColor(INVERSE);
    if ((!esui.bVoltageEnable)&& (((rtc_sec % 2 ) == 1 ))){
      display.fillRect(0, 11, 128, 11);
    }
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 10 , String("DCV  " ) + String((current_ADC[0]*esuc.ADC_Cal_Voltage / 4096) + esuc.ADC_Cal_Ofs_Voltage));
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(74 , 10, String(esuc.MinVoltage ));
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(127 , 10, String(esuc.ResetVoltage ));

    /*    for ( i = 0 ; i < 4 ; i++){
          display.setTextAlignment(TEXT_ALIGN_LEFT);
          display.drawString(0, 10+(10*i) , String("ADC " )+String(i));
          display.setTextAlignment(TEXT_ALIGN_RIGHT);
          display.drawString(127 , 10+(10*i), String(current_ADC[0]));
        }
    */
    if ( esui.bIncomer ) {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      //      display.drawRect(0,22,128,11);
      if (( under_volt_ctr > 45 ) && (((rtc_sec % 2 ) == 1 ))){
        display.fillRect(0, 22, 128, 11);
      }
      display.drawString(64 , 21, String("<<< INCOMER ON >>>"));
    }
    if ( esui.bInverter ) {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      //      display.drawRect(0,33,128,11);
      if (( under_volt_ctr > 20 ) && (((rtc_sec % 2 ) == 0 ))){
        display.fillRect(0, 34, 128, 11);
      }
      display.drawString(64 , 33, String("<<< INVERTER ON >>>"));
    }
    display.setColor(WHITE);

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0 , 44, String(esui.iOnTime));
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(127 , 44, String(esui.iOffTime));
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    snprintf(buff, BUFF_MAX, "%d:%02d:%02d",(lMinUpTime/1440),((lMinUpTime/60)%24),(lMinUpTime%60));    
    display.drawString(63 , 44, String(buff));
    display.display();

    fVoltage = ((current_ADC[0] * esuc.ADC_Cal_Voltage / 4096) + esuc.ADC_Cal_Ofs_Voltage) ;
    if ( fVoltage < esuc.MinVoltage ) {   // low voltage switch off and hold off
      under_volt_ctr ++ ;
      if (under_volt_ctr > 60) {          // need 60 seconds below set voltage
        esui.bVoltageEnable = false ;
      }
    } else {
      under_volt_ctr = 0 ;
    }
    if ( fVoltage > esuc.ResetVoltage ) {   // low voltage switch off and hold off
      esui.bVoltageEnable = true ;
    }
    if ( !esui.bVoltageEnable ) {
      esui.bOnOffState = false ;
    }

    if ( esui.bOnOffState ) {  // Inverter and Incommer sequencer
      if ( esui.bIncomer ) {
        if (( !esui.bInverter ) && ( esui.tOn <= now())) { // turn on incomer
          esui.bInverter = true ;
        }
      } else {
        esui.bIncomer = true ;
        esui.tOn = now() + esuc.iICtoInverter ;
      }
    } else {  // offstate
      if (esui.bInverter) {
        esui.bInverter = false ;
        esui.tOff = now() + esuc.iICtoInverter ;
      } else { // inverter off
        if ( esui.bIncomer && ( esui.tOff <= now())) { // turn on incomer
          esui.bIncomer = false ;
        }
      }
    }

    digitalWrite(esuc.IncommerPIN, esui.bIncomer);
    digitalWrite(esuc.InverterPIN, esui.bInverter);

    esui.bOnOffState = false ;                                                     // default is off
    if ((esuc.bMasterEnable) && (ghks.AutoOff_t <= now())&& ( year() > 2019 )&& (esui.bVoltageEnable )) {   // scan the hour blocks
      for ( i = 0 ; i < 48 ; i++) {  // scan all the on times and turn on if
        if (esuc.ActiveTimes[i] ) {
          if (( i / 2) == hour()) {
            if (( i % 2 ) == 0 ) {   // tophalf of hour
              if ( minute() < 30 ) {
                esui.bOnOffState = true ;
              }
            } else {
              if ( minute() > 29 ) {
                esui.bOnOffState = true ;
              }
            }
          }
        }
      }
    }
    lsst = ( HrsSolarTime(ghks.sunset) * 100 ) + MinSolarTime(ghks.sunset) ;
    lsrt = ( HrsSolarTime(ghks.sunrise) * 100 ) + MinSolarTime(ghks.sunrise) ;

    lcct = ( hour() * 100 ) + minute()  ;
    if ((ghks.AutoOff_t <= now()) && ( year() > 2019 )&& ( esuc.bSolarEnable ) && (esui.bVoltageEnable )){
      if ( lcct == lsst ) {
        esui.iOnTime = esuc.iOnTimeSunset ;
      }
      if ( lcct == ( lsrt - esuc.iOnTimeSunrise ))  {
        esui.iOnTime =  esuc.iOnTimeSunrise ;
      }
    }

    if (( esui.iOnTime > 0 ) && (esui.bVoltageEnable )) { // hold on manual
      esui.bOnOffState = true ;
    }
    if ( esui.iOffTime > 0  ) { // hold off
      esui.bOnOffState = false ;
    }

    rtc_sec = second() ;
    lScanLast = lScanCtr ;
    lScanCtr = 0 ;

    if (((minute() % 15) == 0 ) &&  (second() == 0 )) { // data logging
      i = (hour() * 4) +  ( minute() / 15 ) ;
      esul[i].RecTime = now() ;
      esul[i].Voltage = fVoltage ;
      esul[i].Current = 0 ;
      esul[i].AmpHours = 0 ;
    }

  }                         // #########   end of the once per second stuff  ################

  if (rtc_hour != hour()) {
    bSendCtrlPacket = true ;
    if ( !bConfig ) { // ie we have a network
      if ( hour() == 12 ) {
        sendNTPpacket(ghks.timeServer); // send an NTP packet to a time server  once and hour
        bDoTimeUpdate = false ;
      }
    } else {
      if ( hasRTC ) {
        DS3231_get(&tc);
        setTime((int)tc.hour, (int)tc.min, (int)tc.sec, (int)tc.mday, (int)tc.mon, (int)tc.year ) ; // set the internal RTC
      }
    }
    rtc_hour = hour();
  }
  if ( rtc_min != minute()) {
    lMinUpTime++ ;
    fVD[(hour() * 60) + minute()] = fVoltage ;
    if ((bDoTimeUpdate)) {  // not the correct time try to fix every minute
      sendNTPpacket(ghks.timeServer); // send an NTP packet to a time server
      bDoTimeUpdate = false ;
    }
    if ( hasRTC ) {
      rtc_temp = DS3231_get_treg();
    }

    if ( esui.iOnTime > 0 ) { // hold on
      esui.iOnTime-- ;
    }
    if ( esui.iOffTime > 0 ) { // hold off
      esui.iOffTime-- ;
    }

    ghks.tc.year = year();   // get the time into the structure
    ghks.tc.mon = month() ;
    ghks.tc.mday = day();
    ghks.tc.hour = hour();
    ghks.tc.min = minute();
    ghks.tc.sec = second();

    ghks.solar_az_deg = SolarAzimouthRad(ghks.longitude, ghks.latitude, &ghks.tc, ghks.fTimeZone) * 180 / PI ;
    ghks.solar_el_deg = SolarElevationRad(ghks.longitude, ghks.latitude, &ghks.tc, ghks.fTimeZone) * 180 / PI ;

    ghks.decl = Decl(gama(&ghks.tc)) * 180 / PI ;
    ghks.eqtime = eqTime(gama(&ghks.tc)) ;
    ghks.ha = HourAngle (ghks.longitude , &ghks.tc , ghks.fTimeZone )  ;
    ghks.sunrise = Sunrise(ghks.longitude, ghks.latitude, &ghks.tc, ghks.fTimeZone) ;
    ghks.sunset = Sunset(ghks.longitude, ghks.latitude, &ghks.tc, ghks.fTimeZone);
    ghks.tst = TrueSolarTime(ghks.longitude, &ghks.tc, ghks.fTimeZone);
    ghks.sunX = abs(ghks.latitude) + ghks.decl ;
    if (ghks.solar_el_deg >= 0 ) {          // day
      ghks.iDayNight = 1 ;
    } else {                                // night
      ghks.iDayNight = 0 ;
    }
    if ( year() >= 2020 )  // don't data log if year/time is crap
      DataLog("");  // data log to SD card if present ( collect a days worth at a time )
    rtc_min = minute() ;
  }
  if (second() > 4 ) {
    if ( ntpudp.parsePacket() ) {
      processNTPpacket();
    }
  }


  if (bSendCtrlPacket) {
    //    sendCTRLpacket(ghks.RCIP) ;
  }
  if (lTimePrev > ( lTime + 100000 )) { // Housekeeping --- has wrapped around so back to zero
    lTimePrev = lTime ; // skip a bit
    //    Serial.println("Wrap around");
  }

  //  digitalWrite(SCOPE_PIN,!digitalRead(SCOPE_PIN));  // my scope says we are doing this loop at an unreasonable speed except when we do web stuff

  snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(), month(), day() , hour(), minute(), second());
  if ( !bPrevConnectionStatus && WiFi.isConnected() ){
    Serial.println(String(buff )+ " WiFi Reconnected OK...");  
    MyIP =  WiFi.localIP() ;
  }
  if (!WiFi.isConnected())  {
    lTD = (long)lTimeNext-(long) millis() ;
    if (( abs(lTD)>40000)||(bPrevConnectionStatus)){ // trying to get roll over protection and a 30 second retry
      lTimeNext = millis() - 1 ;
/*      Serial.print(millis());
      Serial.print(" ");
      Serial.print(lTimeNext);
      Serial.print(" ");
      Serial.println(abs(lTD));*/
    }
    bPrevConnectionStatus = false;
    if ( lTimeNext < millis() ){
      Serial.println(String(buff )+ " Trying to reconnect WiFi ");
      WiFi.disconnect(false);
//      Serial.println("Connecting to WiFi...");
      WiFi.mode(WIFI_AP_STA);
      if ( ghks.lNetworkOptions != 0 ) {            // use ixed IP
        WiFi.config(ghks.IPStatic, ghks.IPGateway, ghks.IPMask, ghks.IPDNS );
      }
      if ( ghks.npassword[0] == 0 ) {
        WiFi.begin((char*)ghks.nssid);                    // connect to unencrypted access point
      } else {
        WiFi.begin((char*)ghks.nssid, (char*)ghks.npassword);  // connect to access point with encryption
      }
      lTimeNext = millis() + 30000 ;
    }
  }else{
    bPrevConnectionStatus = true ;
  }  
}   // ##################### BOTOM OF MAIN LOOP ###########################################


