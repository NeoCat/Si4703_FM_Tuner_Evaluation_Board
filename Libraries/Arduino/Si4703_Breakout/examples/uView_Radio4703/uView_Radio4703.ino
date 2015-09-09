// Si4703_Breakout + MicroView (https://www.sparkfun.com/products/12923)

#include <Si4703_Breakout.h>
#include <Wire.h>
#include <MicroView.h>

// Connected to Si4703 FM Tuner Breakout board
const int rdsiPin = A2;  // Used to detect RDS update
const int resetPin = A3;
const int SDIO = A4;
const int SCLK = A5;

// Switches - should be connected to GND when switches are pushed.
//            Note that pins are weakly pulled up.
const int V_UP = A0;
const int V_DOWN = A1;
const int C_UP = 2;
const int C_DOWN = 3;
const int C_A = 5;
const int C_B = 6;

Si4703_Breakout radio(resetPin, SDIO, SCLK, rdsiPin);

int channel = 1067;  // default channel on power up
int volume = 1;      // default volume on power up

char psName[9], lastPsName[9];
char radioText[65] = {0};
char radioTextLen = 0;
unsigned long scroll_ms;
unsigned long last_update_ms = 0;

void setup() {
  uView.begin();
  uView.clear(ALL);
  uView.display();

  Serial.begin(9600);

  Serial.println("Powering on...");
  radio.powerOn(true);
  radio.setVolume(volume);
  radio.setChannel(channel);
  Serial.println("OK.");
  updateDisplay();
  
  pinMode(V_UP, INPUT);    digitalWrite(V_UP, HIGH);
  pinMode(V_DOWN, INPUT);  digitalWrite(V_DOWN, HIGH);
  pinMode(C_UP, INPUT);    digitalWrite(C_UP, HIGH);
  pinMode(C_DOWN, INPUT);  digitalWrite(C_DOWN, HIGH);
  pinMode(C_A, INPUT);     digitalWrite(C_A, HIGH);
  pinMode(C_B, INPUT);     digitalWrite(C_B, HIGH);
}

void loop() {
  char ch = 0;
  if (Serial.available())
    ch = Serial.read();

  if (!digitalRead(C_UP) || ch == 'u') {
    channel = radio.seekUp();
    updateDisplay();
    if (!ch) delay(50);
  }
  else if (!digitalRead(C_DOWN) || ch == 'd') {
    channel = radio.seekDown();
    updateDisplay();
    if (!ch) delay(50);
  }
  else if (!digitalRead(C_A) || ch == 'a') {
    channel = 1067;
    radio.setChannel(channel);
    updateDisplay();
    if (!ch) delay(200);
  }
  else if (!digitalRead(C_B) || ch == 'b') {
    channel = 995;
    radio.setChannel(channel);
    updateDisplay();
    if (!ch) delay(200);
  }
  else if (!digitalRead(V_UP) || ch == '+')  {
    volume ++;
    if (volume == 16) volume = 15;
    radio.setVolume(volume);
    updateDisplay();
    if (!ch) delay(200);
  }
  else if (!digitalRead(V_DOWN) || ch == '-') {
    volume --;
    if (volume < 0) volume = 0;
    radio.setVolume(volume);
    updateDisplay();
    if (!ch) delay(200);
  }

  if (radio.rdsAvailable()) {
    const char *psname = radio.getPSName();
    if (psname) {
      if (memcmp(lastPsName, psname, 9) == 0 && // received the same name twice
          memcmp(psName, psname, 9)) {          // name is updated
        memcpy(psName, psname, 9);
        Serial.print("PSNAME: ");
        Serial.println(psname);
      }
      memcpy(lastPsName, psname, 9);
    }
    const char *text = radio.getText();
    if (text) {
      Serial.print("TEXT: ");
      Serial.println(text);
      if (strncmp(radioText, text, 65)) {
        memcpy(radioText, text, 65);
        radioTextLen = strlen(text);
        scroll_ms = millis();
      }
    }
    displayInfo();
  }
  unsigned long ms = millis();
  if (ms - last_update_ms > 64) {
    last_update_ms = ms;
    displayInfo();
  }
}

void updateDisplay()
{
  static int last_channel;
  if (channel != last_channel) {
    radioText[0] = 0;
    psName[0] = 0;
    radioTextLen = 0;
  }
  last_channel = channel;
  displayInfo();
  serialOutInfo();
}

void serialOutInfo()
{
   Serial.print("Channel:"); Serial.print(channel);
   Serial.print(" Volume:"); Serial.print(volume);
   Serial.print(" RSSI:"); Serial.println(int(radio.rssi()));
}

void displayInfo()
{
   unsigned long ms = millis();
   static unsigned long last_ms = 0;
   if (ms - last_ms > 500) {
     radio.readRegisters();
     last_ms = ms;
   }
  
   uView.clear(PAGE);
   uView.setCursor(0, 0);
   uView.print(channel/10); uView.print(".");
   uView.print(channel%10); uView.print(" MHz");
   uView.setCursor(0, 10);
   uView.print("VOL: "); uView.print(volume);
   uView.setCursor(0, 20);
   uView.print(int(radio.rssi())); uView.print(" dB");
   if (radio.stereo()) uView.print(" [S]");
   
   uView.setCursor(0, 30);
   uView.print(psName);

   int scroll = (ms - scroll_ms)/64 - 16;
   if (scroll > radioTextLen*3) {
     scroll_ms = ms;
     scroll = 0;
   }
   if (scroll < 0) scroll = 0;
   uView.setCursor(-scroll%3*2, 40);
   uView.print(radioText + scroll/3);

  // invert
   uView.invert((millis()>>18) & 1);

   uView.display();
}
