//Aqua Project v1.0 - 
// BY Poulpy 2020 - https://github.com/Poulpy2020/Aqua-project.git
// Include Ephemeris - Auto Sunshine / Sunrise
// Incluse Auto time via Wifi
// Include MQTT Temperatur
// Incluse Auto Fish eat 

// SETUP WIFI / MQTT / on L. 55
// SETUP LED L. 28
// SETUP ephemeris via L. 383 // https://www.coordonnees-gps.fr/
// Fish distribution - comment if need disable / or change time via  L. 389


#include <ESP8266WiFi.h>
#include <time.h>                   // time() ctime()
#include <WiFiUdp.h>
#include <PubSubClient.h> / MQTT
#include <Ephemeris.h> // bibliotheque ephemeris
int Reset_RiseAndSet = 0;

#include <SPI.h>
#include <OneWire.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//LED
#include <Adafruit_NeoPixel.h>
// gestion led nhg
#define LED_PIN     14
#define LED_COUNT   60
#define BRIGHTNESS  100
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
//Servomoteur
#include <Servo.h> 

Servo servoMoteur; 


  unsigned long currentMillis = millis();
  int r = 0;
  int g = 50;
  int b = 100;
  boolean lum_jour = false; // 
boolean lum_nuit = false; // 
  int timer =0;

  //Bouton
  const int buttonPin = 2; 
  int buttonState = 0;         // variable pour mémoriser l'état du bouton
  int buttonPress = 0; // nombre de fois appuier
  

//WIFI
#define NB_TRYWIFI        50    // Nbr de tentatives de connexion au réseau WiFi | Number of try to connect to WiFi network
const char* ssid = "YOUR SSID";
const char* password = "YOUR WIFI PASS";
IPAddress ip(YOUR IP DEVICE); // ex  192, 168, 1, 217
IPAddress dns(YOUR DNS); // ex 192, 168, 1, 1
IPAddress gateway(YOUR gateway); // ex 192, 168, 1, 1 
IPAddress subnet(YOUR subnet); // ex 255, 255, 255, 255

//mqtt server
const char* mqtt_server = "YOUR MQTT SERVER IP";//Adresse IP du Broker Mqtt
const int mqttPort = 1883; //port 
#define mqtt_user "USER MQTT" // MQTT USER
#define mqtt_password "PASS WORD MQTT" // MQTT PASSWORD
char msg[50];
String stringmsg;
#define temperature_topic "aquarium/temperature"

WiFiClient espClient;
WiFiUDP ntpUDP;
PubSubClient client(espClient);

/* Configuration of NTP */
#define MY_NTP_SERVER "europe.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

/* Globals */
time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convient way
int sunsetHour;
int sunsetMin;
int sunriseHour;
int sunriseMin;
int last_min =0;
int count_temps = 0;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Initialisation de l'écran LCD en utilisant la bibliothèque Wire
//SSD1306  display(0x3c, D6, D5);
// la comunication I2C à l'adresse 0x3c (pour l'écran OLED 128x64), broché en D6 pour SDA et D5 pour SCL

/* Broche du bus 1-Wire */
const byte BROCHE_ONEWIRE = 13;

/* Code de retour de la fonction getTemperature() */
enum DS18B20_RCODES {
  READ_OK,  // Lecture ok
  NO_SENSOR_FOUND,  // Pas de capteur
  INVALID_ADDRESS,  // Adresse reçue invalide
  INVALID_SENSOR  // Capteur invalide (pas un DS18B20)
};


/* Création de l'objet OneWire pour manipuler le bus 1-Wire */
OneWire ds(BROCHE_ONEWIRE);
 
 
/**
 * Fonction de lecture de la température via un capteur DS18B20.
 */
