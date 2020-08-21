# PlantInoWifi
Plant Watering and Monitoring system

Developed for Arduino, specifically i am using a NodeMCU (ESP8266) Board.

The software checks soil moisture and waters when needed, as configured.
The software can also optionally upload data to Thingspeak, Serve a local website and send data do a MQTT broker.

The code also uses a little reverse voltage trick to minimize galvanization on the moisture probes ( i am using two zinc-covered nails )

The website allows simple configuration and monitoring.

The circuit schematic is simple and might vary depending on the devices you use, feel free to contact me if you need the schematic i am using, hopefully some day i will upload it here anyway.
