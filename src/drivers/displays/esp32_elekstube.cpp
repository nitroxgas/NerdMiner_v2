#include "DisplayDriver.h"

#ifdef ESP32_ELEKSTUBE

#include <TFT_eSPI.h>
#include "esp32_elekstube/ChipSelect.h"
#include "media/images_320_170.h"
#include "media/images_bottom_320_70.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include "wManager.h"

#define WIDTH 135 //320
#define HEIGHT 160 

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in platformio.ini
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite
ChipSelect chip_select;
Adafruit_NeoPixel pixels(6, BACKLIGHTS_PIN, NEO_GRB + NEO_KHZ800);

extern monitor_data mMonitor;
extern pool_data pData;
extern DisplayDriver *currentDisplayDriver;

bool hasChangedScreen = true;
bool doLed = true;
int timeScr = 0;

OneButton pButton(BUTTON_POWER_PIN);
OneButton mButton(BUTTON_MODE_PIN);
OneButton lButton(BUTTON_LEFT_PIN);
OneButton rButton(BUTTON_RIGHT_PIN);

void printheap(){
  Serial.print(F("============ Free Heap:"));
  Serial.println(ESP.getFreeHeap()); 
}

void createBackgroundSprite(int16_t wdt, int16_t hgt){  // Set the background and link the render, used multiple times to fit in heap
  background.createSprite(wdt, hgt) ; //Background Sprite
 // printheap();
  background.setColorDepth(16);
  background.setSwapBytes(true);
  render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9);
}


void esp32_elekstube_AlternateScreenState(void)
{
  chip_select.setAll();
  int screen_state = digitalRead(TFT_BL);
  Serial.println(F("Switching display state"));
  digitalWrite(TFT_BL, !screen_state);
  chip_select.update();
}

void switchToNextScreenInt()
{
  currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
}

void switchToPrevScreen()
{
  currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->current_cyclic_screen - 1;      
  if (currentDisplayDriver->current_cyclic_screen<0) currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->num_cyclic_screens - 1;
}

void changeLedState(){
  doLed ^= true;
  pixels.clear();
  pixels.show(); 
}

void changeTimer(){
  timeScr = timeScr + 5;
  if (timeScr>30) timeScr = 0;
  char text[10] = "";
  sprintf(text,"Timer %dm",timeScr);
  chip_select.setDigit(2);
  tft.fillRect(0,200,135,30,TFT_YELLOW);
  tft.setFreeFont(FSSB12);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString(text, 67, 205, GFXFF);
}

void esp32_elekstube_Init(void)
{ 
  pButton.setPressTicks(5000);
  pButton.attachClick(esp32_elekstube_AlternateScreenState);  
  pButton.attachLongPressStart(reset_configuration);
  //pButton.attachMultiClick(alternateScreenState);

  rButton.setPressTicks(5000);
  rButton.attachClick(switchToNextScreenInt);

  lButton.setPressTicks(5000);
  lButton.attachClick(switchToPrevScreen); 

  mButton.setPressTicks(2000);
  mButton.attachClick(changeTimer);
  mButton.attachDoubleClick(changeLedState);

  chip_select.begin();
  chip_select.setAll();
  digitalWrite(TFT_ENABLE_PIN, HIGH);
  
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true); // Swap the colour byte order when rendering
  /* tft.fillScreen(TFT_BLACK);
  chip_select.setDigit(5); */
  
 /*  createBackgroundSprite(WIDTH, HEIGHT);
  //background.createSprite(WIDTH, HEIGHT); // Background Sprite  
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto
 */
  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold)))
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println(F("Initialise error"));
    return;
  }
  //pinMode(LED_PIN, OUTPUT);
  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();

  pData.bestDifficulty = "0";
  pData.workersHash = "0";
  pData.workersCount = 0;

  printheap();
}

void esp32_elekstube_AlternateRotation(void)
{
  tft.getRotation() == 0 ? tft.setRotation(2) : tft.setRotation(0);
  hasChangedScreen = true;
}

bool bottomScreenBlue = true;

void printPoolData(){
  pData = getPoolData();
  background.createSprite(135,70); //Background Sprite
  background.setSwapBytes(true);
  background.fillScreen(TFT_CYAN);
  /* if (bottomScreenBlue) {
    background.pushImage(0, 0, 320, 70, bottonPoolScreen);
  } else {
    background.pushImage(0, 0, 320, 70, bottonPoolScreen_g);
  } */
  //background.setTextDatum(MC_DATUM);
  render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(1);
  
  /* render.setFontSize(24);
  render.cdrawString(String(pData.workersCount).c_str(), 160, 35, TFT_BLACK); */
  render.setFontSize(20);
  render.setAlignment(Align::BottomRight);
  render.cdrawString(pData.workersHash.c_str(), 67, 10, TFT_BLACK);
  render.setAlignment(Align::TopLeft);
  render.cdrawString(pData.bestDifficulty.c_str(), 67, 40, TFT_BLACK);

  background.pushSprite(0,170);
  background.deleteSprite();
}

