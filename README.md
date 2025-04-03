# This Repository contain Examples to use ESP32S3 WROOM Board with IDF environment.
## **1) WiFi Base.**
### A simple Wifi Server which connect to local WiFi. The server provides functionality to control switch on\off of a GPIO.
## **2) my_project_ota**
### A simple OTA upgrade demo where a HELLO.bin is read from the SD CARD and updated on top of the current image. The Hello.bin does nothing other than log "hello World" in different ESP_LOG* levels and then reboots.
### NOTE: No anti-rollback and failure\corruption handling implemented.
## **3) my_project_cam**
### Example web cam server using ESP32S3 with OV2640 sensor. Additional Capture button for JPEG.
## **4) USBMS**
### Example usbms device enumeration using tinyffs ###
## **5) SNTP (Simple Network Time Protocol) 
### Example sntp code which syncs the ESP32 system time with SNTP servers and prints the new time.
## **6) Capacative Touch Example
### Example code with acts as a capacitive switch which switches on/off GPIO.
## **7) GPIO ISR
### Example code to gpio isr to be triggered on low on external button press.
## **8) Cam Server Improvements.
### Enhancement to Cam server to allow setting certain controls from captive web page.
## **9) Object Detection and probablity score using google image api.
### Demo to stream image to user and provide button for analysis. On press Analyze, stream stop and image captured and sent to google api. Parse the response and update analysis and probablity score to user.
