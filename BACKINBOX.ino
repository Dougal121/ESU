void BackInTheBoxMemory(){
  int i , j ;

  sprintf(ghks.nssid,"************\0");  // put your default credentials in here if you wish
  sprintf(ghks.npassword,"********\0");  // put your default credentials in here if you wish


  sprintf(ghks.nssid,"WLAN-PLUMMER\0");    // dougals office
  sprintf(ghks.npassword,"cheegh5S\0");

  sprintf(ghks.cpassword,"\0");
  
  sprintf(ghks.NodeName,"ESU SCADA\0") ;
  ghks.fTimeZone = 10.0 ;
  ghks.lNodeAddress = (long)chipid & 0xff ;
  sprintf(ghks.timeServer ,"au.pool.ntp.org\0"); 
  ghks.AutoOff_t = 0 ;
  ghks.localPortCtrl = 8088 ;
  ghks.RemotePortCtrl= 8089 ;
  ghks.lVersion = MYVER ;
  
  ghks.RCIP[0] = 192 ;
  ghks.RCIP[1] = 168 ; 
  ghks.RCIP[2] = 2 ;
  ghks.RCIP[3] = 255 ;
  
  ghks.lNetworkOptions = 0 ;     // DHCP 
  ghks.IPStatic[0] = 192 ;
  ghks.IPStatic[1] = 168 ;
  ghks.IPStatic[2] = 2 ;
  ghks.IPStatic[3] = 234 ;

  ghks.IPGateway[0] = 192 ;
  ghks.IPGateway[1] = 168 ;
  ghks.IPGateway[2] = 1 ;
  ghks.IPGateway[3] = 1 ;

  ghks.IPDNS = ghks.IPGateway ;

  ghks.IPMask[0] = 255 ;
  ghks.IPMask[1] = 255 ;
  ghks.IPMask[2] = 255 ;
  ghks.IPMask[3] = 0 ;

  ghks.latitude = -34.051219 ;
  ghks.longitude = 142.013618 ;

  esuc.MinVoltage = 48.5 ;
  esuc.ResetVoltage = 53.5 ;
  esuc.iICtoInverter = 5 ;
  esuc.ExtraVoltage = 49.3 ;
  esuc.bMasterEnable = true ;
  esuc.bSolarEnable = false ;
  esuc.InverterPIN = 27 ;
  esuc.IncommerPIN = 16 ;

  esuc.iOnTimeSunset = 180 ;
  esuc.iOnTimeSunrise = 60  ;
  esuc.bMasterEnable = true ;
  esuc.bSolarEnable = false ;


  esuc.ADC_Voltage = 35 ;       // 
  esuc.ADC_CurrentIn = 34 ;     //
  esuc.ADC_CurrentOut = 36 ;    //
  esuc.ADC_Extra = 39 ;         //

  esuc.ADC_Cal_Voltage = 290 ;        // 
  esuc.ADC_Cal_CurrentIn = 100 ;     //
  esuc.ADC_Cal_CurrentOut = 100 ;    //
  esuc.ADC_Cal_Extra = 100 ;         //

  esuc.ADC_Cal_Ofs_Voltage = 0 ;        // 
  esuc.ADC_Cal_Ofs_CurrentIn = 0 ;     //
  esuc.ADC_Cal_Ofs_CurrentOut = 0 ;    //
  esuc.ADC_Cal_Ofs_Extra = 0 ;         //

  for ( i = 0 ; i < MAX_LOG ; i++ ) { // clear out the log
    esul[i].RecTime = now() ;
    esul[i].Voltage = 0 ;
    esul[i].Current = 0 ;
    esul[i].AmpHours = 0 ;
  }
  for ( i = 0 ; i < 48 ; i++){   // turn off all slots 
    if (( i >= 38 ) && ( i < 46 )){
      esuc.ActiveTimes[i] = true ;       
    }else{
      esuc.ActiveTimes[i] = false ; 
    }
  }  
  for ( i = 0 ; i < 1440 ; i++ ) {
    fVD[i] = 0 ;
  }  
}


