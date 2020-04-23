void SerialOutParams() {
  String message ;

  message = "Web Request URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  Serial.println(message);
  message = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  Serial.println(message);
}

void SendHTTPHeader() {
  String message ;

  server.sendHeader(F("Server"), F("ESP8266-on-ice"), false);
  server.sendHeader(F("X-Powered-by"), F("Dougal-1.0"), false);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(F("<!DOCTYPE HTML>"));
  server.sendContent("<head><title>ESU - SCADA " + String(Toleo) + "</title>");
  server.sendContent(F("<meta name=viewport content='width=320, auto inital-scale=1'>"));
  server.sendContent(F("</head><body><html><center><h3>"));
  if ( esui.bOnOffState ){
    message = "<font color='green'><b>" ;
  }else{
    message = "<font color='red'><b>" ;    
  }
  server.sendContent("<a title='click for home / refresh' href='/'>" + String(ghks.NodeName) + "</a> " + message + " Battery " +  String(((current_ADC[0]*esuc.ADC_Cal_Voltage/4096)+esuc.ADC_Cal_Ofs_Voltage) ,2)+" (V)</b></font></h3>");
}



void SendHTTPPageFooter() {
  server.sendContent(F("<br><a href='/?command=1'>Load Parameters from EEPROM</a><br><br><a href='/?command=667'>Reset Memory to Factory Default</a><br><a href='/?command=665'>Sync UTP Time</a><br><a href='/stime'>Manual Time Set</a><br><a href='/scan'>I2C Scan</a><br>")) ;
  server.sendContent("<a href='/?reboot=" + String(lRebootCode) + "'>Reboot</a><br>");
  //  server.sendContent(F("<a href='/?command=668'>Save Fert Current QTY</a><br>"));
  server.sendContent(F("<a href='/eeprom'>EEPROM Memory Contents</a><br>"));
  server.sendContent(F("<a href='/setup'>Node Setup</a><br>"));
  server.sendContent(F("<a href='/logs'>Data Logs Page 1</a> . <a href='/log2'>Data Logs Page 2</a><br>"));
  server.sendContent(F("<a href='/info'>Node Infomation</a><br>"));  
  server.sendContent(F("<a href='/vsss'>view volatile memory structures</a><br>"));
  if ((MyIP[0] == 0) && (MyIP[1] == 0) && (MyIP[2] == 0) && (MyIP[3] == 0)) {
    snprintf(buff, BUFF_MAX, "%u.%u.%u.%u", MyIPC[0], MyIPC[1], MyIPC[2], MyIPC[3]);
  } else {
    snprintf(buff, BUFF_MAX, "%u.%u.%u.%u", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  }
  server.sendContent("<a href='http://" + String(buff) + "/serverIndex'>OTA Firmware Update</a><br>");
  server.sendContent("<a href='https://github.com/Dougal121/ESU'>Source at GitHub</a><br>");
  server.sendContent("<a href='http://" + String(buff) + "/backup'>Backup / Restore Settings</a><br>");
  server.sendContent(F("</body></html>\r\n"));
}


void handleNotFound() {
  String message = F("Seriously - No way DUDE\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
  //  Serial.print(message);
}


void handleRoot() {
  boolean currentLineIsBlank = true;
  tmElements_t tm;
  long  i = 0 ;
  int ii  ;
  int iProgNum = 0;
  int j ;
  int k , kk , iTmp ;
  boolean bExtraValve = false ;
  uint8_t iPage = 0 ;
  int iAType = 0 ;
  boolean bDefault = true ;
  //  int td[6];
  long lTmp ;
  String MyCheck , MyColor , MyNum , pinname ;
  byte mac[6];
  String message ;

  SerialOutParams();

  for (uint8_t j = 0; j < server.args(); j++) {
    //    bSaveReq = 1 ;
    i = String(server.argName(j)).indexOf("command");
    if (i != -1) { //
      switch (String(server.arg(j)).toInt()) {
        case 1:  // load values
          LoadParamsFromEEPROM(true);
          //          Serial.println("Load from EEPROM");
          break;
        case 2: // Save values
          LoadParamsFromEEPROM(false);
          //          Serial.println("Save to EEPROM");
          break;
        case 3:
          break;
        case 4:
          break;
        case 5:
          break;
        case 8: //  Cold Reboot
          ESP.restart() ;  // ESP.reset();
          break;
        case 9: //  Warm Reboot
          ESP.restart() ;
          break;
        case 42:
          break;
        case 667: // wipe the memory to factory default
          BackInTheBoxMemory();
          break;
        case 665:
          sendNTPpacket(ghks.timeServer); // send an NTP packet to a time server  once and hour
          break;
        case 668:
          break;
      }
    }
    i = String(server.argName(j)).indexOf("reboot");
    if (i != -1) { //
      if (( lRebootCode == String(server.arg(j)).toInt() ) && (lRebootCode > 0 )) { // stop the phone browser being a dick and retry resetting !!!!
        ESP.restart() ;
      }
    }

    i = String(server.argName(j)).indexOf("atoff");
    if (i != -1){  // have a request to request a time update
      tm.Year = (String(server.arg(j)).substring(0,4).toInt()-1970) ;
      tm.Month =(String(server.arg(j)).substring(5,7).toInt()) ;
      tm.Day = (String(server.arg(j)).substring(8,10).toInt()) ;
      tm.Hour =(String(server.arg(j)).substring(11,13).toInt()) ;
      tm.Minute = (String(server.arg(j)).substring(14,16).toInt()) ;
      tm.Second = 0 ;
      ghks.AutoOff_t = makeTime(tm);
    }  

    i = String(server.argName(j)).indexOf("me");
    if (i != -1) { // delay
      if ( String(server.arg(j)).toInt() == 0 ) {
        esuc.bMasterEnable = false ;
      } else {
        esuc.bMasterEnable = true ;
      }
    }

    for ( k = 0 ; k < 4 ; k++) { // adc input pins
      i = String(server.argName(j)).indexOf("a" + String(k));
      if (i != -1) { 
        switch (k){
          case 0:
            esuc.ADC_Voltage = String(server.arg(j)).toInt() ;
          break;
          case 1:
            esuc.ADC_CurrentIn = String(server.arg(j)).toInt() ;
          break;
          case 2:
            esuc.ADC_CurrentOut = String(server.arg(j)).toInt() ;
          break;
          case 3:
            esuc.ADC_Extra = String(server.arg(j)).toInt() ;
          break;
        }
      }
    }
    for ( k = 0 ; k < 4 ; k++) { // adc input pins
      i = String(server.argName(j)).indexOf("b" + String(k));
      if (i != -1) { 
        switch (k){
          case 0:
            esuc.ADC_Cal_Voltage = String(server.arg(j)).toFloat() ;
          break;
          case 1:
            esuc.ADC_Cal_CurrentIn = String(server.arg(j)).toFloat() ;
          break;
          case 2:
            esuc.ADC_Cal_CurrentOut = String(server.arg(j)).toFloat() ;
          break;
          case 3:
            esuc.ADC_Cal_Extra = String(server.arg(j)).toFloat() ;
          break;
        }
      }
    }
    for ( k = 0 ; k < 4 ; k++) { // adc input pins
      i = String(server.argName(j)).indexOf("o" + String(k));
      if (i != -1) { 
        switch (k){
          case 0:
            esuc.ADC_Cal_Ofs_Voltage = String(server.arg(j)).toFloat() ;
          break;
          case 1:
            esuc.ADC_Cal_Ofs_CurrentIn = String(server.arg(j)).toFloat() ;
          break;
          case 2:
            esuc.ADC_Cal_Ofs_CurrentOut = String(server.arg(j)).toFloat() ;
          break;
          case 3:
            esuc.ADC_Cal_Ofs_Extra = String(server.arg(j)).toFloat() ;
          break;
        }
      }
    }
    
    i = String(server.argName(j)).indexOf("mv");
    if (i != -1) { // Min voltage
      esuc.MinVoltage = String(server.arg(j)).toFloat() ;
    }
    i = String(server.argName(j)).indexOf("rv");
    if (i != -1) { // Min voltage
      esuc.ResetVoltage = String(server.arg(j)).toFloat() ;
    }
    i = String(server.argName(j)).indexOf("ev");
    if (i != -1) { // extrra voltage
      esuc.ExtraVoltage = String(server.arg(j)).toFloat() ;
    }
    i = String(server.argName(j)).indexOf("dl");
    if (i != -1) { // delay
      esuc.iICtoInverter = String(server.arg(j)).toInt() ;
    }
    i = String(server.argName(j)).indexOf("iv");
    if (i != -1) { // incomer pin
      esuc.InverterPIN = String(server.arg(j)).toInt() ;
    }
    i = String(server.argName(j)).indexOf("ic");
    if (i != -1) { // inverter pin
      esuc.IncommerPIN = String(server.arg(j)).toInt() ;
    }

    i = String(server.argName(j)).indexOf("et");
    if (i != -1) { // inverter pin
      esui.iOnTime = String(server.arg(j)).toInt() ;
    }
    i = String(server.argName(j)).indexOf("at");
    if (i != -1) { // inverter pin
      esui.iOffTime = String(server.arg(j)).toInt() ;
    }
    i = String(server.argName(j)).indexOf("as");
    if (i != -1) { // sunset
      esuc.iOnTimeSunset = String(server.arg(j)).toInt() ;
    }
    i = String(server.argName(j)).indexOf("bs");
    if (i != -1) { // sunrise
      esuc.iOnTimeSunrise = String(server.arg(j)).toInt() ;
    }



    i = String(server.argName(j)).indexOf("hr"  );
    if (i != -1) { //
      if ( String(server.arg(j)).toInt()  == 0 ) {
        for ( k = 0 ; k < 24 ; k++) { // handle all the valve control commands for any and all valves
          esuc.ActiveTimes[k] = false ;
        }
      } else {
        for ( k = 24 ; k < 48 ; k++) { // handle all the valve control commands for any and all valves
          esuc.ActiveTimes[k] = false ;
        }
      }
    }
    for ( k = 0 ; k < 48 ; k++) { // handle all the valve control commands for any and all valves
      MyNum = String(k) ;
      if ( k < 10 ) {
        MyNum = "0" + MyNum ;
      }
      i = String(server.argName(j)).indexOf("h" + MyNum  );
      if (i != -1) { //
        esuc.ActiveTimes[k] = true ;
      }
    }


    i = String(server.argName(j)).indexOf("stime");
    if (i != -1) { //
      tm.Year = (String(server.arg(j)).substring(0, 4).toInt() - 1970) ;
      tm.Month = (String(server.arg(j)).substring(5, 7).toInt()) ;
      tm.Day = (String(server.arg(j)).substring(8, 10).toInt()) ;
      tm.Hour = (String(server.arg(j)).substring(11, 13).toInt()) ;
      tm.Minute = (String(server.arg(j)).substring(14, 16).toInt()) ;
      tm.Second = 0 ;
      setTime(makeTime(tm));
      if ( hasRTC ) {
        tc.sec = second();
        tc.min = minute();
        tc.hour = hour();
        tc.wday = dayOfWeek(makeTime(tm));
        tc.mday = day();
        tc.mon = month();
        tc.year = year();
        DS3231_set(tc);                       // set the RTC as well
        rtc_status = DS3231_get_sreg();       // get the status
        DS3231_set_sreg(rtc_status & 0x7f ) ; // clear the clock fail bit when you set the time
      }
    }

    i = String(server.argName(j)).indexOf("ndadd");
    if (i != -1) { //
      ghks.lNodeAddress = String(server.arg(j)).toInt() ;
      ghks.lNodeAddress = constrain(ghks.lNodeAddress, 0, 32768);
    }
    i = String(server.argName(j)).indexOf("tzone");
    if (i != -1) { //
      ghks.fTimeZone = String(server.arg(j)).toFloat() ;
      ghks.fTimeZone = constrain(ghks.fTimeZone, -12, 12);
      bDoTimeUpdate = true ; // trigger and update to fix the time
    }
    i = String(server.argName(j)).indexOf("mylat");    //lat  
    if (i != -1){  // have a request to set the latitude
      ghks.latitude = String(server.arg(j)).toFloat() ;
      if (( ghks.latitude < -90) || ( ghks.latitude > 90 )){
        ghks.latitude = -34.051219 ;
      }
    }        
    i = String(server.argName(j)).indexOf("mylon");    // long
    if (i != -1){  // have a request to set the logitude
      ghks.longitude = String(server.arg(j)).toFloat() ;
      if (( ghks.longitude < -180) || ( ghks.longitude > 180 )){
        ghks.longitude = 142.013618 ;
      }
    }        
    
    i = String(server.argName(j)).indexOf("disop");
    if (i != -1) { //
      ghks.lDisplayOptions = String(server.arg(j)).toInt() ;
      ghks.lDisplayOptions = constrain(ghks.lDisplayOptions, 0, 255);
    }
    i = String(server.argName(j)).indexOf("netop");
    if (i != -1) { //
      ghks.lNetworkOptions = String(server.arg(j)).toInt() ;
      ghks.lNetworkOptions = constrain(ghks.lNetworkOptions, 0, 255);
    }


    i = String(server.argName(j)).indexOf("lpntp");
    if (i != -1) { //
      ghks.localPort = String(server.arg(j)).toInt() ;
      ghks.localPort = constrain(ghks.localPort, 1, 65535);
    }
    i = String(server.argName(j)).indexOf("lpctr");
    if (i != -1) { //
      ghks.localPortCtrl = String(server.arg(j)).toInt() ;
      ghks.localPortCtrl = constrain(ghks.localPortCtrl, 1, 65535);
    }
    i = String(server.argName(j)).indexOf("rpctr");
    if (i != -1) { //
      ghks.RemotePortCtrl = String(server.arg(j)).toInt() ;
      ghks.RemotePortCtrl = constrain(ghks.RemotePortCtrl, 1, 65535);
    }
    i = String(server.argName(j)).indexOf("dontp");
    if (i != -1) { // have a request to request a time update
      bDoTimeUpdate = true ;
    }
    i = String(server.argName(j)).indexOf("cname");
    if (i != -1) { // have a request to request a time update
      String(server.arg(j)).toCharArray( ghks.NodeName , sizeof(ghks.NodeName)) ;
    }
    i = String(server.argName(j)).indexOf("rpcip");
    if (i != -1) { // have a request to request an IP address
      ghks.RCIP[0] = String(server.arg(j)).substring(0, 3).toInt() ;
      ghks.RCIP[1] = String(server.arg(j)).substring(4, 7).toInt() ;
      ghks.RCIP[2] = String(server.arg(j)).substring(8, 11).toInt() ;
      ghks.RCIP[3] = String(server.arg(j)).substring(12, 15).toInt() ;
    }
    i = String(server.argName(j)).indexOf("staip");
    if (i != -1) { // have a request to request an IP address
      ghks.IPStatic[0] = String(server.arg(j)).substring(0, 3).toInt() ;
      ghks.IPStatic[1] = String(server.arg(j)).substring(4, 7).toInt() ;
      ghks.IPStatic[2] = String(server.arg(j)).substring(8, 11).toInt() ;
      ghks.IPStatic[3] = String(server.arg(j)).substring(12, 15).toInt() ;
    }
    i = String(server.argName(j)).indexOf("gatip");
    if (i != -1) { // have a request to request an IP address
      ghks.IPGateway[0] = String(server.arg(j)).substring(0, 3).toInt() ;
      ghks.IPGateway[1] = String(server.arg(j)).substring(4, 7).toInt() ;
      ghks.IPGateway[2] = String(server.arg(j)).substring(8, 11).toInt() ;
      ghks.IPGateway[3] = String(server.arg(j)).substring(12, 15).toInt() ;
    }
    i = String(server.argName(j)).indexOf("mskip");
    if (i != -1) { // have a request to request an IP address
      ghks.IPMask[0] = String(server.arg(j)).substring(0, 3).toInt() ;
      ghks.IPMask[1] = String(server.arg(j)).substring(4, 7).toInt() ;
      ghks.IPMask[2] = String(server.arg(j)).substring(8, 11).toInt() ;
      ghks.IPMask[3] = String(server.arg(j)).substring(12, 15).toInt() ;
    }
    i = String(server.argName(j)).indexOf("dnsip");
    if (i != -1) { // have a request to request an IP address
      ghks.IPDNS[0] = String(server.arg(j)).substring(0, 3).toInt() ;
      ghks.IPDNS[1] = String(server.arg(j)).substring(4, 7).toInt() ;
      ghks.IPDNS[2] = String(server.arg(j)).substring(8, 11).toInt() ;
      ghks.IPDNS[3] = String(server.arg(j)).substring(12, 15).toInt() ;
    }

    i = String(server.argName(j)).indexOf("atoff");
    if (i != -1) { // have a request to request a time update
      tm.Year = (String(server.arg(j)).substring(0, 4).toInt() - 1970) ;
      tm.Month = (String(server.arg(j)).substring(5, 7).toInt()) ;
      tm.Day = (String(server.arg(j)).substring(8, 10).toInt()) ;
      tm.Hour = (String(server.arg(j)).substring(11, 13).toInt()) ;
      tm.Minute = (String(server.arg(j)).substring(14, 16).toInt()) ;
      tm.Second = 0 ;
      ghks.AutoOff_t = makeTime(tm);
    }
    i = String(server.argName(j)).indexOf("nssid");
    if (i != -1) {                                   // SSID
      //    Serial.println("SookyLala 1 ") ;
      String(server.arg(j)).toCharArray( ghks.nssid , sizeof(ghks.nssid)) ;
    }

    i = String(server.argName(j)).indexOf("npass");
    if (i != -1) {                                   // Password
      String(server.arg(j)).toCharArray( ghks.npassword , sizeof(ghks.npassword)) ;
    }

    i = String(server.argName(j)).indexOf("cpass");
    if (i != -1) {                                   // Password
      String(server.arg(j)).toCharArray( ghks.cpassword , sizeof(ghks.cpassword)) ;
    }

    i = String(server.argName(j)).indexOf("timsv");
    if (i != -1) {                                   // timesvr
      String(server.arg(j)).toCharArray( ghks.timeServer , sizeof(ghks.timeServer)) ;
    }
  }

  SendHTTPHeader();   //  ################### START OF THE RESPONSE  ######

  if ( bSaveReq != 0 ) {
    server.sendContent(F("<blink>"));
  }
  server.sendContent(F("<a href='/?command=2'>Save Parameters to EEPROM</a><br>")) ;
  if ( bSaveReq != 0 ) {
    server.sendContent(F("</blink><font color='red'><b>Changes Have been made to settings.<br>Make sure you save if you want to keep them</b><br></font><br>")) ;
  }


  snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(), month(), day() , hour(), minute(), second());
  if (ghks.fTimeZone > 0 ) {
    server.sendContent("<b>" + String(buff) + " UTC +" + String(ghks.fTimeZone, 1) ) ;
  } else {
    server.sendContent("<b>" + String(buff) + " UTC " + String(ghks.fTimeZone, 1) ) ;
  }
  if ( year() < 2020 ) {
    server.sendContent(F("<font color=red><b>  --- CLOCK NOT SET ---</b></font>")) ;
  }
  server.sendContent(F("</b><br>")) ;
  if ( ghks.AutoOff_t > now() )  {
    snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(ghks.AutoOff_t), month(ghks.AutoOff_t), day(ghks.AutoOff_t) , hour(ghks.AutoOff_t), minute(ghks.AutoOff_t), second(ghks.AutoOff_t));
    server.sendContent(F("<b><font color=red>Automation OFFLINE Untill ")) ;
    server.sendContent(String(buff)) ;
    server.sendContent(F("</font></b><br>")) ;
  } else {
    if (( now() > ghks.AutoOff_t) &&(  year() > 2019 )) {
      server.sendContent(F("<b><font color=green>Automation ONLINE</font></b><br>")) ;
    } else {
      server.sendContent(F("<b><font color=red>Automation OFFLINE Invalid time</font></b><br>")) ;
    }
  }

  if (String(server.uri()).indexOf("stime") > 0) { // ################   SETUP TIME    #######################################
    bDefault = false ;
    snprintf(buff, BUFF_MAX, "%04d/%02d/%02d %02d:%02d", year(), month(), day() , hour(), minute());
    server.sendContent("<br><br><form method=post action=" + server.uri() + "><br>Set Current Time: <input type='text' name='stime' value='" + String(buff) + "' size=12>");
    server.sendContent(F("<input type='submit' value='SET'><br><br></form>"));
  }


  if (String(server.uri()).indexOf("setup") > 0) { // ################  SETUP OF THE NODE #####################################
    bDefault = false ;
    server.sendContent("<form method=post action=" + server.uri() + "><table border=1 title='Node Settings'>");
    server.sendContent(F("<tr><th>Parameter</th><th>Value</th><th><input type='submit' value='SET'></th></tr>"));

    server.sendContent(F("<tr><td>Controler Name</td><td align=center>")) ;
    server.sendContent("<input type='text' name='cname' value='" + String(ghks.NodeName) + "' maxlength=15 size=12></td><td></td></tr>");

    snprintf(buff, BUFF_MAX, "%04d/%02d/%02d %02d:%02d", year(ghks.AutoOff_t), month(ghks.AutoOff_t), day(ghks.AutoOff_t) , hour(ghks.AutoOff_t), minute(ghks.AutoOff_t));
    if (ghks.AutoOff_t > now()) {
      MyColor =  F("bgcolor=red") ;
    } else {
      MyColor =  "" ;
    }
    server.sendContent("<tr><td " + String(MyColor) + ">Auto Off Until</td><td align=center>") ;
    server.sendContent("<input type='text' name='atoff' value='" + String(buff) + "' size=12></td><td>(yyyy/mm/dd)</td></tr>");

    server.sendContent(F("<tr><td>Node Address</td><td align=center>")) ;
    server.sendContent("<input type='text' name='ndadd' value='" + String(ghks.lNodeAddress) + "' size=12></td><td>" + String(ghks.lNodeAddress & 0xff) + "</td></tr>");

    server.sendContent(F("<tr><td>Time Zone</td><td align=center>")) ;
    server.sendContent("<input type='text' name='tzone' value='" + String(ghks.fTimeZone, 1) + "' size=12></td><td>(Hours)</td></tr>");

    server.sendContent(F("<tr><td>Latitude</td><td align=center><input type='text' name='mylat' value='")) ; 
    server.sendContent(String(ghks.latitude,8));
    server.sendContent(F("' size=12></td><td> +N -S</td></tr>")) ; 

    server.sendContent(F("<tr><td>Longitude</td><td align=center><input type='text' name='mylon' value='")) ; 
    server.sendContent(String(ghks.longitude,8));
    server.sendContent(F("' size=12></td><td></td></tr>")) ; 

    server.sendContent(F("<tr><td>Display Options</td><td align=center>")) ;
    server.sendContent(F("<select name='disop'>")) ;
    if (ghks.lDisplayOptions == 0 ) {
      server.sendContent(F("<option value='0' SELECTED>0 - Normal"));
      server.sendContent(F("<option value='1'>1 - Invert"));
    } else {
      server.sendContent(F("<option value='0'>0 - Normal"));
      server.sendContent(F("<option value='1' SELECTED>1 - Invert"));
    }
    server.sendContent(F("</select></td><td></td></tr>"));


    server.sendContent("<form method=post action=" + server.uri() + "><tr><td></td><td></td><td></td></tr>") ;

    server.sendContent(F("<tr><td>Local UDP Port NTP</td><td align=center>")) ;
    server.sendContent("<input type='text' name='lpntp' value='" + String(ghks.localPort) + "' size=12></td><td><input type='submit' value='SET'></td></tr>");

    server.sendContent(F("<tr><td>Local UDP Port Control</td><td align=center>")) ;
    server.sendContent("<input type='text' name='lpctr' value='" + String(ghks.localPortCtrl) + "' size=12></td><td></td></tr>");

    server.sendContent(F("<tr><td>Remote UDP Port Control</td><td align=center>")) ;
    server.sendContent("<input type='text' name='rpctr' value='" + String(ghks.RemotePortCtrl) + "' size=12></td><td></td></tr>");

    server.sendContent(F("<tr><td>Network SSID</td><td align=center>")) ;
    server.sendContent("<input type='text' name='nssid' value='" + String(ghks.nssid) + "' maxlength=15 size=12></td><td></td></tr>");

    server.sendContent(F("<tr><td>Network Password</td><td align=center>")) ;
    server.sendContent("<input type='text' name='npass' value='" + String(ghks.npassword) + "' maxlength=15 size=12></td><td></td></tr>");

    server.sendContent(F("<tr><td>Configure Password</td><td align=center>")) ;
    server.sendContent("<input type='text' name='cpass' value='" + String(ghks.cpassword) + "' maxlength=15 size=12></td><td></td></tr>");

    server.sendContent(F("<tr><td>Time Server</td><td align=center>")) ;
    server.sendContent("<input type='text' name='timsv' value='" + String(ghks.timeServer) + "' maxlength=23 size=12></td><td></td></tr>");

    snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", ghks.RCIP[0], ghks.RCIP[1], ghks.RCIP[2], ghks.RCIP[3]);
    server.sendContent(F("<tr><td>Remote IP Address Control</td><td align=center>")) ;
    server.sendContent("<input type='text' name='rpcip' value='" + String(buff) + "' maxlength=16 size=12></td><td></td></tr></form>");

    server.sendContent("<form method=post action=" + server.uri() + "><tr><td></td><td></td><td></td></tr>") ;

    server.sendContent(F("<tr><td>Network Options</td><td align=center>")) ;
    server.sendContent(F("<select name='netop'>")) ;
    if (ghks.lNetworkOptions == 0 ) {
      server.sendContent(F("<option value='0' SELECTED>0 - DHCP"));
      server.sendContent(F("<option value='1'>1 - Static"));
    } else {
      server.sendContent(F("<option value='0'>0 - DHCP"));
      server.sendContent(F("<option value='1' SELECTED>1 - Static IP"));
    }
    server.sendContent(F("</select></td><td><input type='submit' value='SET'></td></tr>"));
    snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", ghks.IPStatic[0], ghks.IPStatic[1], ghks.IPStatic[2], ghks.IPStatic[3]);
    server.sendContent(F("<tr><td>Static IP Address</td><td align=center>")) ;
    server.sendContent("<input type='text' name='staip' value='" + String(buff) + "' maxlength=16 size=12></td><td></td></tr>");

    snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", ghks.IPGateway[0], ghks.IPGateway[1], ghks.IPGateway[2], ghks.IPGateway[3]);
    server.sendContent(F("<tr><td>Gateway IP Address</td><td align=center>")) ;
    server.sendContent("<input type='text' name='gatip' value='" + String(buff) + "' maxlength=16 size=12></td><td></td></tr>");

    snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", ghks.IPMask[0], ghks.IPMask[1], ghks.IPMask[2], ghks.IPMask[3]);
    server.sendContent(F("<tr><td>IP Mask</td><td align=center>")) ;
    server.sendContent("<input type='text' name='mskip' value='" + String(buff) + "' maxlength=16 size=12></td><td></td></tr>");

    snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", ghks.IPDNS[0], ghks.IPDNS[1], ghks.IPDNS[2], ghks.IPDNS[3]);
    server.sendContent(F("<tr><td>DNS IP Address</td><td align=center>")) ;
    server.sendContent("<input type='text' name='dnsip' value='" + String(buff) + "' maxlength=16 size=12></td><td></td></tr>");

    server.sendContent("<tr><td>Last Scan Speed</td><td align=center>" + String(lScanLast) + "</td><td>(per second)</td></tr>" ) ;
    if ( hasRTC ) {
      rtc_status = DS3231_get_sreg();
      if (( rtc_status & 0x80 ) != 0 ) {
        server.sendContent(F("<tr><td>RTC Battery</td><td align=center bgcolor='red'>DEPLETED</td><td></td></tr>")) ;
      } else {
        server.sendContent(F("<tr><td>RTC Battery</td><td align=center bgcolor='green'>-- OK --</td><td></td></tr>")) ;
      }
      server.sendContent("<tr><td>RTC Temperature</td><td align=center>" + String(rtc_temp, 1) + "</td><td>(C)</td></tr>") ;
    }
    server.sendContent(F("</form></table>"));
  }


  if (String(server.uri()).indexOf("vsss") > 0) { // volitile status - all structures status
    bDefault = false ;
    server.sendContent(F("<br><b>Internal Variables</b><br>"));
    server.sendContent(F("<table border=1 title='Internal Memory Values'>"));
    server.sendContent(F("<tr><th><b>Parameter</td><td align=center><b>Value</b></td></tr>")) ;
    server.sendContent("<tr><td>Off Time</td><td align=center>" + String(esui.iOffTime) + "</td></tr>" ) ;
    server.sendContent("<tr><td>On Time</td><td align=center>" + String(esui.iOnTime) + "</td></tr>" ) ;
    server.sendContent("<tr><td>On Off State</td><td align=center>" + String(esui.bOnOffState) + "</td></tr>" ) ;
    server.sendContent("<tr><td>Inverter Output</td><td align=center>" + String(esui.bInverter) + "</td></tr>" ) ;
    server.sendContent("<tr><td>Incommer Output</td><td align=center>" + String(esui.bIncomer) + "</td></tr>" ) ;
    server.sendContent("<tr><td>On Time Register</td><td align=center>" + String(esui.tOn) + "</td></tr>" ) ;
    server.sendContent("<tr><td>Off Time Register</td><td align=center>" + String(esui.tOff) + "</td></tr>" ) ;
    server.sendContent(F("</table>"));
  }

  if (bDefault) {     // #####################################   default valve control and setup  ##############################################
    server.sendContent(F("<br><b>Battery and Inverter Control</b>"));

    message = F("<table border=1 title='Inverter Control'>");
    message += F("<tr><th></th><th colspan=12>Active Hours</th><th>.</th></tr>");
    server.sendContent(message) ; // End of Table Header

    for ( j = 0 ; j < 2 ; j++ ) {
      message = "<form method=post action='" + server.uri() + "'>" ;
      message += "<tr><th><input type='hidden' name='hr' value='" + String(j) + "'>.</th>";
      for ( i = 0 ; i < 12 ; i++ ) {
        k = i + (j * 12 ) ;
        if ( k < 10 ) {
          message += "<th>0" + String(k) + "</th>" ;
        } else {
          message += "<th>" + String(k) + "</th>" ;
        }
      }
      message += F("<th><input type='submit' value='SET'>.</th></tr>");
      server.sendContent(message) ; // End of Table Header
      message = F("<tr><td>00</td>") ;
      for ( i = (j * 24) ; i < (24 + (j * 24)) ; i += 2 ) {
        MyNum = String(i) ;
        if ( i < 10 ) {
          MyNum = "0" + MyNum ;
        }
        if ( esuc.ActiveTimes[i] ) {
          MyCheck = F("CHECKED") ;
          MyColor =  F("bgcolor=green") ;  // check if no start times
        } else {
          MyCheck = F("") ;
          MyColor = F("") ;
        }
        message += "<td " + String(MyColor) + "><input type='checkbox' name='h" + MyNum + "' " + String(MyCheck) + "></td>" ;
      }
      message += F("</td><td></tr>");
      server.sendContent(message) ;

      message = F("<tr><td>30</td>");
      for ( i = 1 + (j * 24) ; i < ( 24 + (j * 24)) ; i += 2 ) {
        MyNum = String(i) ;
        if ( i < 10 ) {
          MyNum = "0" + MyNum ;
        }
        if ( esuc.ActiveTimes[i] ) {
          MyCheck = F("CHECKED") ;
          MyColor =  F("bgcolor=green") ;  // check if no start times
        } else {
          MyCheck = F("") ;
          MyColor = F("") ;
        }
        message += "<td " + String(MyColor) + "><input type='checkbox' name='h" + MyNum + "' " + String(MyCheck) + "></td>" ;
      }
      message += F("</td><td></tr></form>");
      server.sendContent(message) ; // End of Table Header
    }
    server.sendContent(F("</table><br><br><table border=1 title='Inverter Control'>"));
    server.sendContent("<form method=post action='" + server.uri() + "'>");

    server.sendContent("<tr><th>Parameter</td><td align=center><b>Value</td><td><input type='submit' value='SET'></td></tr>" ) ;

    snprintf(buff, BUFF_MAX, "%04d/%02d/%02d %02d:%02d", year(ghks.AutoOff_t), month(ghks.AutoOff_t), day(ghks.AutoOff_t) , hour(ghks.AutoOff_t), minute(ghks.AutoOff_t));
    if (ghks.AutoOff_t > now()){
      MyColor =  F("bgcolor=red") ;
    }else{
      MyColor =  "" ;
    }
    server.sendContent("<tr><td "+String(MyColor)+">Auto Off Until</td><td align=center "+String(MyColor)+">") ; 
    server.sendContent("<input type='text' name='atoff' value='"+ String(buff) + "' size=12></td><td>(yyyy/mm/dd)</td></tr>");
    if ( !esui.bVoltageEnable ){
      MyColor = F("bgcolor=red") ;  // check if no start times
    }else{
      MyColor = F("") ;  // check if no start times      
    }
    server.sendContent("<tr><td>Minimum Voltage</td><td align=center><input type='text' name='mv' value='" + String(esuc.MinVoltage) + "'></td><td "+String(MyColor)+">(V)</td></tr>" ) ;
    server.sendContent("<tr><td>Reset Voltage</td><td align=center><input type='text' name='rv' value='" + String(esuc.ResetVoltage) + "'></td><td "+String(MyColor)+">(V)</td></tr>" ) ;
    server.sendContent("<tr><td>IC to Start Delay</td><td align=center><input type='text' name='dl' value='" + String(esuc.iICtoInverter) + "'></td><td>(s)</td></tr>" ) ;
    
    if ( !esuc.bMasterEnable ){     // MASTER ENABLE 
      MyColor = F("bgcolor=red") ;  // check if no start times
    }else{
      MyColor = F("") ;  // check if no start times      
    }
    server.sendContent("<tr><td>Master Enable</td><td " + String(MyColor) + " align=center><select name='me'>");
    if ( !esuc.bMasterEnable ){
      server.sendContent(F("<option value='0' SELECTED>0 - Disabled")); 
      server.sendContent(F("<option value='1'>1 - Enabled")); 
    }else{
      server.sendContent(F("<option value='0'>0 - Disabled")); 
      server.sendContent(F("<option value='1' SELECTED>1 - Enabled")); 
    }
    server.sendContent(F("</select></td><td>.</td></tr>")) ;
    server.sendContent("<tr><td>Manual On Timer</td><td align=center><input type='text' name='et' value='" + String(esui.iOnTime) + "'></td><td>(min)</td></tr>" ) ;
    server.sendContent("<tr><td>Manual Off Timer</td><td align=center><input type='text' name='at' value='" + String(esui.iOffTime) + "'></td><td>(min)</td></tr>" ) ;

    server.sendContent(F("<tr><td>Sunrise - State - Sunset</td><td align=center>"));
    snprintf(buff, BUFF_MAX, "%02d:%02d", HrsSolarTime(ghks.sunrise), MinSolarTime(ghks.sunrise));
    server.sendContent(String(buff)) ; 
    if ( ghks.iDayNight == 1 ){
      server.sendContent(F(" - DAY - "));
    }else{
      server.sendContent(F(" - NIGHT - "));          
    }
    snprintf(buff, BUFF_MAX, "%02d:%02d", HrsSolarTime(ghks.sunset), MinSolarTime(ghks.sunset));        
    server.sendContent(String(buff)) ; 
    server.sendContent(F("</td><td>(hh:mm)</td></tr>"));

    if ( !esuc.bSolarEnable ){      // SOLAR ENABLE
      MyColor = F("bgcolor=red") ;  // check if no start times
    }else{
      MyColor = F("") ;  // check if no start times      
    }
    server.sendContent("<tr><td>Solar Start Enable</td><td " + String(MyColor) + " align=center><select name='se'>");
    if ( !esuc.bSolarEnable ){
      server.sendContent(F("<option value='0' SELECTED>0 - Disabled")); 
      server.sendContent(F("<option value='1'>1 - Enabled")); 
    }else{
      server.sendContent(F("<option value='0'>0 - Disabled")); 
      server.sendContent(F("<option value='1' SELECTED>1 - Enabled")); 
    }
    server.sendContent(F("</select></td><td>.</td></tr>")) ;
    server.sendContent("<tr><td>After Sunset On Timer</td><td align=center><input type='text' name='as' value='" + String(esuc.iOnTimeSunset) + "'></td><td>(min)</td></tr>" ) ;
    server.sendContent("<tr><td>Before Sunrise On Timer</td><td align=center><input type='text' name='bs' value='" + String(esuc.iOnTimeSunrise) + "'></td><td>(min)</td></form></tr>" ) ;
    server.sendContent("<tr><td>Computer Uptime</td><td align=center>"+String(lMinUpTime/60) + ":" + String(lMinUpTime%60) + "</td><td>(hr:min)</td></tr>" ) ;

    server.sendContent("</table><br>Physical Setup<table border=1 title='Calibration'><form method=post action='" + server.uri() + "'>") ;
    server.sendContent("<tr><th>Parameter</th><th align=center>Multiplier</th><th align=center>Offset</td><td><input type='submit' value='SET'></td></tr>" ) ;
    server.sendContent("<tr><td>ADC Voltage PIN</td><td><input type='text' name='b0' value='" + String(esuc.ADC_Cal_Voltage) + "'></td><td><input type='text' name='o0' value='" + String(esuc.ADC_Cal_Ofs_Voltage) + "'></td><td><select name='a0'>");
    message = "" ;
    
    for (ii = 34; ii < 40; ii++) {
      if (esuc.ADC_Voltage == ii ){
        MyColor = F(" SELECTED ");
      }else{
        MyColor = "";            
      }
      iTmp = 0 ;
      switch(ii){
        case 35: pinname = F("GPIO 35") ; break;
        case 34: pinname = F("GPIO 34") ; break;
        case 36: pinname = F("GPIO 36") ; break;
        case 39: pinname = F("GPIO 39") ; break;
        default: pinname = F("- UNKNOWN-") ; iTmp = 1 ; break;
      }
      if ( iTmp == 0 ) {
        message += "<option value="+String(ii)+ MyColor +">" + pinname ;          
      }
    }
    message += "</select></td></tr>" ;
    server.sendContent(message);
    server.sendContent("<tr><td>ADC Current In PIN</td><td><input type='text' name='b1' value='" + String(esuc.ADC_Cal_CurrentIn) + "'></td><td><input type='text' name='o1' value='" + String(esuc.ADC_Cal_Ofs_CurrentIn) + "'></td><td><select name='a1'>");
    message = "" ;
    
    for (ii = 34; ii < 40; ii++) {
      if (esuc.ADC_CurrentIn == ii ){
        MyColor = F(" SELECTED ");
      }else{
        MyColor = "";            
      }
      iTmp = 0 ;
      switch(ii){
        case 35: pinname = F("GPIO 35") ; break;
        case 34: pinname = F("GPIO 34") ; break;
        case 36: pinname = F("GPIO 36") ; break;
        case 39: pinname = F("GPIO 39") ; break;
        default: pinname = F("- UNKNOWN-") ; iTmp = 1 ; break;
      }
      if ( iTmp == 0 ) {
        message += "<option value="+String(ii)+ MyColor +">" + pinname ;          
      }
    }
    message += "</select></td></tr>" ;
    server.sendContent(message);
    server.sendContent("<tr><td>ADC Current Out PIN</td><td><input type='text' name='b2' value='" + String(esuc.ADC_Cal_CurrentOut) + "'></td><td><input type='text' name='o2' value='" + String(esuc.ADC_Cal_Ofs_CurrentOut) + "'></td><td><select name='a2'>");
    message = "" ;
    
    for (ii = 34; ii < 40; ii++) {
      if (esuc.ADC_CurrentOut == ii ){
        MyColor = F(" SELECTED ");
      }else{
        MyColor = "";            
      }
      iTmp = 0 ;
      switch(ii){
        case 35: pinname = F("GPIO 35") ; break;
        case 34: pinname = F("GPIO 34") ; break;
        case 36: pinname = F("GPIO 36") ; break;
        case 39: pinname = F("GPIO 39") ; break;
        default: pinname = F("- UNKNOWN-") ; iTmp = 1 ; break;
      }
      if ( iTmp == 0 ) {
        message += "<option value="+String(ii)+ MyColor +">" + pinname ;          
      }
    }
    message += "</select></td></tr>" ;
    server.sendContent(message);
    server.sendContent("<tr><td>ADC Extran PIN</td><td><input type='text' name='b3' value='" + String(esuc.ADC_Cal_Extra) + "'></td><td><input type='text' name='o3' value='" + String(esuc.ADC_Cal_Ofs_Extra) + "'></td><td><select name='a3'>");
    message = "" ;
    
    for (ii = 34; ii < 40; ii++) {
      if (esuc.ADC_Extra == ii ){
        MyColor = F(" SELECTED ");
      }else{
        MyColor = "";            
      }
      iTmp = 0 ;
      switch(ii){
        case 35: pinname = F("GPIO 35") ; break;
        case 34: pinname = F("GPIO 34") ; break;
        case 36: pinname = F("GPIO 36") ; break;
        case 39: pinname = F("GPIO 39") ; break;
        default: pinname = F("- UNKNOWN-") ; iTmp = 1 ; break;
      }
      if ( iTmp == 0 ) {
        message += "<option value="+String(ii)+ MyColor +">" + pinname ;          
      }
    }
    message += "</select></td></tr>" ;
    server.sendContent(message);

//    server.sendContent("<tr><td>Inverter Output PIN</td><td align=center><input type='text' name='iv' value='" + String(esuc.InverterPIN) + "'></td><td>.</td></tr>" ) ;
//    server.sendContent("<tr><td>Incommer Output PIN</td><td align=center><input type='text' name='ic' value='" + String(esuc.IncommerPIN) + "'></td><td>.</td></tr>" ) ;
    if (digitalRead(esuc.InverterPIN)){
      MyColor =  F("bgcolor='green'") ;
      MyCheck = F(" ON ") ;
    }else{
      MyColor =  F("bgcolor='red'") ;
      MyCheck = F(" OFF ") ;
    }
    server.sendContent("<tr><td>DIO Inverter Output PIN</td><td colspan=2 align=center " + MyColor + ">" + MyCheck + "</td><td><select name='iv'>");
    message = "" ;
    for (ii = 12; ii < 33; ii++) {
      if (esuc.InverterPIN == ii ){
        MyColor = F(" SELECTED ");
      }else{
        MyColor = "";            
      }
      iTmp = 0 ;
      switch(ii){
        case 14: pinname = F("GPIO 14") ; break;
        case 27: pinname = F("GPIO 27") ; break;
        case 16: pinname = F("GPIO 16") ; break;
        case 17: pinname = F("GPIO 17") ; break;
//        case 25: pinname = F("GPIO 25") ; break;
//        case 26: pinname = F("GPIO 26") ; break;
//        case 13: pinname = F("GPIO 13") ; break;
//        case 12: pinname = F("GPIO 12") ; break;
//        case 15: pinname = F("GPIO 15") ; break;
//        case 32: pinname = F("GPIO 32") ; break;
//        case 33: pinname = F("GPIO 33") ; break;
        default: pinname = F("- UNKNOWN-") ; iTmp = 1 ; break;
      }
      if ( iTmp == 0 ) {
        message += "<option value="+String(ii)+ MyColor +">" + pinname ;          
      }
    }

    message += "</select></td></tr>" ;
    server.sendContent(message);
    
    if (digitalRead(esuc.IncommerPIN)){
      MyColor =  F("bgcolor='green'") ;
      MyCheck = F(" ON ") ;
    }else{
      MyColor =  F("bgcolor='red'") ;
      MyCheck = F(" OFF ") ;
    }
    server.sendContent("<tr><td>DIO Incommer Output PIN</td><td colspan=2 align=center " + MyColor + ">" + MyCheck + "</td><td><select name='ic'>");
    message = "" ;
    for (ii = 12; ii < 33; ii++) {
      if (esuc.IncommerPIN == ii ){
        MyColor = F(" SELECTED ");
      }else{
        MyColor = "";            
      }
      iTmp = 0 ;
      switch(ii){
        case 14: pinname = F("GPIO 14") ; break;
        case 27: pinname = F("GPIO 27") ; break;
        case 16: pinname = F("GPIO 16") ; break;
        case 17: pinname = F("GPIO 17") ; break;
//        case 25: pinname = F("GPIO 25") ; break;
//        case 26: pinname = F("GPIO 26") ; break;
//        case 13: pinname = F("GPIO 13") ; break;
//        case 12: pinname = F("GPIO 12") ; break;
//        case 15: pinname = F("GPIO 15") ; break;
//        case 32: pinname = F("GPIO 32") ; break;
//        case 33: pinname = F("GPIO 33") ; break;
        default: pinname = F("- UNKNOWN-") ; iTmp = 1 ; break;
      }
      if ( iTmp == 0 ) {
        message += "<option value="+String(ii)+ MyColor +">" + pinname ;          
      }
    }

    message += "</select></td></tr>" ;
    server.sendContent(message);
    

    server.sendContent(F("</table></form><br><br><table border=1 title='Current ADC Data'>"));
    server.sendContent(F("<tr><th><b>ADC Channel</td><td align=center><b>Raw Value</b></th><th align=center><b>Cal Factor</b></th><th align=center><b>Eng Value</b></th><th align=center><b>Units</b></th></tr>")) ;

    server.sendContent("<tr><td>Voltage</td><td align=center>" + String(current_ADC[0]) + "</td><td align=center>"+String(esuc.ADC_Cal_Voltage)+"</td><td align=center>" + String((current_ADC[0]*esuc.ADC_Cal_Voltage/4096)+esuc.ADC_Cal_Ofs_Voltage) + "</td><td align=center>(V)</td></tr>" ) ;
    server.sendContent("<tr><td>Current In</td><td align=center>" + String(current_ADC[1]) + "</td><td align=center>"+String(esuc.ADC_Cal_CurrentIn)+"</td><td align=center>" + String((current_ADC[1]*esuc.ADC_Cal_CurrentIn/4096)+esuc.ADC_Cal_Ofs_CurrentIn) + "</td><td align=center>(A)</td></tr>" ) ;
    server.sendContent("<tr><td>Current Out</td><td align=center>" + String(current_ADC[2]) + "</td><td align=center>"+String(esuc.ADC_Cal_CurrentOut)+"</td><td align=center>" + String((current_ADC[2]*esuc.ADC_Cal_CurrentOut/4096)+esuc.ADC_Cal_Ofs_CurrentOut) + "</td><td align=center>(A)</td></tr>" ) ;
    server.sendContent("<tr><td>???</td><td align=center>" + String(current_ADC[3]) + "</td><td align=center>"+String(esuc.ADC_Cal_Extra)+"</td><td align=center>" + String((current_ADC[3]*esuc.ADC_Cal_Extra/4096)+esuc.ADC_Cal_Ofs_Extra) + "</td><td align=center>()</td></tr>" ) ;
    server.sendContent(F("</table>"));

  }

  SendHTTPPageFooter();

}

