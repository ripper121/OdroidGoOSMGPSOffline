# OdroidGoOSMGPSOffline
Odroid Go OSM GPS Offline

   GPS Module Connection (Neo-6M Module)
   
   GND|TX|RX|VCC
   
   GND|04|05|P3V3
   
   ODROID-GO Header(P2)
   
   
How to generate Tiles (offline Map) for your Area:
1. Open Maperitive.exe (http://maperitive.net/)
2. Move the map to your favor position
3. MAP->Set Geometry Bounds
4. MAP->Set Printing Bounds
Now the area of the map you want to Export is set.
5. TOOLS-> Generate Tiles (this can take some time, depending on how high your Zoom Level is)
6. Now you can find some png files in the Maperitive /tiles folder
8. Open Flexxi.exe (https://sourceforge.net/projects/flexxi-image-resizer/)
9. Import the Tiles Folder
10. Resize the Image to 240x240 px (best Fit for GO Screen)
11. Convert to JPG Files
12. Start
13. Now you have your tiles in the correct size and format
14. Copy the "TILES" (all Uppercase) folder to the root directory of your SD card


Forum Thread: https://forum.odroid.com/viewtopic.php?f=162&t=33629

Download Tools: https://drive.google.com/open?id=18JUOutDZzVzfiJgyUtiNcj4cO6Xv8Wn8
Download TILES (sample set): https://drive.google.com/open?id=1ng5wpFVR5ual7zt3KRj-fZQsR8js7pFB