void printMinerScreen(unsigned long mElapsed){
  printheap();
  chip_select.setDigit(5);
  mining_data data = getMiningData(mElapsed);
  if (hasChangedScreen) tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);

  // Create background sprite to print data at once
  createBackgroundSprite(WIDTH, HEIGHT-60);  
  //Print background screen    
  background.pushImage(0, -60, MinerWidth, MinerHeight, MinerScreen);
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
  // Hashrate 
  render.setFontSize(35);
  //render.setCursor(19, 118);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 118, 114-60, TFT_BLACK);
  // Push prepared background to screen
  background.pushSprite(0, 60);
  // Delete sprite to free the memory heap
  background.deleteSprite(); 
  //delay(5);
  printPoolData();
  //printheap();
}

String btcPrice_prev = "";
String clk_prev = "";

void esp32_elekstube_MinerScreen(unsigned long mElapsed)
{
  clock_data cdata = getClockData(mElapsed);
  printheap();
  // createBackgroundSprite(WIDTH, HEIGHT);
  printMinerScreen(mElapsed);

  if ((btcPrice_prev != cdata.btcPrice)||(clk_prev != cdata.currentTime)||(hasChangedScreen)) {
    //Serial.println(cdata.btcPrice);
    hasChangedScreen = false;
    //createBackgroundSprite(WIDTH, HEIGHT);      
      tft.setTextColor(TFT_WHITE, TFT_BLACK);    
    char n = 4;
    String dgt = "";
    for (char dsp=0; dsp<=4; dsp++){
      chip_select.setDigit(dsp);          
      tft.fillScreen(TFT_BLACK); 
      dgt = cdata.btcPrice.substring(n,n+1);
      n--;
      // Print the digit
      tft.setFreeFont(FSSB24);
      tft.setTextSize(4);
      tft.drawCentreString(dgt.c_str(), 67, 40, GFXFF);

      // Print extras
      switch (dsp)
      {
      case 4:
        tft.setFreeFont(FSSB12);
        tft.setTextSize(1);
        tft.drawCentreString("BTC/USDT", 67, 2, GFXFF);
        break;
      case 0:
        tft.setFreeFont(FSSB18);
        tft.setTextSize(1);
        tft.drawCentreString(cdata.currentTime.c_str(), 67, 205, GFXFF);
        break;
      }
      

      // Push prepared background to screen
      // background.pushSprite(0, 0);      
      //Serial.println("Aqui! "+dgt);
    }
    printheap();
    // Delete sprite to free the memory heap    
    btcPrice_prev = cdata.btcPrice;    
    clk_prev = cdata.currentTime;
  }
  // background.deleteSprite(); 
printheap();
  #ifdef DEBUG_MEMORY
    // Print heap
    printheap();
  #endif
}

void esp32_elekstube_ClockScreen(unsigned long mElapsed)
{
   clock_data cdata = getClockData(mElapsed);

  printMinerScreen(mElapsed);

  if ((clk_prev != cdata.currentTime) || (hasChangedScreen)) {
      hasChangedScreen = false; 
      tft.setTextColor(TFT_WHITE, TFT_BLACK);    
    char n = 4;
    String dgt = "";
    for (char dsp=0; dsp<=4; dsp++){
      chip_select.setDigit(dsp);          
      tft.fillScreen(TFT_BLACK); 
      dgt = cdata.currentTime.substring(n,n+1);;
      n--;
      // Print the digit
      tft.setFreeFont(FSSB24);
      tft.setTextSize(4);
      tft.drawCentreString(dgt.c_str(), 67, 40, GFXFF);

      // Print extras
      switch (dsp)
      {
      case 4:
        tft.setFreeFont(FSSB12);
        tft.setTextSize(1);
        tft.drawCentreString("TIME", 67, 2, GFXFF);
        break;
      case 0:
        tft.setFreeFont(FSSB18);
        tft.setTextSize(1);
        tft.drawCentreString(cdata.btcPrice.c_str(), 67, 205, GFXFF);
        
        break;
      }
      
      // Push prepared background to screen
      // background.pushSprite(0, 0);      
      //Serial.println("Aqui! "+dgt);
    }
    printheap();
    // Delete sprite to free the memory heap    
    btcPrice_prev = cdata.btcPrice;    
    clk_prev = cdata.currentTime;
  }
  #ifdef DEBUG_MEMORY
  // Print heap
  printheap();
  #endif
}

