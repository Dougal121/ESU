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
  server.sendContent(F("<center>\r\n<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\r\n"));  

  server.sendContent(F("\r\n<script type=\"text/javascript\">\r\n"));      
  server.sendContent(F("google.charts.load('current', {'packages':['corechart']});\r\n"));  // Load the Visualization API and the piechart package.    
  server.sendContent(F("google.charts.setOnLoadCallback(drawChart);\r\n"));                 // Set a callback to run when the Google Visualization API is loaded.

      // Callback that creates and populates a data table, 
      // instantiates the pie chart, passes in the data and
      // draws it.
  server.sendContent(F("function drawChart() {\r\n"));
      // Create the data table.
  server.sendContent(F("var data = google.visualization.arrayToDataTable([[{label: 'Time', type: 'datetime'},{label: 'Battery Volatage', type: 'number'}],\r\n"));
  ii = (((hour()*60)+minute())*60)+ 1 ;
  MyTime = now()-second() - (1440*60) ;                                   // 24hrs ago to the minute
  for ( i = 0 ; i < 1440 ; i++ ) {
    snprintf(buff, BUFF_MAX, "new Date(\'%4d-%02d-%2dT%02d:%02d:%02d\')", year(MyTime), month(MyTime), day(MyTime) , hour(MyTime), minute(MyTime), second(MyTime));    
    if ( !isnan(fVD[(i+ii)%1440]) ){
      server.sendContent("[ " + String(buff) + " , " + String(fVD[(i+ii)%1440]) + " ] ,\r\n" ) ; 
//      server.sendContent("[ " + String(i) + " , " + String(fVD[(i+ii)%1440]) + " ] ,\r\n" ) ; 
    }
    MyTime += 60 ;        // add one minute
  }
  server.sendContent(F("]);\r\n"));
     
  server.sendContent(F("var options = {title: 'Battery Volatge Log By Minute for last 24 Hours', legend: 'none' , vAxis:{viewWindow:{ max: 60, min: 40}} , height: 700 , opacity:100 , interpolateNulls:true , colors: ['Blue'], backgroundColor: '#FFFFFF', "));  // Set chart options
  server.sendContent(F("  };\r\n"));

  server.sendContent(F("var chart = new google.visualization.LineChart(document.getElementById('linechart'));\r\n"));
  server.sendContent(F("chart.draw(data, options); } </script>\r\n"));
  server.sendContent(F("<div id='linechart'></div><br>\r\n"));  //  style='width:1000; height:800'
  
/*  server.sendContent(F("<table border=1 title='Data Logs by Minute'>\r\n"));
  server.sendContent(F("<tr><th>Date</th><th>Voltage</th></tr>\r\n")) ; 
  
  ii = (((hour()*60)+minute())*60)+ 1 ;
  MyTime = now()-second() - (1440*60) ;                                   // 24hrs ago to the minute
  for ( i = 0 ; i < 1440 ; i++ ) {
    snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d", year(MyTime), month(MyTime), day(MyTime) , hour(MyTime), minute(MyTime));    
    if ( !isnan(fVD[(i+ii)%1440]) ){
      server.sendContent("<tr><td>" + String(buff) + "</td><td align=right>" + String(fVD[(i+ii)%1440]) + "</td></tr>" ) ; 
    }
    MyTime += 60 ;        // add one minute
  }
  server.sendContent(F("</table><br>"));    */
  SendHTTPPageFooter();
}
