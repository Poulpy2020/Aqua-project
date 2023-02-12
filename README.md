# Aqua-project
Automatisation Aquarium 

Self-contained aquarium automation project

Principle:
Reading of the aquarium temperature and sending on a screen, and/or via MQTT
Adaptive day/night lighting;
- Automatic day and night time adjustment
- Gradual day/night light transition
Fixed time food distribution module



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


// ADD Ephemeris.zip lib. to the \Arduino\libraries or your project


// Materiel ;
D1 mini - 
Led type - WS2812B
sonde temp : DS18B20
mini screen SSD1306

// Auto fish eat module 
Step motor 
