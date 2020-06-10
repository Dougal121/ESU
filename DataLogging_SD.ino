void DataLog(String TargetFolder){
bool bHeader = false ;
  if ( TargetFolder == "EOD" ){
    snprintf(buff, BUFF_MAX, "%s/%04d%02d%02d.csv", TargetFolder.c_str() , year() , month() , day() );
  }else{
    snprintf(buff, BUFF_MAX, "%s/%04d%02d%02d.csv", TargetFolder.c_str() , year() , month() , day() );      
  }
  
//  Serial.println(buff);
  if ( hasSD ){
    if (!SD.exists(buff)){    // ok if its the first write include a header
      bHeader = true ;
    }
    File dataFile = SD.open(buff, FILE_WRITE);
    if (dataFile) {
      Serial.print(".");
      if ( bHeader) {
        dataFile.println("Date,Voltage,Current In,Current Out,Extra,Temp1,Temp2,Temp3,RSSI");
        Serial.print("*");
      }else{
        dataFile.seek(dataFile.size());
      }
      Serial.print(".");
      snprintf(buff, BUFF_MAX, "%4d-%02d-%02dT%02d:%02d:%02d", year(), month(), day() , hour(), minute(), second() );
      dataFile.print(String(buff)+",");
      dataFile.print(String((current_ADC[0] * esuc.ADC_Cal_Voltage / 4096) + esuc.ADC_Cal_Ofs_Voltage)+",");
      dataFile.print(String((current_ADC[1] * esuc.ADC_Cal_CurrentIn / 4096) + esuc.ADC_Cal_Ofs_CurrentIn)+",");
      dataFile.print(String(esui.fAmpHours)+",");
      dataFile.print(String((current_ADC[3] * esuc.ADC_Cal_Extra / 4096) + esuc.ADC_Cal_Ofs_Extra)+",");
      dataFile.println(String(esui.fTemp[0],2)+","+String(esui.fTemp[1],2)+","+String(esui.fTemp[2],2)+","+String(abs(WiFi.RSSI())));
      dataFile.close();
    }    
  }
}


void printDirectory(File dir, int numTabs,  String MyFolder) {
File entry ;  
int i ;
String strLeaf ;
static unsigned long lTB ;

//  Serial.println("Called " + String(numTabs));
  if( numTabs == 0 ) {
    lTB = 0 ;  
    dir.rewindDirectory(); // start at the begining
     server.sendContent("<table border=0>");
  }
  while(entry = dir.openNextFile()) {  // single = very subtle assign and test
//    Serial.println(entry.name());
    if (entry.isDirectory()) {
        MyFolder = String("/") + entry.name() ;
        MyFolder = entry.name() ;
        server.sendContent("<tr><td><b>" + MyFolder + "</td></tr>" );
        printDirectory(entry, numTabs+1, MyFolder);
//        Serial.println("Returned ");
    }else{
      server.sendContent("<tr><td><A href='download?file="); 
      server.sendContent(entry.name());  //      + "/"
      server.sendContent("'>");
      server.sendContent(entry.name());
      server.sendContent("</a></td><td> </td><td align=right>");
      server.sendContent(String(entry.size(), DEC));      
      lTB += entry.size() ;  
      server.sendContent("</td>");
      i = String(entry.name()).lastIndexOf(".csv");
      if (i != -1) { //
        strLeaf = String("<td><A href='chartfile?file="+String(entry.name())+"'><img src='download?file=/chart.ico' alt='chart' width=16 height=16></a></td>");
        server.sendContent(strLeaf);
      }       
      server.sendContent("</tr>");
    }
  }
  if( numTabs == 0 ) {
     server.sendContent("<tr><td> </td></tr>");
     server.sendContent("<tr><td><b>Total Bytes</td><td> </td><td align=right><b>");
     server.sendContent(String(lTB, DEC));      
     server.sendContent("</td></tr>");
     server.sendContent("</table>");      
  }
}


