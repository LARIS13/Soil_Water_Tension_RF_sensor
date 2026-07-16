#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Manchester.h>
#include <time.h>

const int Rx = 10000;  //fixed resistor attached in series to the sensor and ground...the same value repeated for all WM and Temp Sensor.
const long default_TempC = 20;
const long open_resistance = 35000; //check the open resistance value by replacing sensor with an open and replace the value here...this value might vary slightly with circuit components
const long short_resistance = 200; // similarly check short resistance by shorting the sensor terminals and replace the value here.
const long short_CB = 240, open_CB = 255 ;
const int SupplyV = 5; // Assuming 5V output for SupplyV, this can be measured and replaced with an exact value if required
const float cFactor = 1.1; //correction factor optional for adjusting curve, 1.1 recommended to match IRROMETER devices as well as CS CR1000

#define MY_TZ "CST6"
#ifndef STASSID
#define STASSID "Totalplay-76A6"
#define STAPSK "76A62E0DZBSSSGqk"
#endif

#define RX_PIN 4 //any pin can transmit
#define LED_PIN 2
#define BUFFER_SIZE 4
#define num_sens 4

uint8_t size;
uint8_t buffer[BUFFER_SIZE];

const char* ssid = STASSID;
const char* password = STAPSK;

unsigned int localPort = 123;

IPAddress timeServerIP;  // time.nist.gov NTP server address
const char* NTPServerName = "mx.pool.ntp.org";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

byte NTPBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP UDP;

//Secure client for google sheets connection
WiFiClientSecure secureClient;
//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;
//ID for google sheet
String GAS_ID = "AKfycbyZUmnkbehKTs4XBfrhy3ALrgaAvBeiw3Qt3fuprX7xDVVYYRtO6HuookU0MZy900m_mw";

ESP8266WebServer server(80);

unsigned long intervalNTP = 60000; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
unsigned long currentMillis = millis();
uint32_t timeUNIX = 0;
uint32_t actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;
unsigned long prevActualTime = 0;
time_t miTiempo = static_cast<time_t>(actualTime);
struct tm *timeinfo = localtime(&miTiempo);

tm* readtimes[num_sens];

