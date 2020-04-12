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