void esp32_elekstube_GlobalHashScreen(unsigned long mElapsed)
{

  coin_data data = getCoinData(mElapsed);

  printMinerScreen(mElapsed);

  if ((clk_prev != data.currentTime) || (hasChangedScreen)) {
      hasChangedScreen = false; 
      tft.setTextColor(TFT_WHITE, TFT_BLACK);    
    char n = 5;
    String dgt = "";
    for (char dsp=0; dsp<=4; dsp++){
      chip_select.setDigit(dsp);          
      tft.fillScreen(TFT_BLACK); 
      dgt = data.blockHeight.substring(n,n+1);
      n--;
      // Print extras
      switch (dsp)
      {
      case 4:
        tft.setFreeFont(FSSB12);
        tft.setTextSize(1);
        tft.drawCentreString("BLOCK", 67, 2, GFXFF);
        tft.drawCentreString("HEIGHT", 67, 210, GFXFF);
        tft.setFreeFont(FSSB24);
        tft.setTextSize(3);        
        dgt=data.blockHeight.substring(n,n+1)+dgt;                
        tft.drawCentreString(dgt.c_str(), 67, 50, GFXFF);
        break;
      case 0:
      // Print the digit
        tft.setFreeFont(FSSB24);
        tft.setTextSize(4);
        tft.drawCentreString(dgt.c_str(), 67, 40, GFXFF);
        // Price
        tft.setFreeFont(FSSB18);
        tft.setTextSize(1);
        tft.drawCentreString(data.btcPrice.c_str(), 67, 210, GFXFF);
        break;
      default:
        // Print the digit
        tft.setFreeFont(FSSB24);
        tft.setTextSize(4);
        tft.drawCentreString(dgt.c_str(), 67, 40, GFXFF);
        break;
      }
      
      // Push prepared background to screen
      // background.pushSprite(0, 0);      
      //Serial.println("Aqui! "+dgt);
    }
    printheap();
    // Delete sprite to free the memory heap    
    btcPrice_prev = data.btcPrice;    
    clk_prev = data.currentTime;
  }
  #ifdef DEBUG_MEMORY
  // Print heap
  printheap();
  #endif

  /* if (hasChangedScreen) tft.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);
  hasChangedScreen = false;
  coin_data data = getCoinData(mElapsed);

  // Create background sprite to print data at once
  createBackgroundSprite(169,105);
  // Print background screen
  background.pushImage(-160, -3, minerClockWidth, minerClockHeight, globalHashScreen);
  //background.fillScreen(TFT_BLUE);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 198-160, 0, GFXFF);
  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 268-160, 0, GFXFF);

  // Print Last Pool Block
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), 302-160, 49, GFXFF);

  // Print Difficulty
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), 302-160, 85, GFXFF);
  // Push prepared background to screen
  background.pushSprite(160, 3);
  // Delete sprite to free the memory heap
  background.deleteSprite();   

 // Create background sprite to print data at once
  createBackgroundSprite(280,30);
  // Print background screen
  background.pushImage(0, -139, minerClockWidth, minerClockHeight, globalHashScreen);
  //background.fillSprite(TFT_CYAN);
  // Print Global Hashrate
  render.setFontSize(17);
  render.rdrawString(data.globalHashRate.c_str(), 274, 145-139, TFT_BLACK);

  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149-139, x2, 168, 0xDEDB);

  // Print Remaining BLocks
  background.setTextFont(FONT2);
  background.setTextSize(1); 
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 72, 159-139, FONT2);

  // Push prepared background to screen
  background.pushSprite(0, 139);
  // Delete sprite to free the memory heap
  background.deleteSprite();   

 // Create background sprite to print data at once
  createBackgroundSprite(140,40);
  // Print background screen
  background.pushImage(-5, -100, minerClockWidth, minerClockHeight, globalHashScreen);
  //background.fillSprite(TFT_CYAN);
  // Print BlockHeight
  render.setFontSize(28);
  render.rdrawString(data.blockHeight.c_str(), 140-5, 104-100, 0xDEDB);

  // Push prepared background to screen
  background.pushSprite(5, 100);
  // Delete sprite to free the memory heap
  background.deleteSprite();   
  #ifdef esp32_elekstube      
   printPoolData();
  #endif      

  #ifdef DEBUG_MEMORY
  // Print heap
  printheap();
  #endif */
}

