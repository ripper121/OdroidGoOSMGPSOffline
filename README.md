# OdroidGoOSMGPSOffline
Odroid Go OSM GPS Offline

GPS Module Connection (Neo-6M Module)
GND|TX|RX|VCC
GND|04|05|P3V3
ODROID-GO Header(P2)


How to generate Tiles (offline Map) for your Area:

URL:
-URL TileServer

Zoom Leve:
-z ZOOM

Bounding Box:
left bottom
-left >min-longitude
-bottom >min-latitude

right top
-right >max-longitude
-top >max-latitude

Example >>
`TileDownloader.exe -URL https://a.tile.openstreetmap.org/${z}/${x}/${y}.png -z 5 -left -0.489 -botton 51.28 -right 0.236 -top 51.686`
Tile servers: https://wiki.openstreetmap.org/wiki/Tile_servers

If your arguments look like that `Arguments: -z 5 -left -489 -botton 0 -right 236 -top 51686` use a ',' instead of a '.' (depends on your region setting.)


Forum Thread: https://forum.odroid.com/viewtopic.php?f=162&t=33629
Download TILES (sample set): https://drive.google.com/open?id=1ng5wpFVR5ual7zt3KRj-fZQsR8js7pFB