void LoadParamsFromEEPROM(bool bLoad){
long lTmp ;  
int i ;
int j ;
int bofs ,ofs ;
int eeAddress ;

  if ( bLoad ) {
    EEPROM.get(0,ghks);
    eeAddress = sizeof(ghks) ;
    Serial.println("read - ghks structure size " +String(eeAddress));   

    ghks.lNodeAddress = constrain(ghks.lNodeAddress,0,32768);
    ghks.fTimeZone = constrain(ghks.fTimeZone,-12,12);
    ghks.localPort = constrain(ghks.localPort,1,65535);
    ghks.localPortCtrl = constrain(ghks.localPortCtrl,1,65535);
    ghks.RemotePortCtrl = constrain(ghks.RemotePortCtrl,1,65535);
    if ( year(ghks.AutoOff_t) < 2000 ){
       ghks.AutoOff_t = now();
    }
    ghks.lDisplayOptions = constrain(ghks.lDisplayOptions,0,1);

    ghks.latitude = constrain(ghks.latitude,-90.0,90.0);
    ghks.longitude = constrain(ghks.longitude,-180.0,180.0);
    ghks.iDayNight = 1 ;

    eeAddress = PROG_BASE ;  // 192 which is 48 * sizeof(float)
    EEPROM.get(eeAddress,esuc);
    eeAddress += sizeof(esuc) ;
    EEPROM.get(eeAddress,esul);
    eeAddress += sizeof(esul) ;

    esuc.IncommerPIN = constrain(esuc.IncommerPIN,12,33);
    esuc.InverterPIN = constrain(esuc.InverterPIN,12,33);

    esuc.ADC_Voltage = constrain(esuc.ADC_Voltage,34,39);        // 
    esuc.ADC_CurrentIn = constrain(esuc.ADC_CurrentIn,34,39);    //
    esuc.ADC_CurrentOut = constrain(esuc.ADC_CurrentOut,34,39);  //
    esuc.ADC_Extra = constrain(esuc.ADC_Extra,34,39);            //

    esuc.ADC_Cal_Voltage = constrain(esuc.ADC_Cal_Voltage ,0 ,10000 ) ;         //   
    esuc.ADC_Cal_CurrentIn = constrain(esuc.ADC_Cal_CurrentIn ,0 ,10000 ) ;     //
    esuc.ADC_Cal_CurrentOut = constrain(esuc.ADC_Cal_CurrentOut ,0 ,10000 );    //
    esuc.ADC_Cal_Extra = constrain(esuc.ADC_Cal_Extra,0,10000) ;                //


     
/*    efertAddress = eeAddress ;
    EEPROM.get(eeAddress,efert);
    eeAddress += sizeof(efert) ;
    EEPROM.get(eeAddress,efilter);
    eeAddress += sizeof(efilter) ;
    EEPROM.get(eeAddress,eboard);
    eeAddress += sizeof(eboard) ;
    EEPROM.get(eeAddress,elocal);
    eeAddress += sizeof(elocal) ;
    EEPROM.get(eeAddress,pn);
    eeAddress += sizeof(pn) ;
*/    
    Serial.println("Final EEPROM load adress " +String(eeAddress));   
    
  }else{
    ghks.lVersion  = MYVER ;
    EEPROM.put(0,ghks);
    eeAddress = sizeof(ghks) ;
    Serial.println("write - ghks structure size " +String(eeAddress));   

    eeAddress = PROG_BASE ;
    EEPROM.put(eeAddress,esuc);
    eeAddress += sizeof(esuc) ;
    EEPROM.put(eeAddress,esul);
    eeAddress += sizeof(esul) ;    
/*    EEPROM.put(eeAddress,efert);
    eeAddress += sizeof(efert) ;
    EEPROM.put(eeAddress,efilter);
    eeAddress += sizeof(efilter) ;
    EEPROM.put(eeAddress,eboard);
    eeAddress += sizeof(eboard) ;
    EEPROM.put(eeAddress,elocal);
    eeAddress += sizeof(elocal) ;
    EEPROM.put(eeAddress,pn);
    eeAddress += sizeof(pn) ;
 */   
    Serial.println("Final EEPROM Save adress " +String(eeAddress));   

    EEPROM.commit();                                                       // save changes in one go ???
    bSaveReq = 0 ;
  }
}

