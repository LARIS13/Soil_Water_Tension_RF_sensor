# Soil_Water_Tension_RF_sensor
Implementation of Watermark sensor using arduino, attiny85 and RF modules including:

C++ code  
KiCad blueprints for PCB  
LaTeX document explaining theory, limitations and implementation of water measurement techniques.


# Dependencies:  
Manchester RF library: https://github.com/mchr3k/arduino-libs-manchester  
tinysnore is only used to sleep Attiny85 for more than the max Watchdog timer time, which is around 8 seconds: https://github.com/connornishijima/TinySnore
