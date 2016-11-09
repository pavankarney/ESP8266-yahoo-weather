

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = "iPK";
const char* password = "qwru2367m";

const char* host = "query.yahooapis.com";
const char* url = "/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20u%3D'c'%20and%20woeid%20in%20(select%20woeid%20from%20geo.places(1)%20where%20text%3D%22New%20Delhi%2C%20India%22)&format=json";
const int httpsPort = 443;

// Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate

const char* fingerprint1 = "FB 67 CC A4 6B 33 40 35 F0 05 0D 20 1B BB E3 08 A0 0F B9 5A";
const char* fingerprint2 = "D6 A3 11 FA 78 10 0E 05 12 B5 08 53 9A 00 42 20 C9 9D B0 69";

const unsigned long BAUD_RATE = 115200;                 // serial connection speed
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from host
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response
String json;

// The type of data that we want to extract from the page
struct UserData {
  char created[32];
  char humidity[6];
  char pressure[12];
  char rising[2];
  char visibility[6];
  char sunrise[12];
  char sunset[12];
  char date[64];
  char temp[4];
  char text[32];
  //char description[64];
};
const char *description;
// ARDUINO entry point #1: runs once when you press reset or power the board
void setup() {
  initSerial();
  initWiFi();
}

// ARDUINO entry point #2: runs over and over again forever
void loop() {

  if (connect(host)) {
    if (sendRequest(host, url) && skipResponseHeaders()) {

      readReponsejson(json);

      // Length (with one extra character for the null terminator)
      int str_len = json.length() + 1;

      // Prepare the character array (the buffer)
      char response[str_len];

      // Copy it over
      json.toCharArray(response, str_len);

      //Serial.println(str_len);
      // Serial.println(response);

      UserData userData;
      if (parseUserData(response, &userData)) {
        printUserData(&userData);
      }
    }
    disconnect();
  }
  wait();
}

// Initialize Serial port
void initSerial() {
  Serial.begin(BAUD_RATE);
  while (!Serial) {
    ;  // wait for serial port to initialize
  }
  Serial.println("Serial ready");
}

// Initialize Ethernet library
void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(500);
}

// Wifi security check
void securitycheck()
{
  delay(500);
  if (client.verify(fingerprint1, host)) {
    Serial.println("fingerprint1 certificate matches");
  } else {
    if (client.verify(fingerprint2, host)) {
      Serial.println("fingerprint2 certificate matches");
    } else {
      Serial.println(" fingerprint2 certificate doesn't match & ");
    }
    Serial.println("fingerprint1 certificate doesn't match");
  }

}

// Open connection to the HTTP host
bool connect(const char* host) {
  Serial.print("Connect to ");
  Serial.println(host);

  bool ok = client.connect(host, httpsPort);

  Serial.println(ok ? "Connected" : "Connection Failed!");

  if (ok) {
    securitycheck();
  }

  return ok;
}

// Send the HTTP GET request to the host
bool sendRequest(const char* host, const char* url) {
  // Serial.print("GET ");
  //Serial.println(url);

  client.print("GET ");
  client.print(url);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();

  return true;
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);
  String line = client.readStringUntil('\n');
  Serial.print("skipping last unwanted line: [");
  Serial.print(line);
  Serial.println("].");

  if (!ok) {
    Serial.println("No response or invalid response!");
  }

  return ok;
}

// Read the body of the json from the HTTP host
void readReponsejson(String content) {
  content = client.readStringUntil('\n');
  json = content;
}



bool parseUserData(char* content, struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.

  // Allocate a temporary memory pool on the stack
  //StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  // If the memory pool is too big for the stack, use this instead:
  DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  strcpy(userData->created, root["query"]["created"]);
  strcpy(userData->humidity, root["query"]["results"]["channel"]["atmosphere"]["humidity"]);
  //strcpy(userData->pressure, root["query"]["results"]["channel"]["atmosphere"]["pressure"]);
  //strcpy(userData->rising, root["query"]["results"]["channel"]["atmosphere"]["rising"]);
  //strcpy(userData->visibility, root["query"]["results"]["channel"]["atmosphere"]["visibility"]);
  strcpy(userData->sunrise, root["query"]["results"]["channel"]["astronomy"]["sunrise"]);
  strcpy(userData->sunset, root["query"]["results"]["channel"]["astronomy"]["sunset"]);
  strcpy(userData->date, root["query"]["results"]["channel"]["item"]["condition"]["date"]);
  strcpy(userData->temp, root["query"]["results"]["channel"]["item"]["condition"]["temp"]);
  strcpy(userData->text, root["query"]["results"]["channel"]["item"]["condition"]["text"]);

  description = root["query"]["results"]["channel"]["item"]["description"];
  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
}

// Print the data extracted from the JSON
void printUserData(const struct UserData* userData) {
  Serial.print("Weather updated on ");

  int hourt = (String(userData->created[11]) + String(userData->created[12])).toInt();
  int mint = (String(userData->created[14]) + String(userData->created[15])).toInt();
  int sect = (String(userData->created[17]) + String(userData->created[18])).toInt();
  char Datet[12];
  strncpy(Datet, userData->created, 10);
  Serial.print(Datet);
  // adjusting to India IST time by adding 5 hours 30 min
  if ((mint + 30) > 59) {
    mint = (mint + 30) - 60;
    hourt += 6;
  } else if ((mint + 30) <= 59) {
    mint = (mint + 30);
    hourt += 5;
  }
  char msg[50];
  snprintf (msg, 50, " at New Delhi, India -  Time: # %02d:%02d:%02d IST", hourt, mint, sect);
  Serial.println(msg);

  Serial.print("humidity = ");
  Serial.println(userData->humidity);
  //Serial.print("pressure = ");
  //Serial.println(userData->pressure);
  //Serial.print("rising = ");
  //Serial.println(userData->rising);
  //Serial.print("visibility = ");
  //Serial.println(userData->visibility);
  Serial.print("sunrise = ");
  Serial.println(userData->sunrise);
  Serial.print("sunset = ");
  Serial.println(userData->sunset);
  Serial.println("Weather Now: ");
  Serial.print("Date = ");
  Serial.println(userData->date);
  Serial.print("Temperature = ");
  Serial.print(userData->temp);
  Serial.println(" C");
  Serial.print("Condition = ");
  Serial.println(userData->text);
  Serial.print("description = ");
  Serial.println(description);
}

// Close the connection with the HTTP host
void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}

// Pause for a 1 minute
void wait() {
  Serial.println("Wait 60 seconds");
  delay(60000);
}