void esp32_elekstube_SatsUsdScreen(unsigned long mElapsed)
{
  clock_data cdata = getClockData(mElapsed);
  
  // createBackgroundSprite(WIDTH, HEIGHT);
  printMinerScreen(mElapsed);

  if ((btcPrice_prev != cdata.btcPrice)||(clk_prev != cdata.currentTime)||(hasChangedScreen)) {
    char satsUsd[5] = "";
    int btcv = atoi(cdata.btcPrice.substring(0,5).c_str());
    float btcvd = 1.0/btcv;
    dtostrf((btcvd*100000000),5,0,satsUsd);
    //satsUsd = (1/atoi(cdata.btcPrice.c_str() ))*100000000;
    Serial.println(btcvd,20);
    hasChangedScreen = false;
    //createBackgroundSprite(WIDTH, HEIGHT);      
    tft.setTextColor(TFT_WHITE, TFT_BLACK);    
    char n = 4;
    String dgt = "";
    for (char dsp=0; dsp<=4; dsp++){
      chip_select.setDigit(dsp);          
      tft.fillScreen(TFT_BLACK); 
      dgt = satsUsd[n];
      n--;
      // Print the digit
      tft.setFreeFont(FSSB24);
      tft.setTextSize(4);
      tft.drawCentreString(dgt.c_str(), 67, 40, GFXFF);

      // Print extras
      switch (dsp)
      {
      case 4:
        tft.setFreeFont(FSSB12);
        tft.setTextSize(1);
        tft.drawCentreString("SATS/USDT", 67, 2, GFXFF);        
        tft.drawCentreString("MSCW/TIME", 67, 205, GFXFF);
        break;
      case 0:
        tft.setFreeFont(FSSB18);
        tft.setTextSize(1);
        tft.drawCentreString(cdata.currentTime.c_str(), 67, 205, GFXFF);
        break;
      }
      

      // Push prepared background to screen
      // background.pushSprite(0, 0);      
      //Serial.println("Aqui! "+dgt);
    }
    //printheap();
    // Delete sprite to free the memory heap    
    btcPrice_prev = cdata.btcPrice;    
    clk_prev = cdata.currentTime;
  }
  // background.deleteSprite(); 
printheap();
  #ifdef DEBUG_MEMORY
    // Print heap
    printheap();
  #endif
}

void esp32_elekstube_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 33, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(CURRENT_VERSION, 24, 147, FONT2);
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  // tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
}

void esp32_elekstube_SetupScreen(void)
{
  tft.pushImage(0, 33, setupModeWidth, setupModeHeight, setupModeScreen);
}

void esp32_elekstube_AnimateCurrentScreen(unsigned long frame)
{
}

// Variables para controlar el parpadeo con millis()
unsigned long previousMillis = 0;
unsigned long previousTimerMillis = 0;
char currentScreen = 0; 
char px = 6;

void esp32_elekstube_DoLedStuff(unsigned long frame)
{
  mButton.tick();pButton.tick();lButton.tick();rButton.tick();
  unsigned long currentMillis = millis();    
  
  if (timeScr>0){
    if (currentMillis>((timeScr*60*1000)+previousTimerMillis)) {
       switchToNextScreenInt();
       previousTimerMillis = currentMillis;
    }
  }

  if (currentScreen != currentDisplayDriver->current_cyclic_screen) hasChangedScreen ^= true;
    currentScreen = currentDisplayDriver->current_cyclic_screen;
  if (doLed) {      
      switch (mMonitor.NerdStatus)
      {      
      case NM_waitingConfig:
        //digitalWrite(LED_PIN, HIGH); // LED encendido de forma continua
        px--;
        pixels.setPixelColor(px, pixels.Color(0, 255, 0));        
        break;

      case NM_Connecting:
        if (currentMillis - previousMillis >= 500)
        { // 0.5sec blink
          previousMillis = currentMillis;
          //digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
          px--;
          pixels.clear();
          pixels.setPixelColor(px, pixels.Color(0, 0, 255));      
        }
        break;

      case NM_hashing:
        if (currentMillis - previousMillis >= 200)
        { // 0.1sec blink
          px--;
          pixels.clear();
          pixels.setPixelColor(px, pixels.Color(255, 0, 0));
          previousMillis = currentMillis;                    
          //digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
        }
        break;
      }      
      if (!px) px=6;
      //Serial.println(px);
      pixels.show();  
  }
}

CyclicScreenFunction esp32_elekstubeCyclicScreens[] = {esp32_elekstube_ClockScreen, esp32_elekstube_MinerScreen, esp32_elekstube_GlobalHashScreen , esp32_elekstube_SatsUsdScreen};

DisplayDriver esp32_elekstubeDriver = {
    esp32_elekstube_Init,
    esp32_elekstube_AlternateScreenState,
    esp32_elekstube_AlternateRotation,
    esp32_elekstube_LoadingScreen,
    esp32_elekstube_SetupScreen,
    esp32_elekstubeCyclicScreens,
    esp32_elekstube_AnimateCurrentScreen,
    esp32_elekstube_DoLedStuff,
    SCREENS_ARRAY_SIZE(esp32_elekstubeCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif