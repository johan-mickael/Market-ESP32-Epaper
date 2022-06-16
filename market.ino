// Choosing the model of the arduino card (required by the board module)
#define LILYGO_T5_V213

#include <boards.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <U8g2_for_Adafruit_GFX.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "logo.h"
#include "ESPAsyncWebServer.h"
#include "font_logisoso20_tx.h"


#define LILYGO_T5_V213

// Defining the graphics that we are about to use
GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

// using the U8g2 fonts library
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// The product printed on the screen will be stored here
StaticJsonDocument<2048> products;

// Esp32 server instance
AsyncWebServer server(80);

boolean exec_fetch = false;
String product_id = "";

// Wifi login identifiers
const char* SSID = "msi";
const char* PASSWORD = "12345678";

String apiBaseURL = "http://192.168.137.1:8000";

/*
All used methods references.
Add the signature of your methods here.
*/
void setup(void);
void loop();
void initAdafruitGfxOptions();
void setThemeToDark(bool dark);
void displayLogo();
bool connectToWifi();
void subscribeToRoutes();
StaticJsonDocument<2048> fetchData(String id);
bool printItem(StaticJsonDocument<2048> doc);
void setTemplate(StaticJsonDocument<2048> doc);
void printMessage(String message);
void printProductName(StaticJsonDocument<2048> doc);
void printProductPrice(StaticJsonDocument<2048> doc);
void printProductWeight(StaticJsonDocument<2048> doc);
void printProductPricePerUnit(StaticJsonDocument<2048> doc);
void printProductDate(StaticJsonDocument<2048> doc);
void printProductPromotion(StaticJsonDocument<2048> doc);
void printProductRemainingStock(StaticJsonDocument<2048> doc);
void printProductOldPrice(StaticJsonDocument<2048> doc);
char *stringToCharArray(String s);
void drawXLine(int stroke);
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int stroke);
void handleError(String message);

// On board init
void setup(void) {
  Serial.begin(115200);
  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);

  // Call our custom methods on the card initialization
  initAdafruitGfxOptions();
  displayLogo();
  connectToWifi();
  subscribeToRoutes();

  // Starting the card server on the 80 port
  server.begin();
}

// This method will execute indefinitely
void loop()
{
  // If the server detect an incoming request
  if(exec_fetch) {
    exec_fetch = false;                 // reseting the variable to the default value to avoid infinite fetch
    products = fetchData(product_id);   // updating the product data
    printItem(products);                // Printing the requested product in the card display
  }
}

// Add here all your GFX default options
void initAdafruitGfxOptions() {
  display.init();                  // enable diagnostic output on Serial
  u8g2Fonts.begin(display);        // initializing the font into our display
  u8g2Fonts.setFontMode(1);        // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(1);   // left to right (this is default)
}

/*
Changing the theme of the display.
if dark parameter is set to true we will have a high contrast (dark background / light background)
*/
void setThemeToDark(bool dark) {
  if(dark) {
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);          // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);          // apply Adafruit GFX color
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);          // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }  
}

// Displaying our custom logo on the screen
void displayLogo() {
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(gImage_logo, 0, 0, display.width(), display.height(), GxEPD_BLACK);  // Our custom logo array is stored inside <logo.h>
  display.update();
}

// Connecting the card to a WiFi network
bool connectToWifi() {
  WiFi.begin(SSID, PASSWORD);             
  Serial.print("Connecting to wifi");
  while(WiFi.status() != WL_CONNECTED) {  // Trying to connect to the WiFi
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected");          
  Serial.println(WiFi.localIP());
  return true;
}

// Listening to incoming request on the card server
void subscribeToRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    int paramsNr = request->params();                 // Getting the query parameters
    AsyncWebParameter *p = request->getParam(0);      // Getting the first query parameters (id)
    product_id = p->value();                          // updating the current id of the products
    exec_fetch=true;                                  // we can now fetch and update our product data
    request->send(200, "text/plain", "received !");   // sending a response
  });
}

