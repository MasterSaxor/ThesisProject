#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define RX 02
#define TX 03
SoftwareSerial RFID(RX, TX); // RX and TX

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

const int BUFFER_SIZE = 13;       // RFID DATA FRAME FORMAT: 1byte head (AA=170), 10byte data (3byte fixed + 2byte country + 5byte tag), 1byte checksum, 1byte tail (BB=187)
const int DATA_SIZE = 10;         // 3 byte fixed (0F 08 00) + 7byte data (2byte country + 5byte tag) (03 84 + 12 DB FA E7 D5)
const int DATA_FIXED_SIZE = 3;    // 3byte fixed (0F 08 00)
const int DATA_COUNTRY_SIZE = 2;  // 2byte country (example 03 84)
const int DATA_TAG_SIZE = 5;      // 5byte tag (example 12 DB FA E7 D5)
const int CHECKSUM_SIZE = 1;      // 1byte checksum (example 81)

uint8_t buffer[BUFFER_SIZE]; // used to store an incoming data frame

int buffer_index = 0;

const char* ssid     = "pixel";
const char* password = "smartbro@2022";

const char* host = "137.184.250.237";
const char* streamId   = "....................";
const char* privateKey = "....................";
byte i;
String value = "";
int b = 0;
String c = "";
//String tag = "";
char dataStr[12];
void setup() {
  Serial.begin(9600);
  delay(10);
  Serial.println();
  RFID.begin(9600); // start serial to RFID reader
  u8g2.begin();
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

  if (RFID.available() > 0)
  {

    bool call_extract_tag = false;

    int ssvalue = RFID.read(); // lees data

    if (ssvalue == -1) {         // no data was read
      return;
    }

    if (ssvalue == 170 && buffer_index == 0) {           // EM4305RFID found a tag => tag incoming
      //      buffer_index = 0;

    } else if (ssvalue == 187 && buffer_index == BUFFER_SIZE - 1) {    // tag has been fully transmitted
      call_extract_tag = true;      // extract tag at the end of the function call
    }

    if (buffer_index >= BUFFER_SIZE) {  // checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
      Serial.println("Error: Buffer overflow detected!");

      buffer_index = 0;
      return;
    }

    buffer[buffer_index++] = ssvalue; // everything is alright => copy current value to buffer
    Serial.print(ssvalue);
    Serial.print(" ");

    if (call_extract_tag == true) {
      if (buffer_index == BUFFER_SIZE) {
        extract_tag();

        buffer_index = 0;

        //Sending Data
      } else { // something is wrong... start again looking for preamble (value: 170)
        buffer_index = 0;
        return;
      }
    }

  }


}


