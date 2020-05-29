// 7.02.2020 13:10
//   Autor: Jan Burbicki

// **Biblioteki** - poza standardowymi
// nalezy pobrac:
//  - Adafruit BME280 Library by Adafruit
//  - Adafruit SSD1206 by Adafruit

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <stdlib.h>

Adafruit_BME280 bme; // obiekt do obslugi funkcji czujnika temp, wilgo i cisnienia
Adafruit_SSD1306 display(128, 64, &Wire, 4); // ustawienia wyswietlacza OLED

// stałe do miernika pylu
#define PIN_LED           7 // numer pinu ILED
#define PIN_ANALOG        0 // numer pinu AOUT

const int  buttonPin = 2;    // the pin that the pushbutton is attached to
uint8_t buttonPushCounter = 0;   // counter for the number of button presses
uint8_t buttonState = 0;         // current state of the button


double AVR_DUST_TABLE[20]; // tabela do usredniania wynikow
int i = 0; // zmienna do przechodzenia tablicy
char* info[] = {"g/m3", "pm 2,5",
                "*C", "temp",
                "hPa", "press",
                "%", "humid",
                "pomiar", "cos" // tu jest miejsce na komunikacje z uzytkownikiem
               }; // NIE zmieniajcie kolejnosci tych zmiennych

void setup() {

  unsigned status;
  double AVG_DUST = 0;

  pinMode(buttonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), toggle, FALLING); // ISR - Interrupt Service Routine

  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Alokacja SSD1306 nie powiodla sie!"));
    Serial.println(F("Mozliwe przyczyny:"));
    Serial.println(F(" - Blad podlaczenia"));
    Serial.println(F(" - Przeladowanie pamieci RAM"));
    for (;;);
  }

  // JAKO JEDYNY OPERUJE NA 3.3V!!!
  if (!bme.begin(0x76)) {
    Serial.println("Blad podlaczenia czujnika BME280!");
    Serial.println(F("Mozliwe przyczyny:"));
    Serial.println(F(" - Blad podlaczenia"));
    Serial.println(F(" - Podpiecie do 5v - czujnik spalony"));
    for (;;);
  }

  // komunikacja z uzytkownikiem
  // info o obliczaniu wyniku tabeli
  AVG_DUST = normal_avrage();
  delay(1000);
}


void loop(void) {

  switch (buttonPushCounter) {
    case 0:
      //pyl(500);
      Serial.print("ComputeDust()= ");
      Serial.println(computeDust());
      delay(500);
      break;
    case 1:
      temperatura(500);
      break;
    case 2:
      cisnienie(500);
      break;
    case 3:
      wilgotnosc(500);
      break;
  }
}


double normal_avrage() { // funkcja oblicza srednia z tabeli AVR_DUST_TABEL[] i podaje ja
  double AVG_DUST = 0;
  for (int i = 0; i < 20; i++) {
    AVR_DUST_TABLE[i] = avrageDust(5);
  }
  for (int i = 0; i < 20; i++) {
    AVG_DUST = AVG_DUST + AVR_DUST_TABLE[i];
  }
  return AVG_DUST / 20;
}


double rolling_avrage(int i) {// funkcja oblicza toczaca sie srednia z tabeli AVR_DUST_TABEL[] i podaje ja
  double AVG_DUST = 0;
  AVR_DUST_TABLE[i] = avrageDust(5);
  for (int i = 0; i < 20; i++) {
    AVG_DUST = AVG_DUST + AVR_DUST_TABLE[i];
  }
  return AVG_DUST / 20;
}


double avrageDust(int times) { // usrednia wyniki funkcji computeDust()
  double avrDust = 0;
  for (int i = 0; i < times; i++) {
    avrDust += computeDust();
  }
  return avrDust / times;
}


double computeDust() { // pojedynczy pomiar pylu przez czujnik
  float VOLTAGE = 0;
  int ADC_VALUE = 0;
  digitalWrite(PIN_LED, HIGH); // Blyskamy IR, czekamy 280ms, odczytujemy napiecie ADC
  delayMicroseconds(280);
  ADC_VALUE = analogRead(PIN_ANALOG);
  digitalWrite(PIN_LED, LOW);
  VOLTAGE = (5000 / 1024.0) * ADC_VALUE * 11; // Przeliczamy na mV. Calosc mnozymy przez 11, poniewaz w module zastosowano dzielinik napiecia 1k/10k
  //Serial.println(VOLTAGE);
  if (VOLTAGE > 50) // Obliczamy zanieczyszczenie jesli zmierzone napiecie ponad prog
  {
    return (VOLTAGE - 50) * 0.2;
  }
  return 0;
}

long lastInterrupt = 0;
void toggle() {
  if (millis() - lastInterrupt > 200) {
    buttonPushCounter = (buttonPushCounter + 1) % 4;
    lastInterrupt = millis();
  }
}


void screen(int wynik, int data) {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.cp437(true);

  display.print(wynik);
  display.println(info[data]);

  display.setCursor(0, 32);
  display.setTextSize(2);

  char* output = info[data + 1];
  display.println(output);

  display.display();
}


void pyl(int czas) {
  i = (i + 1) % 20;
  screen(rolling_avrage(i), 0);
  //  for (int i = 0; i < 20; i++) {
  //    Serial.print(i);
  //    Serial.print(" ");
  //    Serial.println(AVR_DUST_TABLE[i]);
  //  }
  Serial.println();
  Serial.print("pm 2,5 = ");
  Serial.print(rolling_avrage(i));
  Serial.println("g/m3");
  delay(czas);
}


void temperatura(int czas) {
  screen(double(bme.readTemperature()), 2);
  Serial.print("Temperature = ");
  Serial.print(double(bme.readTemperature()));
  Serial.println("*C");
  delay(czas);
}


void cisnienie(int czas) {
  screen(int(bme.readPressure() / 100.0F), 4);
  Serial.print("Pressure = ");
  Serial.print(int(bme.readPressure() / 100.0F));
  Serial.println("hPa");
  delay(czas);
}


void wilgotnosc(int czas) {
  screen(double(bme.readHumidity()), 6);
  Serial.print("Humidity = ");
  Serial.print(double(bme.readHumidity()));
  Serial.println("%");
  delay(czas);
}
