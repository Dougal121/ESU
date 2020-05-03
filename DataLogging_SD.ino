void DataLog(String TargetFolder){
bool bHeader = false ;
  if ( TargetFolder == "EOD" ){
    snprintf(buff, BUFF_MAX, "%s/%04d%02d%02d.csv", TargetFolder.c_str() , tc.year , tc.mon , tc.mday );
  }else{
    snprintf(buff, BUFF_MAX, "%s/%04d%02d%02d.csv", TargetFolder.c_str() , tc.year , tc.mon , tc.mday );      
  }
  
  Serial.println(buff);
  if ( hasSD ){
    if (!SD.exists(buff)){    // ok if its the first write include a header
      bHeader = true ;
    }
    File dataFile = SD.open(buff, FILE_WRITE);
    if (dataFile) {
      if ( bHeader) {
        dataFile.println("Date,Voltage,Current In,Current Out,Extra");
      }
      snprintf(buff, BUFF_MAX, "%4d-%02d-%2dT%02d:%02d:%02d", tc.year, tc.mon, tc.mday , tc.hour, tc.min, tc.sec);
      dataFile.print(String(buff)+",");
      dataFile.print(String((current_ADC[0] * esuc.ADC_Cal_Voltage / 4096) + esuc.ADC_Cal_Ofs_Voltage)+",");
      dataFile.print(String((current_ADC[1] * esuc.ADC_Cal_CurrentIn / 4096) + esuc.ADC_Cal_Ofs_CurrentIn)+",");
      dataFile.print(String((current_ADC[2] * esuc.ADC_Cal_CurrentOut / 4096) + esuc.ADC_Cal_Ofs_CurrentOut)+",");
      dataFile.print(String((current_ADC[3] * esuc.ADC_Cal_Extra / 4096) + esuc.ADC_Cal_Ofs_Extra));
      dataFile.close();
    }    
  }
}

int SendFile (String FileName  ){
  File dataFile = SD.open(FileName);
  if (dataFile) {                                 // if the file is available, write to it:
    while (dataFile.available()) {
//      streamFile(dataFile,"");
      server.sendContent(String(dataFile.read()));
    }
    dataFile.close();
  }else {    // if the file isn't open, pop up an error:
    server.sendContent("error opening " + FileName);
  }
}

void printDirectory(File dir, int numTabs,  String MyFolder) {
File entry ;  
static unsigned long lTB ;

//     Serial.println("Called " + String(numTabs));
    if( numTabs == 0 ) {
      lTB = 0 ;  
      dir.rewindDirectory(); // start at the begining
       server.sendContent("<table border=0>");
    }
   while(entry = dir.openNextFile()) {  // single = very subtle assign and test
//     Serial.println(entry.name());
     if (entry.isDirectory()) {
        MyFolder = String("/") + entry.name() ;
        server.sendContent("<tr><td><b>" + MyFolder + "</td></tr>" );
        printDirectory(entry, numTabs+1, MyFolder);
//        Serial.println("Returned ");
     }else{
       server.sendContent("<tr><td><A href='"); 
       server.sendContent(MyFolder+ "/" + entry.name());
       server.sendContent("'>");
       server.sendContent(entry.name());
       server.sendContent("</a></td><td> </td><td align=right>");
       server.sendContent(String(entry.size(), DEC));      
       lTB += entry.size() ;  
       server.sendContent("</td></tr>");
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