// Finding the product by id
StaticJsonDocument<2048> fetchData(String id) {
  StaticJsonDocument<2048> doc;
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient client;
    String endpoint = apiBaseURL + "/product/" + id + "/json";
    client.begin(endpoint);
    int httpCode = client.GET();
    Serial.println(httpCode);
    if(httpCode > 0) {
      DeserializationError deserializationError = deserializeJson(doc, client.getStream());
      if(deserializationError) {
        handleError("Data error.");
      }
    } else {
      handleError("Data error.");
    }
    client.end();
  } else {
    handleError("Connexion error.");
  }
  return doc;
}

bool printItem(StaticJsonDocument<2048> doc) {
    if(!doc.isNull()) {
      setTemplate(doc);
      return false;
    }
    return true;
      
}

void setTemplate(StaticJsonDocument<2048> doc){
    display.fillScreen(GxEPD_WHITE);      // Clearing the screen (fill with white)
    setThemeToDark(true);
    printProductName(doc);  
    setThemeToDark(false);
    printProductPrice(doc);  
    if(doc["promotion"].as<bool>()) {     // Only print those informations if the product is a promotion
      printProductPromotion(doc);
      printProductOldPrice(doc);
    }
    printProductWeight(doc);
    printProductPricePerUnit(doc);
    printProductDate(doc);
    printProductRemainingStock(doc);
    display.update();                     // refresh the screen
}

// Displaying a custom message on the center of the screen
void printMessage(String message) {
    display.fillScreen(GxEPD_WHITE);
    setThemeToDark(true);
    u8g2Fonts.setFont(u8g2_font_luBS18_tf); 
    uint16_t blockSize = u8g2Fonts.getUTF8Width(stringToCharArray(message));      // Calculate the width of the text message
    u8g2Fonts.setCursor(display.width() / 2, display.height() - (blockSize/2));   // Displaying the message in the center of the screen              
    u8g2Fonts.print(message);
    display.update();
    setThemeToDark(false);
}

void printProductName(StaticJsonDocument<2048> doc) {
    display.fillRect((display.width() - display.width() / 3) + 10, 0, display.width() / 3, display.height(), GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_crox5hb_tr);   
    u8g2Fonts.setCursor((display.width() - display.width() / 6) - 3, 2);                        
    u8g2Fonts.print(doc["name"].as<String>());  
}

void printProductPrice(StaticJsonDocument<2048> doc) {
    /*
      Here, we want to put the price on the right on the screen se WE NEED TO
      Calculate the dynamic coordonates of the cursor to start writing data on the screen
    */
    uint16_t blockSize = 0;
    u8g2Fonts.setFont(u8g2_font_fub42_tf);
    uint16_t integerBlockSize = u8g2Fonts.getUTF8Width(doc["price"]["integer"].as<char*>());
    blockSize += integerBlockSize;
    u8g2Fonts.setFont(u8g2_font_fub20_tf);  
    uint16_t floatingBlockSize = u8g2Fonts.getUTF8Width(doc["price"]["floating"].as<char*>());
    blockSize += floatingBlockSize;
    // End of coordinates calculation
    
    // Displaying the price on the screen
    u8g2Fonts.setCursor(display.width() / 6 + 20, (display.height() - blockSize - 3));                          
    u8g2Fonts.setFont(u8g2_font_fub42_tf);
    u8g2Fonts.print(doc["price"]["integer"].as<String>());
    u8g2Fonts.setFont(u8g2_font_fub20_tf);  
    u8g2Fonts.print(doc["price"]["floating"].as<String>());
    u8g2Fonts.setCursor(display.width() / 6 + 45, (display.height() - blockSize - 3 + integerBlockSize + 2));  
    u8g2Fonts.setFont(u8g2_font_logisoso20_tx);  
    u8g2Fonts.print(doc["price"]["unit"].as<String>());
}