//conversion of ohms to CB
int myCBvalue(int ARead_A1, int ARead_A2, float TC, float cF) {   //conversion of ohms to CB
  float SenVWM1 = ((ARead_A1 / 255.0f) * SupplyV); //get the average of the readings in the first direction and convert to volts
  float SenVWM2 = ((ARead_A2 / 255.0f) * SupplyV); //get the average of the readings in the second direction and convert to volts
  double WM_ResistanceA = (Rx * (SupplyV - SenVWM1) / SenVWM1); //do the voltage divider math, using the Rx variable representing the known resistor
  double WM_ResistanceB = Rx * SenVWM2 / (SupplyV - SenVWM2);  // reverse
  double res = ((WM_ResistanceA + WM_ResistanceB) / 2.0); //average the two directions
  Serial.println("Calculating ");
  Serial.println(res);
  int WM_CB;
  float resK = res / 1000.0;
  float tempD = 1.00 + 0.018 * (TC - 24.00);

  if (res > 550.00) { //if in the normal calibration range
    if (res > 8000.00) { //above 8k
      WM_CB = (-2.246 - 5.239 * resK * (1 + .018 * (TC - 24.00)) - .06756 * resK * resK * (tempD * tempD)) * cF;
    } else if (res > 1000.00) { //between 1k and 8k
      WM_CB = (-3.213 * resK - 4.093) / (1 - 0.009733 * resK - 0.01205 * (TC)) * cF ;
    } else { //below 1k
      WM_CB = -(resK * 23.156 - 12.736) * tempD;
    }
  } else { //below normal range but above short (new, unconditioned sensors)
    if (res > 300.00)  {
      WM_CB = 0.00;
    }
    if (res < 300.00 && res >= short_resistance) { //wire short
      WM_CB = short_CB; //240 is a fault code for sensor terminal short
      Serial.print("Sensor Short WM \n");
    }
  }
  if (res >= open_resistance || res==0) {

    WM_CB = open_CB; //255 is a fault code for open circuit or sensor not present

  }
  return WM_CB;
}

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP8266 Sensor Hub</title>
<style>
  :root {
    --bg:     #0d1117;
    --card:   #161b22;
    --border: #30363d;
    --accent: #58a6ff;
    --green:  #3fb950;
    --text:   #e6edf3;
    --muted:  #8b949e;
    --font:   'Courier New', monospace;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: var(--font);
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 24px;
  }
  header {
    text-align: center;
    margin-bottom: 32px;
  }
  header h1 {
    font-size: 1.1rem;
    letter-spacing: 0.2em;
    text-transform: uppercase;
    color: var(--accent);
  }
  header p {
    font-size: 0.7rem;
    color: var(--muted);
    margin-top: 6px;
    letter-spacing: 0.1em;
  }
  .grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    width: 100%;
    max-width: 480px;
  }
  @media (max-width: 360px) {
    .grid { grid-template-columns: 1fr; }
  }
  .card {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 20px 16px;
    position: relative;
    overflow: hidden;
  }
  .card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 2px;
    background: var(--green);
  }
  .card .label {
    font-size: 0.65rem;
    letter-spacing: 0.15em;
    color: var(--muted);
    margin-bottom: 12px;
  }
  .card .value {
    font-size: 2.4rem;
    font-weight: bold;
    color: var(--text);
    line-height: 1;
  }
  .card .unit {
    font-size: 1rem;
    color: var(--accent);
    margin-left: 4px;
  }
  .card .ts {
    font-size: 0.58rem;
    color: var(--muted);
    margin-top: 12px;
  }
  .card .ts span {
    color: var(--green);
  }
  footer {
    margin-top: 28px;
    font-size: 0.6rem;
    color: var(--muted);
    letter-spacing: 0.1em;
    text-align: center;
  }
</style>
</head>
<body>
<header>
  <h1>&#9632; Huerto La Pausa </h1>
  <p>ESP8266 &bull; %IP%</p>
</header>

<div class="grid">

  <div class="card">
    <div class="label">Sensor: %S1_LABEL%</div>
    <div class="value">%S1_VALUE%<span class="unit">%S1_UNIT%</span></div>
    <div class="ts">Updated:<br><span>%S1_TS%</span></div>
  </div>

  <div class="card">
    <div class="label">%S2_LABEL%</div>
    <div class="value">%S2_VALUE%<span class="unit">%S2_UNIT%</span></div>
    <div class="ts">Updated:<br><span>%S2_TS%</span></div>
  </div>

</div>

<footer>%UPTIME%s desde ultimo incidente</footer>
<footer>Para ver datos pasados da click aqui: <a href="https://docs.google.com/spreadsheets/d/1KwV3gNRVEFmph4-SC8QsB9Q-XX5qjFLy4nvLQSTLsgs/edit?gid=0#gid=0">Google Sheets</a></footer>
</body>
</html>
)rawliteral";

// ─── Build page: replace placeholders ────────────────────────────────────────
String buildPage() {
  String page = FPSTR(HTML_PAGE);

  //if buffer[3] == 0 
  page.replace("%S1_LABEL%", String(buffer[3]));
  if (myCBvalue(buffer[1],buffer[2],default_TempC, cFactor) == 255) {
    page.replace("%S1_VALUE%", "<span style=\"color:red;\">" + String(myCBvalue(buffer[1],buffer[2],default_TempC, cFactor)) + "</span>");
  } else {
    page.replace("%S1_VALUE%", String(myCBvalue(buffer[1],buffer[2],default_TempC, cFactor)));
  }
  page.replace("%S1_UNIT%",  "kPa");
  page.replace("%S1_TS%", String(readtimes[0]->tm_mday) + "/" + String(readtimes[0]->tm_mon + 1) + "/" + String(readtimes[0]->tm_year + 1900) + " " +String(readtimes[0]->tm_hour) + ":" + String(readtimes[0]->tm_min) + ":" + String(readtimes[0]->tm_sec));
  //elif buffer[3] == 1 ALO MEJOR hacerlo en loop para poder ense;ar los dos
  page.replace("%S2_LABEL%", "TBD");
  page.replace("%S2_VALUE%", "TBD");
  page.replace("%S2_UNIT%",  "TBD");
  page.replace("%S2_TS%",   "TBD");

  page.replace("%IP%",     WiFi.localIP().toString());
  page.replace("%UPTIME%", String(millis() / 1000));

  return page;
}