byte getTemperature(float *temperature, byte reset_search) {
  byte data[9], addr[8];
  // data[] : Données lues depuis le scratchpad
  // addr[] : Adresse du module 1-Wire détecté
  
  /* Reset le bus 1-Wire ci nécessaire (requis pour la lecture du premier capteur) */
  if (reset_search) {
    ds.reset_search();
  }
 
  /* Recherche le prochain capteur 1-Wire disponible */
  if (!ds.search(addr)) {
    // Pas de capteur
    return NO_SENSOR_FOUND;
  }
  
  /* Vérifie que l'adresse a été correctement reçue */
  if (OneWire::crc8(addr, 7) != addr[7]) {
    // Adresse invalide
    return INVALID_ADDRESS;
  }
 
  /* Vérifie qu'il s'agit bien d'un DS18B20 */
  if (addr[0] != 0x28) {
    // Mauvais type de capteur
    return INVALID_SENSOR;
  }
 
  /* Reset le bus 1-Wire et sélectionne le capteur */
  ds.reset();
  ds.select(addr);
  
  /* Lance une prise de mesure de température et attend la fin de la mesure */
  ds.write(0x44, 1);
  delay(800);
  
  /* Reset le bus 1-Wire, sélectionne le capteur et envoie une demande de lecture du scratchpad */
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);
 
 /* Lecture du scratchpad */
  for (byte i = 0; i < 9; i++) {
    data[i] = ds.read();
  }
   
  /* Calcul de la température en degré Celsius */
  *temperature = (int16_t) ((data[1] << 8) | data[0]) * 0.0625; 
  
  // Pas d'erreur
  return READ_OK;
}
 
 
/** Fonction setup() **/
void setup() {

  /* Initialisation du port série */
  Serial.begin(115200);
 delay( 3000 ); // power-up safety delay

  // configure la broche numérique en SORTIE
  pinMode(buttonPin, INPUT); 
 
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds

    // Clear the buffer
  display.clearDisplay();

  Serial.println("");
  Serial.print("Startup reason:");Serial.println(ESP.getResetReason());
  WiFi.config(ip, dns, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi.");
  int _try = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("..");
    delay(500);
    _try++;
    if ( _try >= NB_TRYWIFI ) {
        Serial.println("Impossible to connect WiFi network, go to deep sleep");
        ESP.deepSleep(10e6);
    }
  }
  Serial.println("Connected to the WiFi network");
  // Démarrage client MQTT
  client.setServer(mqtt_server, mqttPort);
 // reconnect();
  //client.publish("Aquarium @MQTT", "Hello from ESP32");
  
  // Démarrage du client NTP - Start NTP client
  configTime(MY_TZ, MY_NTP_SERVER);
  
  printRiseAndSet("Illzach",47.7784881,7.3433351,+1,tm.tm_mday,tm.tm_mon +1,tm.tm_year + 1900);// On donne sa latitude (https://www.coordonnees-gps.fr/)
  

  //initilalise Led
    leds.begin();           // INITIALIZE NeoPixel ring object (REQUIRED)
    leds.setBrightness(BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)
    leds.show();            // Turn OFF all pixels ASAP

  

}//Fin Setup

 void reconnect(){
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("MQTT connecté");
    }
    else {
       display.clearDisplay();
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      // Display static text
      display.print(client.state());
      display.display(); 
    delay(2000);
    }
  }
 }
 