void printProductOldPrice(StaticJsonDocument<2048> doc) {
    /*
      Here, we want to put the price on the right on the screen se WE NEED TO
      Calculate the dynamic coordonates of the cursor to start writing data on the screen
    */
    uint16_t blockSize = 0;
    u8g2Fonts.setFont(u8g2_font_samim_16_t_all );
    uint16_t integerBlockSize = u8g2Fonts.getUTF8Width(doc["old_price"]["integer"].as<char*>());
    blockSize += integerBlockSize;
    u8g2Fonts.setFont(u8g2_font_samim_12_t_all);  
    blockSize += u8g2Fonts.getUTF8Width(doc["old_price"]["unit"].as<char*>());
    uint16_t floatingBlockSize = u8g2Fonts.getUTF8Width(doc["old_price"]["floating"].as<char*>());
    blockSize += floatingBlockSize;
    // End of coordinates calculation
    
    // Displaying the price on the screen
    u8g2Fonts.setCursor(20, (display.height() - blockSize - 3));                          
    u8g2Fonts.setFont(u8g2_font_samim_16_t_all);
    u8g2Fonts.print(doc["old_price"]["integer"].as<String>());
    u8g2Fonts.setFont(u8g2_font_samim_12_t_all);  
    u8g2Fonts.print(doc["old_price"]["unit"].as<String>());
    u8g2Fonts.print(doc["old_price"]["floating"].as<String>());
    drawLine(25, display.height() - blockSize - 7, 25, display.height(), 1);
}

void printProductWeight(StaticJsonDocument<2048> doc) {
    u8g2Fonts.setFont(u8g2_font_helvR14_tf);
    u8g2Fonts.setCursor((display.width() - display.width() / 6) - 30, 0);  
    u8g2Fonts.print(doc["weight"].as<String>() + doc["unit_suffix"].as<String>());
}

void printProductPricePerUnit(StaticJsonDocument<2048> doc) {
    u8g2Fonts.setFont(u8g2_font_nine_by_five_nbp_t_all);
    String pricePerUnitStr = doc["price_per_unit"].as<String>() + doc["price"]["unit"].as<String>(); 
    uint16_t blockSize = u8g2Fonts.getUTF8Width(stringToCharArray(pricePerUnitStr));
    u8g2Fonts.setCursor((display.width() - display.width() / 6) - 47, 0);  
    u8g2Fonts.print(doc["price_per_unit"].as<String>());
    u8g2Fonts.print(doc["price"]["unit"].as<String>());
    u8g2Fonts.setCursor((display.width() - display.width() / 6) - 52 , blockSize); 
    u8g2Fonts.println("/" + doc["unit_suffix"].as<String>());
}

void printProductRemainingStock(StaticJsonDocument<2048> doc) {
    int remainingStock = doc["stock"].as<int>();
    if(remainingStock > 0) {
      u8g2Fonts.setCursor(17, 0);  
      u8g2Fonts.setFont(u8g2_font_7x13B_tf); 
      u8g2Fonts.print(doc["stock"].as<String>() + " restant");
      if(remainingStock > 1) {
        u8g2Fonts.print("s"); // If there are several products stock, display <restants> else display <restant>. Logic right ? just french orthographic rules ...
      }
    } else {
      drawXLine(8);           // Draw a cross on the screen to tell the users that there is no remaining product on the market
    }     
}

void printProductDate(StaticJsonDocument<2048> doc) {
    u8g2Fonts.setCursor(3, 0);  
    u8g2Fonts.setFont(u8g2_font_7x13B_tf); 
    u8g2Fonts.print("expire le " + doc["date"].as<String>());
}

void printProductPromotion(StaticJsonDocument<2048> doc) {
    u8g2Fonts.setCursor(33, 0);
    u8g2Fonts.setFont(u8g2_font_sticker_mel_tr);
    u8g2Fonts.print("promotion");
}

// Just an helper method to convert a String to an array of character
char *stringToCharArray(String s) {
    int n = s.length();
    char *char_array = new char[n + 1];
    for(int i=0; i<n; i++) {
      char_array[i] = s[i];
    }
    char_array[n] = NULL;       
    return char_array;
}

// Draw a cross 
void drawXLine(int stroke) {
    drawLine(display.width(), 0, 0, display.height(), stroke);
    drawLine(0, 0, display.width(), display.height(), stroke);
}

// Draw line with a given strokes
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int stroke) {
  for(int i=0; i<stroke; i++) {
    display.drawLine(x0 -i, y0, x1 -i, y1, GxEPD_BLACK);
  }
}


// Display message on the screen and the monitor
void handleError(String message) {
  printMessage(message);
  Serial.println(message);
}
