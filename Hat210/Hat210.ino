#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

////////////////////////////////////////////
// Really Rough Version of the code powering the hat.
// Author: jjdb210
// Email: justin@jrcorps.com
// Website: http://emcot.world/
// Description: Basic Bluetooth, Pixel, and PWM reader from making a glow had from a magicband+ and glow with the show wand. Hardware details to be posted later.
// Designed For: Seeed Studio XIAO ESP32C3
////////////////////////////////////////////

#define PIN      10
#define N_LEDS 49         //Should be 50, but 1 broke off. Set to max pins of pixel strp. Other values may be needed for ears later in code.

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);

#define SERVICE_UUID        "210fc201-1fb5-459e-8fcc-000000000000"
#define CHARACTERISTIC_UUID "2105483e-36e1-4688-b7f5-000000000001"
#define CHARACTERISTIC_UUID2 "2105483e-36e1-4688-b7f5-000000000002" 
extern int mode=0;

//The bluetooth Controls
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      try {                 //Try block here because some values cause all sorts of problems I guess.
        mode = stoi(value);
      } catch(...){
        //Serial.println("Invalid value");    //Serial Debugging    
      }
    }
    void onConnect(BLEServer* pServer) {
      pServer->startAdvertising(); // restart advertising
    };

     void onDisconnect(BLEServer* pServer) {
      pServer->startAdvertising(); // restart advertising
    }
};

