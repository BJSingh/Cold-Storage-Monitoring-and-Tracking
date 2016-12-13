# Cold-Storage-Monitoring-and-Tracking
IoT based solution to make delivery of temperature sensitive perishable products more efficient.

The device with GPRS connection has a unique ID so as to recognize which vehicle is it and what the vehicle is carrying. Temperature and Humidity sensor is interfaced to monitor the temperature and humidity periodically. The sensor data along with the Unique ID, GPS co-ordinates and battery level is sent to PubNub Cloud service. So that the temperature and position of each delivery vehicle can be monitored in real time remotely through a pc / smartphone / tablet. RTC is also connected for logging time-based record of the temperatures experienced along with the location. All data values are stored in a SD card memory. Other sensors too can be added like accelerometer to detect door opening.

Components Used: -
1) LinkIt ONE development board

2) Grove Temperature and Humidity sensor (TH02)

3) Grove RTC Module

4) Grove Base Shield for LinkIt ONE

5) GPS and GSM Antenna

6) Li-ion Battery

7) 4GB Micro SD Card for data logging

8) 2G Sim Card with Internet Plan