void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
}

uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  NTPBuffer[1] = 0;           // Stratum, or type of clock
  NTPBuffer[2] = 6;           // Polling Interval
  NTPBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  NTPBuffer[12] = 49;
  NTPBuffer[13] = 0x4E;
  NTPBuffer[14] = 49;
  NTPBuffer[15] = 52;
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime) {
  return ((UNIXTime / 3600) -6) % 24; //CDMX Time
}

inline int getDay(uint32_t UNIXTime) {
  return UNIXTime / 86400;
}

// Subroutine for sending data to Google Sheets
void sendData(uint8_t ID, int cb_val, struct tm *time) {
 Serial.println("==========");
 Serial.print("connecting to ");
 Serial.println(host); 
 //----------------------------------------Connect to Google host
 secureClient.setInsecure(); 
 if (!secureClient.connect(host, httpsPort)) {
 Serial.println("connection failed");
 return;
 }
 //----------------------------------------

 //----------------------------------------Processing data and sending data
 String string_cb = String(cb_val);
 // String string_temperature = String(tem, DEC); 
 String string_id = String(ID, DEC); 
 String url = "/macros/s/" + GAS_ID + "/exec?Sensor=" + string_id + "&Tension=" + string_cb + "&Hora=" + String(readtimes[0]->tm_mday) + "%2F" + String(readtimes[0]->tm_mon + 1) + "%2F" + String(readtimes[0]->tm_year + 1900) + "-" +String(readtimes[0]->tm_hour) + "%3A" + String(readtimes[0]->tm_min) + "%3A" + String(readtimes[0]->tm_sec);
 Serial.print("requesting URL: ");
 Serial.println(url);

 secureClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
 "Host: " + host + "\r\n" +
 "User-Agent: BuildFailureDetectorESP32 \r\n" +
 "Connection: close\r\n\r\n");

 Serial.println("request sent");
 //----------------------------------------

 //----------------------------------------Checking whether the data was sent successfully or not
 while (secureClient.connected()) {
 String line = secureClient.readStringUntil('\n');
 if (line == "\r") {
 Serial.println("headers received");
 break;
 }
 }
 String line = secureClient.readStringUntil('\n'); //cambiar esta linea para debugging a veces te dice error moved temporarily pero de todas sirve.
 if (line.startsWith("{\"state\":\"success\"")) {
 Serial.println("ESP32 /Arduino CI successfull!");
 } else {
 Serial.println("ESP32 /Arduino CI has failed");
 }
 Serial.print("reply was : ");
 Serial.println(line);
 Serial.println("closing connection");
 Serial.println("==========");
 Serial.println();
}