//There is probably a better way to do this.
//Binary tables to map multiplexors to their respective pins.
int ad3[] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1 };
int ad4[] = {0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1 };
int ad5[] = {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1 };
int ad6[] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1 };
int ad7[] = {0,0,0,0,1,1,1,1};
int ad8[] = {0,0,1,1,0,0,1,1};
int ad9[] = {0,1,0,1,0,1,0,1};
int averages[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int next[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int last[] =  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int last2[] =  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int last3[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//Values red in from the other devices. Values may vary based on source data/circut.
int highs[] = {3900,3350,3200,  3900,3300,2700,   3900,3220,3000,   3900,3350,3000, 2000,3100,3000,  500, 700, 2000,   2400, 700, 500};
int lows[] = {3370,2600,2200,  3300,2600,2200, 3300,2800,2200,   3300,2600,2100,  0,2600,2300,  0,0,900     , 700,0,0};

void setup() {
  Serial.begin(9600);       //For Debug. Can be disabled when production.
  strip.begin();            // Pixel strip ON!

  pinMode(A0, INPUT);   //Magicband Multiplex
  pinMode(A1, INPUT);   //Wand Multiplex
  
  pinMode(D2, OUTPUT);  //Turn Wand On/Off
  digitalWrite(D2, HIGH); 
      
  //Multiplexor Pins
  pinMode(D3, OUTPUT);  //Magicband S3
  pinMode(D4, OUTPUT);  //Magicband S2
  pinMode(D5, OUTPUT);  //Magicband S1
  pinMode(D6, OUTPUT);  //Magicband S0
  pinMode(D7, OUTPUT);  //Wand S2
  pinMode(D8, OUTPUT);  //Wand S1
  pinMode(D9, OUTPUT);  //Wand S0

  //Setup the ADC and some initial values
  analogReadResolution(12); 
  strip.setBrightness(80);  //Power save/appearance control

  //Setup Bluetooth
  BLEDevice::init("ttv/jjdb210?DisneyHat210");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("0");
  pService->start();
 
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  //Mode 0 is default mode (read in data from other 2 devices)
  if (mode == 0){   
    int sensorValue = 0;    //Value of sensor currently being read. 
    String output = "";     //Used in debugging
    
    for (int z=0; z< 21; z++){          //Loop Over all Multiplexors
      if (z < 15){                      //Magic Band Reads
        digitalWrite(D3, ad3[z]); 
        digitalWrite(D4, ad4[z]); 
        digitalWrite(D5, ad5[z]); 
        digitalWrite(D6, ad6[z]); 
        sensorValue = analogRead(A0);   //MagicBand Input
        //if (z == 12 || z == 13 || z == 14){     //Used to get values of each LED in Debugging
        //  output = output + String(sensorValue) + " ";
        //}
      }
      if (z >= 15){
        int j = 0;
        j = z-15;
        digitalWrite(D7, ad7[j]); 
        digitalWrite(D8, ad8[j]); 
        digitalWrite(D9, ad9[j]); 
        sensorValue = analogRead(A1);   //Wand Input
      }
      next[z] = sensorValue;            //Keep an array of all the values
    }

    //Poor Man's PWM filter
    int temp =0; 
    for (int i=0; i < 7; i++){
      temp = 0;
      temp = next[i * 3] + next[1 + (i*3)] + next[2+(i*3)];
      if ((i >= 5 && temp > 5800)){     //We aren't using the filter on the magicband at this time.
      }  else {
        averages[(i * 3)] = next[(i * 3)];
        averages[1 + (i*3)] =next[1 + (i*3)];
        averages[2+(i*3)] =next[2+(i*3)];
      }
    }

    for( int i = 34; i<50; i++){
      strip.setPixelColor(i, strip.Color(mscale(17, averages[17]), mscale(16, averages[16]), mscale(15, averages[15])));
    } 
    strip.setPixelColor(33, strip.Color(255,255, 0));
    for(int i=20; i<33; i++){
      strip.setPixelColor(i, strip.Color(mscale(18, averages[18]), mscale(19, averages[19]), mscale(20, averages[20])));
    }

    for (int i=0; i<4; i++){
      strip.setPixelColor(i, strip.Color(mscale(0, averages[0]), mscale(1, averages[1]), mscale(2, averages[2])));
    }
    for (int i=4; i<8; i++){
      strip.setPixelColor(i, strip.Color(mscale(3, averages[3]), mscale(4, averages[4]), mscale(5, averages[5])));
    }
    for (int i=8; i<12; i++){
      strip.setPixelColor(i, strip.Color(mscale(6, averages[6]), mscale(7, averages[7]), mscale(8, averages[8])));
    }
    for (int i=12; i<16; i++){
      strip.setPixelColor(i, strip.Color(mscale(9, averages[9]), mscale(10, averages[10]), mscale(11, averages[11])));
    }
    for (int i=16; i<20; i++){
      strip.setPixelColor(i, strip.Color(mscale(12, averages[12]), mscale(13, averages[13]), mscale(14, averages[14])));
    }
    
    strip.show();
    //End Mode 0 
  } else if (mode == 1){      //Mode 1 = Hat is off, Magicband is on.
    digitalWrite(D2, LOW);    //Turn off Wand
    for (int i=0; i<N_LEDS; i++){ //Turn Off LED's
      strip.setPixelColor(i, strip.Color(0,0,0));
    }
    delay(1000);              //Save power?
    strip.show();
  } else if (mode == 2) {     //Mode 2= Hat is Blue
    digitalWrite(D2, LOW);
    for (int i=0; i<N_LEDS; i++){
      strip.setPixelColor(i, strip.Color(0,0,255));
    }
    strip.show();
    delay(1000);              //Save Power? Might be a better way to do this.
  } else if (mode == 3){      //Mode 3 = Sparkling Lights
    digitalWrite(D2, LOW);
    chase();     
  }
}

//Used to scale values to 0-255
static int mscale(int i,int value){
  String output;       //Used for Debug
  if (value < lows[i]){   //Don't let Value exceed range.
    value = lows[i];
  }
  if (value > highs[i]){
    value = highs[i];
  }
  
  int calc = highs[i] - value;
  int total = highs[i] - lows[i];
  float calc2 = calc / float(total);
  int final = 255 * calc2;

  //Limiter (we light fades not flashes with the PWM in play).
  if (last[i] > final && abs(last[i] - final) > 2){
    final = last[i] - 2;
  }
  if (last[i] < final && abs(last[i] - final) > 2){
    final = last[i] + 2;
  }

   last[i] = final;
   return final;

}

//Not my code. Taken from a NEO pixel Library.
void chase() {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(200);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
