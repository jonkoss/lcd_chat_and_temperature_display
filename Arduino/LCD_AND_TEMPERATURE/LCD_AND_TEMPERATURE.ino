#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

LiquidCrystal_I2C lcd(0x3F, 16, 2);

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 3
#define DHTTYPE DHT11   // DHT 11

// TO BE CHANGED
const char* ssid     = "*****************"; // SSID from your WIFI
const char* password = "*****************"; // Password from your WIFI
const int DHTPin = 16; //PIN D0 for DHT11 Input DATA
// *************************************************

IPAddress gateway(192,168,178,1); // IP-Adress Wifi-Gateway
IPAddress subnet(255,255,255,0);  // Subnet
IPAddress ip(192,168,178,50); // Static IP Adress for WeMos

WiFiServer server(23); // --> default port for communication usign TELNET protocol | Server Instance
WiFiClient serverClients[MAX_SRV_CLIENTS]; // --> Client Instanse

DHT dht(DHTPin, DHTTYPE);

String Name;
int i;
String temperature = "Temperature: ";
String humidity = "Humidity: ";

void setup() {
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.init();

  // Turn on the backlight.
  lcd.backlight();

  WiFi.config(ip, gateway, subnet);
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  dht.begin();
  // -->Try to connect to particular host for 20 times, If still not connected then automatically resets.
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1);
  }
  //start UART and the server
  Serial.begin(115200);
  server.begin();
  server.setNoDelay(true); // --> Won't be storing data into buffer and wait for the ack. rather send the next data and in case nack is received, it will resend the whole data
  
  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

void loop() {
     lcd.print(getString());
}

char* getString()
{
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        Serial.print("New client: "); Serial.println(i);
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      static char str[21]; // For strings of max length=20
      if(serverClients[i].available()){
          delay(64); // wait for all characters to arrive
          memset(str,0,sizeof(str)); // clear str
          byte count=0;
        //get data from the telnet client and push it to the UART
        while(serverClients[i].available()){
           char c=serverClients[i].read();
           if (c>=32 && count<sizeof(str)-1)
             {
                lcd.clear();
                str[count]=c;
                count++;
             }
        }  
        str[count]='\0'; // make it a zero terminated string
        if (strcmp(str, "Temperature") != 0)
            return str;
        else
        {
            float h = dht.readHumidity();
            float t = dht.readTemperature();
            if (isnan(h) || isnan(t)) 
               lcd.print("Failed to read from DHT sensor!");
            else
               lcd.print("Temperature:");
               lcd.print(t);
               lcd.setCursor(0,1);
               lcd.print("Humidity:");
               lcd.print(h);
        }
    }
  }
  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        serverClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
 }
}