void indexSDCard()
{
  String message ;

  SerialOutParams();
  SendHTTPHeader();   //  ################### START OF THE RESPONSE  ######
  File root = SD.open("/");                
  printDirectory(root,0,"/");
  root.close();
  SendHTTPPageFooter();  
}

void chartFile()
{
  String strExt = "" ;
  String dataType = "" ;
  String strFile = "" ;
  String strLeaf = "" ;
  String strDate = "" ;
  int i , ii , iTmp , iX ;
  int j , k , kk ;
  String message ;  
  String msg2 ;  
  byte mac[6];
  byte  sdchar ; 
  time_t prev_time;
  File dataFile ;
  SerialOutParams();
    
  for (uint8_t j = 0; j < server.args(); j++) {
    //    bSaveReq = 1 ;
    i = String(server.argName(j)).indexOf("file");
    if (i != -1) { //
      i = String(server.arg(j)).lastIndexOf(".");
      if (i != -1) { //
        strExt = String(server.arg(j)).substring(i+1);
        strExt.toLowerCase();
        strFile = String(server.arg(j)) ;
        strLeaf = strFile ; 
        i = String(server.arg(j)).lastIndexOf("/");
        if (i != -1) { //
          strLeaf = String(server.arg(j)).substring(i+1);
        }
      }
    }
  }
//  Serial.println("Leaf: "+String(strLeaf));
//  Serial.println("File: "+String(strFile));
//  Serial.println("Ext: "+String(strExt));

  SendHTTPHeader();

  if (SD.exists(strFile)){    // ok if its the first write include a header
    dataFile = SD.open(strFile, FILE_READ);    
  
    server.sendContent(F("<center>\r\n<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\r\n"));  
    server.sendContent(F("\r\n<script type=\"text/javascript\">\r\n"));      
    server.sendContent(F("google.charts.load('current', {'packages':['corechart']});\r\n"));  // Load the Visualization API and the piechart package.    
    server.sendContent(F("google.charts.setOnLoadCallback(drawChart);\r\n"));                 // Set a callback to run when the Google Visualization API is loaded.
    server.sendContent(F("function drawChart() {\r\n"));
    server.sendContent(F("var data = google.visualization.arrayToDataTable([[{label: 'Time', type: 'datetime'},{label: 'Battery Volatage', type: 'number'},{label: 'Net Current', type: 'number'},{label: 'Amp Hours', type: 'number'},{label: 'Extra', type: 'number'},{label: 'Temp 1', type: 'number'},{label: 'Temp 2', type: 'number'},{label: 'Temp 3', type: 'number'},{label: 'RSSI', type: 'number'}],\r\n"));
      
    snprintf(buff, BUFF_MAX, "new Date(\'%4d-%02d-%02dT%02d:%02d:%02d\')", year(esul[j].RecTime), month(esul[j].RecTime), day(esul[j].RecTime) , hour(esul[j].RecTime), minute(esul[j].RecTime), second(esul[j].RecTime));    
  //    if (( !isnan(esul[j].Voltage)) && (esul[j].RecTime>=prev_time)){
  //      server.sendContent("[ " + String(buff) + "," + String(esul[j].Voltage) + "," + String(esul[j].Current) + "," + String(esul[j].AmpHours) + " ] ,\r\n" ) ; 
    sdchar = 0 ;
    while ((dataFile.available())&&(char(sdchar) != '\n')){ // skip the firstline
      sdchar = dataFile.read();
//      Serial.print(char(sdchar));
    }
    message = "" ;    
    while(dataFile.available()){
      sdchar = dataFile.read();
//      Serial.print(char(sdchar));
      message += String(char(sdchar));                       // read a byte
      if ((char(sdchar) == '\n' ) ){
//        Serial.print(message + "<");
        if (message.length()>0 ){
          i = message.indexOf(',');
//          Serial.print("<");
          if ( i > 0 ){
//            Serial.print("<");
            msg2 = "new Date(\'" + message.substring(0,i) + "\')" + message.substring(i,(message.length()-2));            
            server.sendContent("[ " + msg2 + " ] ,\r\n" );
            if ( strDate.length() == 0 )
              strDate = message.substring(0,10) ;
            message = "" ;
          } 
        }
      }
    }

    dataFile.close();
    
    server.sendContent(F("]);\r\n"));
       
    server.sendContent("var options = {title: 'System Logs 1 min intervals 24 Hours for " + strDate + "' , vAxis:{viewWindow:{ max: 60, min: 0}} , height: 700 , opacity:100 , interpolateNulls:true , colors: ['Blue','Red','Orange','Green','Magenta','#00007F','#00005F','#00003F'], backgroundColor: '#FFFFFF', ");  // Set chart options
    server.sendContent(F("  };\r\n"));
  
    server.sendContent(F("var chart = new google.visualization.LineChart(document.getElementById('linechart'));\r\n"));
    server.sendContent(F("chart.draw(data, options); } </script>\r\n"));
    server.sendContent(F("<div id='linechart'></div><br>\r\n"));  //  style='width:1000; height:800'  
  
  }else{
    server.sendContent(F("<center>No File on SD card<br><br>"));  
  }


  
  SendHTTPPageFooter();
 
}


