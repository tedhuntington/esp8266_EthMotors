#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>


#define ROBOT_MOTORS_PCB_NAME 1
#define ROBOT_MOTORS_SEND_4BYTE_INST 0x20

WiFiUDP Udp;
ESP8266WebServer server(80);

const char* ssid = "EthMotors";
const char *passphrase = "";
String st;
String content;
int statusCode;

unsigned int localUdpPort = 53510;
unsigned char incomingPacket[255],ReturnPacket[255],mac[6];
char tempstr[255];
String DestStr;



void setup()
{

  Serial.begin(115200);
  Serial.println();

//  pinMode(3,INPUT); //set 3=GPIO0 (was 2=GPIO02) as input

  //WiFi.begin("SSID", "pw");

  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  //Serial.println(epass);  
  Serial.println("hidden");  
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);

        //Serial.println("waiting for connection to start UDP Server");
        //while (WiFi.status() != WL_CONNECTED) { //wait until connected
        //  delay(500);
        //}
        if (Udp.begin(localUdpPort)) {
          Serial.println("UDP Server Initiiated");
        } else {
          Serial.println("UDP Server Failed");
        }
        //udp.beginMulticast(WiFi.localIP(), multicast_ip_addr, localUdpPort)
      

        return;
      } 
  }
  setupAP();

}


bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 26 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
} 

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        content += ipStr;
        content += "<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        content += "</html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              //Serial.print("Wrote: ");
              //Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              //Serial.print("Wrote: ");
              //Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
          statusCode = 200;
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
      WiFi.disconnect();
    });
  }
}

void loop() {

  //if gpio00 goes low, reset ssid and restart (in ap mode)
#if 0 
  if (!digitalRead(3)) {
//    wifiManager.resetSettings();
    Serial.println("Resetting ESP");
    Serial.println("clearing eeprom");
    for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
    EEPROM.commit();
    WiFi.disconnect();
    ESP.restart(); //restart the ESP8266
  } //if (!digitalRead(2)) {
#endif

 
  //note that we receive broadcast packets on the same subnet too
  int packetSize = Udp.parsePacket();
  
  if (packetSize)
  {
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    //if broadcast ID, respond with "MOTOR" so the Robot App knows that this IP belongs to an EthMotors PCB
   // sprintf(tempstr,Udp.destinationIP().toString().c_str());
    DestStr= Udp.destinationIP().toString().c_str();
    if (DestStr.endsWith("255")) {
    //tlen=strlen(tempstr);
    //if (endsWith(,"255") {
      if (incomingPacket[4]==ROBOT_MOTORS_PCB_NAME) {
        //Serial.printf("Broadcast\n");
        //send back "MOTOR"
        memcpy(ReturnPacket,incomingPacket,5); //copy IP + instruction byte      
        WiFi.macAddress(mac);
        memcpy(ReturnPacket+5,mac,6); //add mac address
        sprintf(tempstr,"Motor");
        memcpy(ReturnPacket+11,tempstr,5); //add "Motor"
        Udp.beginPacket(Udp.remoteIP(),localUdpPort);
        Udp.write(ReturnPacket,16);
        Udp.endPacket();
        //Serial.write(ReturnPacket,16);  
      } //if (incomingPacket[4]==ROBOT_MOTORS_PCB_NAME) {         
    } else { //if (DestStr.endsWith("255")) {
      //not broadcast
      if (incomingPacket[4]==ROBOT_MOTORS_SEND_4BYTE_INST) {
        //Serial.printf("Motor Inst\n");
        //for now only sending along motor instructions to the usart
        Serial.write(incomingPacket,9);          
      } //if (incomingPacket[4]==ROBOT_MOTORS_SEND_4BYTE_INST) {
    } //if (DestStr.endsWith("255")) {
    //send instruction to EthMotors over USART
    //Serial.printf("UDP packet contents: %s\n", incomingPacket);  
    //Serial.write(incomingPacket,len);  
   }//if (packetSize)


  server.handleClient();  //handle web server tasks

}
