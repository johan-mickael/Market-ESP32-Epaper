#define LILYGO_T5_V213
#define HIGH_RESISTOR 100000
#define LOW_RESISTOR  100000
#define VOLT_PIN      35

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300        /* Time ESP32 will go to sleep (in seconds) */

#include <boards.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <U8g2_for_Adafruit_GFX.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <WiFi.h>
const char* SSID = "msi";
const char* PASSWORD = "12345678";

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "logo.h"
 
GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

RTC_DATA_ATTR uint16_t compteur = 0; // Stocké en mémoire de la RTC

DynamicJsonDocument products(1024);

#include "ESPAsyncWebServer.h"
AsyncWebServer server(80);

void setup(void);
void loop();
void initAdafruitGfxOptions();
void setThemeToDark(bool dark);
void displayLogo();
bool connectToWifi();
void subscribeToRoutes();
DynamicJsonDocument fetchData(String id);
bool printItem(DynamicJsonDocument doc);
void setTemplate(DynamicJsonDocument doc);
void printMessage(String message);
void printProductName(DynamicJsonDocument doc);
void printProductPrice(DynamicJsonDocument doc);
void printProductWeight(DynamicJsonDocument doc);
void printProductDate(DynamicJsonDocument doc);
void printProductPromotion(DynamicJsonDocument doc);

boolean exec_fetch = false;

String product_id = "";

void setup(void) {
  Serial.begin(115200);
  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
  initAdafruitGfxOptions();
  displayLogo();
  connectToWifi();
  subscribeToRoutes();
}

void loop()
{
  if(exec_fetch) {
    exec_fetch = false;
    products = fetchData(product_id);
    printItem(products);
  }
}

void initAdafruitGfxOptions() {
  display.init(); // enable diagnostic output on Serial
  u8g2Fonts.begin(display);
  u8g2Fonts.setFontMode(1);                           // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(1);                      // left to right (this is default)

}

void setThemeToDark(bool dark) {
  if(dark) {
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);          // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);            // apply Adafruit GFX color
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);          // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }
    
}

void displayLogo() {
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(gImage_logo, 0, 0, display.width(), display.height(), GxEPD_BLACK);
  display.update();
}

bool connectToWifi() {
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to wifi");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected");
  return true;
}

void subscribeToRoutes() {
  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request)    {
    request->send(200, "text/plain", WiFi.localIP().toString());
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    int paramsNr = request->params();
    request->send(200, "text/plain", "message received");
    AsyncWebParameter *p = request->getParam(0);
    product_id = p->value();
    exec_fetch=true;
  });
  server.begin();
}

DynamicJsonDocument fetchData(String id) {
  Serial.println("id:" + product_id);
  DynamicJsonDocument doc(1024);
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient client;
    String endpoint = "http://192.168.137.1:8000/product/" + id + "/json";
    Serial.println(endpoint);
    client.begin(endpoint);
    int httpCode = client.GET();
    Serial.println(httpCode);
    if(httpCode > 0) {
      deserializeJson(doc, client.getStream());
    } else {
      printMessage("Error !");
    }
    client.end();
  }
  return doc;
}

bool printItem(DynamicJsonDocument doc) {
      if(!doc.isNull()) {
        setTemplate(doc);
        return false;
      }
      return true;
        
    //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    //esp_deep_sleep_start();
}

void setTemplate(DynamicJsonDocument doc){
    display.fillScreen(GxEPD_WHITE);
    setThemeToDark(true);
    printProductName(doc);  
    setThemeToDark(false);
    printProductPromotion(doc);
    printProductPrice(doc);  
    printProductWeight(doc);
    printProductDate(doc);
    
    display.update();
}

void printMessage(String message) {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setFont(u8g2_font_luBS18_tf); 
    uint16_t x1 = display.width() - 3 * display.width() / 6;
    u8g2Fonts.setCursor(x1, display.width() -2 * display.width() / 4);                 
    u8g2Fonts.print(message);
    display.update();
}

void printProductName(DynamicJsonDocument doc) {
    display.fillRect((display.width() - display.width() / 3) + 10, 0, display.width() / 3, display.height(), GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_crox5hb_tr);   
    u8g2Fonts.setCursor(display.width() - display.width() / 6, 0);                        
    u8g2Fonts.print(doc["name"].as<String>());  
}

void printProductPrice(DynamicJsonDocument doc) {
    uint16_t blockSize = 0;
    u8g2Fonts.setFont(u8g2_font_fub42_tf);
    blockSize += u8g2Fonts.getUTF8Width(doc["price"]["integer"].as<char*>());
    u8g2Fonts.setFont(u8g2_font_fub20_tf);  
    blockSize += u8g2Fonts.getUTF8Width(doc["price"]["unit"].as<char*>());
    if(!doc["price"]["floating"].isNull()) {
      blockSize += u8g2Fonts.getUTF8Width(doc["price"]["floating"].as<char*>());
    }
    
    u8g2Fonts.setCursor(display.width() / 6 + 20, (display.height() - blockSize - 10));                          
    u8g2Fonts.setFont(u8g2_font_fub42_tf);
    u8g2Fonts.print(doc["price"]["integer"].as<String>());
    u8g2Fonts.setFont(u8g2_font_fub30_tf);  
    u8g2Fonts.print(doc["price"]["unit"].as<String>());
    if(!doc["price"]["floating"].isNull()) {
      u8g2Fonts.setFont(u8g2_font_fub20_tf); 
      u8g2Fonts.print(doc["price"]["floating"].as<String>());
    }
}

void printProductWeight(DynamicJsonDocument doc) {
    u8g2Fonts.setFont(u8g2_font_helvR14_tf);
    u8g2Fonts.setCursor((display.width() - display.width() / 6) - 30, 0);  
    u8g2Fonts.print(" " + doc["weight"].as<String>() + doc["unit_suffix"].as<String>());
}

void printProductDate(DynamicJsonDocument doc) {
    u8g2Fonts.setFont(u8g2_font_helvR14_tf); 
    u8g2Fonts.setCursor(0, 0);  
    u8g2Fonts.setFont(u8g2_font_7x13B_tf); 
    u8g2Fonts.print("expiration " + doc["date"].as<String>());
}

void printProductPromotion(DynamicJsonDocument doc) {
    u8g2Fonts.setFont(u8g2_font_sticker_mel_tr);
    u8g2Fonts.print("  "+doc["promotion"].as<String>());
}
