
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#define RX 02
#define TX 03
SoftwareSerial RFID(RX, TX); // RX and TX

const char* ssid     = "Google2";
const char* password = "DrioMatriX96";

const char* host = "192.168.254.114";
const char* streamId   = "....................";
const char* privateKey = "....................";
byte i;
String value="";
int b=0;
String c="";
String tag = "";
char dataStr[12];
void setup() {
  Serial.begin(9600);
  delay(10);
  Serial.println();
  RFID.begin(9600); // start serial to RFID reader
 
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected"); 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int trigger = 0;

void loop() {

  if(RFID.available() > 0)
    {
    i = RFID.read();
    //delay(1000);
    value = i;
    c = value.length();
    // Serial.print(c);
    b = b + c.toInt();
    //Serial.println(b);
    tag = tag + value ;
    Serial.println("RFID Tag: " + value);
    }
    if(b==22){
      b=0;
      //Serial.println("RFID Tag: " + tag);
      trigger=1;
    }
    //sending data
    if(trigger==1){
    delay(5000);
     //++value;

  //Serial.print("connecting to ");
  //Serial.println(host);
 
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 81;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
 
  // We now create a URI for the request
//  String url = "/input/";
//  url += streamId;
//  url += "?private_key=";
//  url += privateKey;
//  url += "&value=";
//  url += value;

  String url = "/iot/";
  url += "insertdata.php?tag=";
  url += tag;
  Serial.println("Sending data to web.");
  //Serial.print("Requesting URL: ");
  //Serial.println(url);
 
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(1000);
 
  // Read all the lines of the reply from server and print them to Serial
  //int x = 0;
  while(client.available()){
    char line = client.read();
    //String line = client.readStringUntil('\r');
    //Serial.print(line);
    if(String(line) == "!"){
      Serial.println("Record Exist!");
    }
    if(String(line) == "~"){
      Serial.println("Record Added!");
    }
    
    //dataStr[x] = line;
    //x++;
  }
  //for(x=0;x<=12;x++){

  //  Serial.print(dataStr[x]);

  //}

 
  Serial.println();
  Serial.println("Closing connection");
  trigger =0;
  value="";
  b=0;
  c="";
  tag="";
 }
  
}