/** Fonction loop() **/
void loop() {
  float temperature;


   
  /* Lit la température ambiante à ~1Hz */
  if (getTemperature(&temperature, true) != READ_OK) {
    Serial.println(F("Erreur de lecture du capteur"));
    delay(1000);
    return;
  }

  // Heure du miam

  //DETECTION NIVEAU EAU

  // Alumer led en fonction jour & nuit

  // Allumer lum Mode JOUR 
  if ((tm.tm_hour > sunriseHour && tm.tm_hour < sunsetHour) || (tm.tm_hour == sunriseHour && tm.tm_min >= sunriseMin) || (tm.tm_hour == sunsetHour && tm.tm_min <= sunsetMin)){
  Serial.println("Mode JOUR");
     // Plein jour
    if (lum_jour == false){
    // Start timer 1H
    if ((currentMillis + 50000) <= (millis()) && (timer <= 60)){
    timer ++;
    currentMillis = millis();
    r = r +4;
    g = g +3;
    b = b +2;
    if (timer >58){
    r = g = b = 250;
    }
    Couleur_lampe(r,g,b); // appel fonction avec couleur blanc
    }
    if (timer >=60){
      lum_jour = true;
      lum_nuit = false;
      timer = 0;
    }
    }
          
  }//  Allumer lum Mode Nuit
  else{
  // 1h pendant coucher du soleil
   //pleine nuit
   Serial.println("Mode NUIT");

  // verifier si pleine nuit
  if ((tm.tm_hour > sunsetHour) && (timer == 0) && (lum_nuit == false)) {
    Serial.println("Mode NUIT - Forcer");
    timer = 59;
  }
        if (lum_nuit == false){
       // Start timer 1H
            if ((currentMillis + 50000) <= (millis()) && (timer <= 60)){
            timer ++;
            Serial.println("Mode NUIT TIMER");
            currentMillis = millis();
            r = r -4;
            g = g -3;
            b = b -2;
             if (timer >58){
              Serial.println("Mode NUIT - FIN TIMER");
             r = 0;
             g = 50;
             b = 100;
            }
          Couleur_lampe(r,g,b); // appel fonction avec couleur bleu nuit
           }
             if (timer >=60){
                lum_jour = false;
                lum_nuit = true;
                timer = 0;
                }
      }
  }

    // bouton focer mode nuit / jour
    /*
    buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH) {    
      if (buttonPress == 0){
       // mode jour,
      Couleur_lampe(250,250,250);
      lum_jour = true;
      lum_nuit = false;
      delay(250);
      buttonPress  ++;
      }else{
        // mode nuit
      Couleur_lampe(0,50,100);
      lum_jour = false;
      lum_nuit = true;
      delay(250);
      buttonPress --;
      }
    }
  */

  
    leds.show(); 
  /* Affiche la température */
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.print(F("T: "));
  display.print(temperature, 2);
  display.print((char)247);
     display.print("C");

  //affiche date
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print(tm.tm_hour);
  display.print(":");
  display.print(tm.tm_min);
  

  //Calcul 1x par jour à 03:00 ou reset
  if ((tm.tm_hour)== 3 && (tm.tm_min)<1 && (tm.tm_sec)<6 || Reset_RiseAndSet == 0){
     printRiseAndSet("PARIS",48.862725,2.287592,+1,tm.tm_mday,tm.tm_mon +1,tm.tm_year + 1900);// On donne sa latitude (https://www.coordonnees-gps.fr/)
     Reset_RiseAndSet = 1;
  }

  // Distributino nouriture 1x par jour à 21h00
  if ((tm.tm_hour)== 21 && (tm.tm_min)<1 && (tm.tm_sec)<3){
  // attach servo moteur
  servoMoteur.attach(2);
  servoMoteur.write(30);
  delay(2000);
  servoMoteur.write(160);
  delay(2000);
  servoMoteur.write(30);
  delay(500);
  servoMoteur.detach();
  }

  // Afficharge heure lever & coucher soleil
  display.print(" \\");
  display.print(sunriseHour);
  display.print(":");
  display.print(sunriseMin);
  display.print(":");
  display.print("//");
  display.print(sunsetHour);
  display.print(":");
  display.print(sunsetMin);
  //Afficher 
  display.display(); 

  // Message MQTT
  //Publie 1x par toute les 5 min
  if ((tm.tm_min) == (last_min) || (last_min == 0)){
    if (!client.connected()){
     reconnect();
    }
    if (tm.tm_min >55 ){
    last_min = (tm.tm_min) - 55;
    }else{
     last_min = (tm.tm_min +5);
    }
    stringmsg=temperature;
    stringmsg.toCharArray(msg,5);
    char msg[8];
    //dtostrf(temperature, 6, 2, msg);
  client.publish(temperature_topic, dtostrf(temperature, 6, 2, msg), true);
  }
  

  
  delay(900);

} // Fin Loop


void Couleur_lampe(int r, int g, int b){

   for (int i=0; i<LED_COUNT; i++){
   leds.setPixelColor(i, leds.Color(r,g,b));           
    }
}

// Calcul lever & coucher du soleil
void printRiseAndSet(char *city, FLOAT latitude, FLOAT longitude, int UTCOffset, int day, int month, int year)
{
  Ephemeris::setLocationOnEarth(latitude,longitude);
             
  SolarSystemObject sun = Ephemeris::solarSystemObjectAtDateAndTime(Sun,
                                                                    day,month,year,
                                                                    0,0,0);

    // Print sunrise and sunset if available according to location on Earth
  if( sun.riseAndSetState == RiseAndSetOk )
  {
    int hours,minutes;
    FLOAT seconds;

    // Convert floating hours to hours, minutes, seconds and display.
    Ephemeris::floatingHoursToHoursMinutesSeconds(Ephemeris::floatingHoursWithUTCOffset(sun.rise,UTCOffset), &hours, &minutes, &seconds);
    Serial.print("  Sunrise: ");
    sunriseHour = hours;
    Serial.print(hours);
    Serial.print("h");
    sunriseMin = minutes;
    Serial.print(minutes);
    Serial.print("m");
    Serial.print(seconds,0);
    Serial.println("s");

    // Convert floating hours to hours, minutes, seconds and display.
    Ephemeris::floatingHoursToHoursMinutesSeconds(Ephemeris::floatingHoursWithUTCOffset(sun.set,UTCOffset), &hours, &minutes, &seconds);
    Serial.print("  Sunset:  ");
    sunsetHour = hours;
    Serial.print(hours);
    Serial.print("h");
    sunsetMin = minutes;
    Serial.print(minutes);
    Serial.print("m");
    Serial.print(seconds,0);
    Serial.println("s");
  }  
}