void downloadFile()
{
  String strExt = "" ;
  String dataType = "" ;
  String strFile = "" ;
  String strLeaf = "" ;
  int i = 0 ;

  SerialOutParams();
    
  for (uint8_t j = 0; j < server.args(); j++) {
    //    bSaveReq = 1 ;
    i = String(server.argName(j)).indexOf("file");
    if (i != -1) { //
      i = String(server.arg(j)).lastIndexOf(".");
      if (i != -1) { //
        strExt = String(server.arg(j)).substring(i+1);
        strExt.toLowerCase();
        strFile = String(server.arg(j)) ;
        strLeaf = strFile ; 
        i = String(server.arg(j)).lastIndexOf("/");
        if (i != -1) { //
          strLeaf = String(server.arg(j)).substring(i+1);
        }
      }
    }
  }
  Serial.println("Leaf: "+String(strLeaf));
  Serial.println("File: "+String(strFile));
  Serial.println("Ext: "+String(strExt));

  if (strExt.lastIndexOf("htm")>0) {
    dataType = "text/html";
  } else if (strExt.lastIndexOf("css")>=0) {
    dataType = "text/css";
  } else if (strExt.lastIndexOf("js")>=0) {
    dataType = "application/javascript";
  } else if (strExt.lastIndexOf("png")>=0) {
    dataType = "image/png";
  } else if (strExt.lastIndexOf("gif")>=0) {
    dataType = "image/gif";
  } else if (strExt.lastIndexOf("jpg")>=0) {
    dataType = "image/jpeg";
  } else if (strExt.lastIndexOf("ico")>=0) {
    dataType = "image/x-icon";
  } else if (strExt.lastIndexOf("xml")>=0) {
    dataType = "text/xml";
  } else if (strExt.lastIndexOf("pdf")>=0) {
    dataType = "application/pdf";
  } else if (strExt.lastIndexOf("zip")>=0) {
    dataType = "application/zip";
  } else if (strExt.lastIndexOf("csv")>=0) {
    dataType = "application/csv";
  }
  Serial.println(dataType);
  File dataFile = SD.open(strFile);
  if (dataFile) {                                 // if the file is available, write to it:
//    while (dataFile.available()) {
      server.sendHeader(F("Content-Disposition"),"inline; filename=" + String(strLeaf),true);
      server.streamFile(dataFile,dataType);
//      server.sendContent(String(dataFile.read()));
//    }
    dataFile.close();
  }else {    // if the file isn't open, pop up an error:
    server.sendContent("Error opening " + strFile);
  }
}

