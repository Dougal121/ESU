void datalog_page(){
  int i , ii , iTmp , iX ;
  uint8_t j , k , kk ;
  String message ;  
  String MyNum ;  
  String MyColor ;
  String MyColor2 ;
  byte mac[6];
 

//  SerialOutParams();
  
  for (uint8_t j=0; j<server.args(); j++){
  }
  
  SendHTTPHeader();

  server.sendContent(F("<br><center><b>Data Log</b><br>"));
  server.sendContent(F("<table border=1 title='Data Log'>"));
  server.sendContent("<tr><th>Date</th><th>Voltage</th><th>Current</th><th>Amp Hours</th></tr>" ) ; 
  for ( i = 0 ; i < MAX_LOG ; i++ ) {
    snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(esul[i].RecTime), month(esul[i].RecTime), day(esul[i].RecTime) , hour(esul[i].RecTime), minute(esul[i].RecTime), second(esul[i].RecTime));    
    if ( !isnan(esul[i].Voltage) ){
      server.sendContent("<tr><td>" + String(buff) + "</td><td>" + String(esul[i].Voltage) + "</td><td>" + String(esul[i].Current) + "</td><td>" + String(esul[i].AmpHours) + "</td></tr>" ) ; 
    }
  }
  server.sendContent(F("</table><br>"));    
  SendHTTPPageFooter();
}


void datalog2_page(){
  int i , ii , iTmp , iX ;
  uint8_t j , k , kk ;
  String message ;  
  String MyNum ;  
  String MyColor ;
  String MyColor2 ;
  byte mac[6];
  time_t MyTime;

//  SerialOutParams();
  
  for (uint8_t j=0; j<server.args(); j++){
  }
  
  SendHTTPHeader();

  server.sendContent(F("<br><center><b>Battery Log by the minute</b><br>"));
  server.sendContent(F("<table border=1 title='Data Logs by Minute'>"));
  server.sendContent("<tr><th>Date</th><th>Voltage</th></tr>" ) ; 
  ii = (((hour()*60)+minute())*60)+ 1 ;
  MyTime = now() - (1440*60) ;                                   // 24hrs ago
  for ( i = 0 ; i < 1440 ; i++ ) {
    snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(MyTime), month(MyTime), day(MyTime) , hour(MyTime), minute(MyTime), second(MyTime));    
    if ( !isnan(fVD[(i+ii)%1440]) ){
      server.sendContent("<tr><td>" + String(buff) + "</td><td>" + String(fVD[(i+ii)%1440]) + "</td></tr>" ) ; 
    }
    MyTime += 60 ;        // add one minute
  }
  server.sendContent(F("</table><br>"));    
  SendHTTPPageFooter();
}
