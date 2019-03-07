#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


#define ROBOT_MOTORS_PCB_NAME 1
#define ROBOT_MOTORS_SEND_4BYTE_INST 0x20

WiFiUDP Udp;
unsigned int localUdpPort = 53510;
unsigned char incomingPacket[255],ReturnPacket[255],mac[6];
char tempstr[255];
String DestStr;

void setup()
{

  Serial.begin(115200);
  Serial.println();

  WiFi.begin("TedHuntington", "b00bf00d00");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  Udp.begin(localUdpPort);
  //udp.beginMulticast(WiFi.localIP(), multicast_ip_addr, localUdpPort)
}

void loop() {

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
   }  
}