void extract_tag() {          // analiseer data

  uint8_t msg_checksum = buffer[11]; // 1 byte
  uint8_t msg_data[DATA_SIZE];
  uint8_t msg_data_country[DATA_COUNTRY_SIZE];
  uint8_t msg_data_tag[DATA_TAG_SIZE];
  char countrymessage[2 * DATA_COUNTRY_SIZE]; // used to store ASCII code of hex data of country part
  char tagmessage[2 * DATA_TAG_SIZE]; // used tot store ASCII code of hex data of tag part
  long checksum = 0;
  long country = 0;
  long long tag = 0;
  long s;                 // second half of tag to show and determine leading 0
  int expo;               //exponent counter

  // test chip numbers, comment out if not testing
  // uint8_t buffer[BUFFER_SIZE] = {170, 15, 8, 0, 2, 16, 52, 39, 128, 22, 6, 150, 187};  // test known chip SKIP 528 224001005062
  // uint8_t buffer[BUFFER_SIZE] = {170, 15, 8, 0, 2, 16, 0, 29, 210, 159, 249, 0, 187};  // test chip 0, 29, 210, 159, 249 = 00 1D D2 9F F9 or DEC 500342777  (checksum not right)
  // uint8_t buffer[BUFFER_SIZE] = {170, 15, 8, 0, 2, 16, 0, 29, 252, 190, 25, 0, 187};  // 0, 29, 252, 190, 25 =  00 1D FC BE 19 or dec 503103001
  // end testing

  Serial.println("");
  Serial.print("Buffer (DEC): ");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(buffer[i], DEC);
    Serial.print(" ");
  }

  Serial.println("");
  Serial.print("Buffer (HEX): ");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  Serial.print("Buffer DATA (HEX): ");
  for (int i = 0; i < DATA_SIZE; i++) {
    msg_data[i] = buffer[i + 1];
    Serial.print(msg_data[i], HEX);
    checksum ^= msg_data[i];              // a = a XOR b
  }
  Serial.println("");

  Serial.print("Extracted Checksum (HEX): ");
  Serial.print(checksum, HEX);

  if (checksum == msg_checksum) { // compare calculated checksum to retrieved checksum
    Serial.print(" (OK)"); // calculated checksum corresponds to transmitted checksum!
  } else {
    Serial.print(" (NOT OK)"); // checksums do not match
  }
  Serial.println("");

  Serial.print("Buffer COUNTRY (HEX): ");
  for (int i = 0; i < DATA_COUNTRY_SIZE; i++) {
    msg_data_country[i] = buffer[i + 4];
    Serial.print(msg_data_country[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  // put buffer country in reverse into country message
  for (int i = 0; i < DATA_COUNTRY_SIZE ; i++) {
    String str = String(buffer[i + 4], HEX);
    if (str.length() == 1 && buffer[i + 6] - 48 < 10) {  // add leading 0 if < 10
      str = "0" + str;
    }
    str.toUpperCase();
    for (int k = 0; k < str.length(); k++) {
      byte x = str.charAt(k);
      countrymessage[2 * DATA_COUNTRY_SIZE - 1 - i * 2 - k] = x;
    }
  }

  country = hexInDec(countrymessage, 0, 2 * DATA_COUNTRY_SIZE);

  Serial.print("Country (DEC): ");
  Serial.println(country);

  Serial.print("Buffer TAG (HEX): ");
  for (int i = 0; i < DATA_TAG_SIZE; i++) {
    msg_data_tag[i] = buffer[i + 6];
    Serial.print(msg_data_tag[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  // Buffer TAG in reverse to tagmessage
  for (int i = 0; i < DATA_TAG_SIZE ; i++) {
    String str = String(buffer[i + 6], HEX);
    if (str.length() == 1 && buffer[i + 6] - 48 < 10) {  // add leading 0 if < 10
      str = "0" + str;
    }
    str.toUpperCase();
    for (int k = 0; k < str.length(); k++) {
      byte x = str.charAt(k);
      tagmessage[2 * DATA_TAG_SIZE - 1 - i * 2 - k] = x;
    }
  }

  tag = hexInDec(tagmessage, 0, 2 * DATA_TAG_SIZE);

  Serial.print("Tag (DEC): ");
  Serial.print(long(tag / 1000000));
  Serial.print(" ");
  //Serial.println(long(tag % 1000000));  // leading 0 are not printed
  s = long(tag % 1000000);
  for (long i = 100000; s < i && i > 1; i = i / 10) {   // print leading 0
    Serial.print("0");
  }
  Serial.println(s);

  Serial.println("--------");


  delay(5000);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request

  String url = "/iot/";
  int y = (long long) tag / 1000000;
  url += "insertdata.php?tag=";
  url += String(country) + String(y) + String(s);
  //url += String(country);
  Serial.println("Sending data to web.");


  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(1000);

  // Read all the lines of the reply from server and print them to Serial
  //int x = 0;
  while (client.available()) {
    char line = client.read();
    //String line = client.readStringUntil('\r');
    Serial.print(line);
    //if(String(line) == "!"){
    //  Serial.println("Record Exist!");
    //}
    //if(String(line) == "~"){
    //  Serial.println("Record Added!");
    //}

    //dataStr[x] = line;
    //x++;
  }
  // Fetch Data from web
  HTTPClient http;  //Declare an object of class HTTPClient
  String url1 = "http://www.rabbitfarm.tech/iot/getdata.php?tag=";
  url1 += String(country) + String(y) + String(s);
  http.begin(url1);  //Specify request destination
  int httpCode = http.GET();                                                                  //Send the request

  if (httpCode > 0) { //Check the returning code
    Serial.println("Connecting to Web...");
    String payload = http.getString();   //Get the request response payload
    Serial.println(payload);                     //Print the response payload
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
    //u8g2.drawStr(0, 8, "ID: 900215004080236"); // write something to the internal memory
    //u8g2.drawStr(0, 20, "Breed:Champange de argent"); // write something to the internal memory
    //u8g2.setFont(u8g2_font_trixel_square_tf);
    //u8g2.drawStr(0, 30, "D.O.B.: 08/18/2022"); // write something to the internal memory
    //u8g2.drawStr(68, 30, "Gender: Unknown"); // write something to the internal memory
    String readString; //main captured String
    String tid; //data String
    String breed;
    String dob;
    String gender;

    int ind1; // , locations
    int ind2;
    int ind3;
    int ind4;
    ind1 = payload.indexOf(",");  //finds location of first ,
    tid = payload.substring(0, ind1);   //captures first data String
    ind2 = payload.indexOf(",", ind1 + 1 );
    breed = payload.substring(ind1 + 1, ind2); //captures second data String
    ind3 = payload.indexOf(",", ind2 + 1 );
    dob = payload.substring(ind2 + 1, ind3 );
    ind4 = payload.indexOf(",", ind3 + 1 );
    gender = payload.substring(ind3 + 1, ind4); //captures remain part of data after last ,

    u8g2.drawStr(0, 8, tid.c_str());
    u8g2.drawStr(0, 20, breed.c_str());
    u8g2.setFont(u8g2_font_trixel_square_tf);
    u8g2.drawStr(0, 30, dob.c_str());
    u8g2.drawStr(66, 30, gender.c_str());
    u8g2.sendBuffer();         // transfer internal memory to the display
    delay(3000);
  }

  http.end();   //Close connection



  delay(1000);    //Send a request every 30 seconds


  Serial.println();
  Serial.println("Closing connection");
  trigger = 0;
  value = "";
  b = 0;
  c = "";
  //  tag="";

}

long long hexInDec(char message[], int beg , int len) {
  long long mult = 1;
  long long nbr = 0;
  byte nextInt;
  for (int i = beg; i < beg + len; i++) {
    nextInt = message[i];
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    nbr = nbr + (mult * nextInt);
    mult = mult * 16;
  }
  return nbr;
}