void setup(void) {
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  UDP.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(UDP.localPort());

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  
  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);

  if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

  // Set the global Time Zone environment variable
  setenv("TZ", MY_TZ, 1);
  tzset(); // Force the system to update and apply the new TZ settings
  Serial.println("Timezone set");

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  /////////////////////////////////////////////////////////
  // Hook examples

  server.addHook([](const String& method, const String& url, WiFiClient* client, ESP8266WebServer::ContentTypeFunction contentType) {
    (void)method;       // GET, PUT, ...
    (void)url;          // example: /root/myfile.html
    (void)client;       // the webserver tcp client connection
    (void)contentType;  // contentType(".html") => "text/html"
    Serial.printf("A useless web hook has passed\n");
    Serial.printf("(this hook is in 0x%08x area (401x=IRAM 402x=FLASH))\n", esp_get_program_counter());
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String& url, WiFiClient*, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/fail")) {
      Serial.printf("An always failing web hook has been triggered\n");
      return ESP8266WebServer::CLIENT_MUST_STOP;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String& url, WiFiClient* client, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/dump")) {
      Serial.printf("The dumper web hook is on the run\n");

      // Here the request is not interpreted, so we cannot for sure
      // swallow the exact amount matching the full request+content,
      // hence the tcp connection cannot be handled anymore by the
      // webserver.
#ifdef STREAMSEND_API
      // we are lucky
      client->sendAll(Serial, 500);
#else
      auto last = millis();
      while ((millis() - last) < 500) {
        char buf[32];
        size_t len = client->read((uint8_t*)buf, sizeof(buf));
        if (len > 0) {
          Serial.printf("(<%d> chars)", (int)len);
          Serial.write(buf, len);
          last = millis();
        }
      }
#endif
      // Two choices: return MUST STOP and webserver will close it
      //                       (we already have the example with '/fail' hook)
      // or                  IS GIVEN and webserver will forget it
      // trying with IS GIVEN and storing it on a dumb WiFiClient.
      // check the client connection: it should not immediately be closed
      // (make another '/dump' one to close the first)
      Serial.printf("\nTelling server to forget this connection\n");
      static WiFiClient forgetme = *client;  // stop previous one if present and transfer client refcounter
      return ESP8266WebServer::CLIENT_IS_GIVEN;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  // Hook examples
  /////////////////////////////////////////////////////////

  server.begin();
  Serial.println("HTTP server started");

  man.setupReceive(RX_PIN, MAN_4800);
  man.beginReceiveArray(BUFFER_SIZE, buffer);
  Serial.println("Receiver started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();

  currentMillis = millis();
  //  unsigned long currentMillis = millis();

  if (currentMillis - prevNTP > intervalNTP) { // If a minute has passed since last NTP request
    prevNTP = currentMillis;
    Serial.println("\r\nSending NTP request ...");
    sendNTPpacket(timeServerIP);               // Send an NTP request
  }

  if (man.receiveComplete()) { //something is in RX buffer
    Serial.println("Recieved TX:");
    Serial.println(buffer[3]);
    Serial.println("Recieved A1:");
    Serial.println(buffer[1]);
    Serial.println("Recieved A2:");
    Serial.println(buffer[2]);
    digitalWrite(LED_PIN, HIGH); //blink the LED on receive
    man.beginReceiveArray(BUFFER_SIZE, buffer);

    uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
    if (time) {                                  // If a new timestamp has been received
      timeUNIX = time;
      Serial.println("NTP response:");
      Serial.println(timeUNIX);
      lastNTPResponse = currentMillis;
    } else if ((currentMillis - lastNTPResponse) > 3600000) {
      Serial.println("More than 1 hour since last NTP response. Rebooting.");
      Serial.flush();
      ESP.reset();
    }

    actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;

    miTiempo = static_cast<time_t>(actualTime);
    tm *timeinfo = localtime(&miTiempo);
    if (actualTime != prevActualTime && timeUNIX != 0) { // If a second has passed since last print ------ NEED CHANGING
      prevActualTime = actualTime;
      Serial.printf("Local Time: %02d/%02d/%02d %02d:%02d:%02d\n", 
                timeinfo->tm_mday,
                timeinfo->tm_mon + 1,
                timeinfo->tm_year + 1900,            
                timeinfo->tm_hour, 
                timeinfo->tm_min, 
                timeinfo->tm_sec);
    }

    readtimes[buffer[3]] = timeinfo;
    digitalWrite(LED_PIN, LOW);

    sendData(buffer[3], myCBvalue(buffer[1],buffer[2],default_TempC, cFactor), readtimes[buffer[3]]); //send to google sheets
  }

}
