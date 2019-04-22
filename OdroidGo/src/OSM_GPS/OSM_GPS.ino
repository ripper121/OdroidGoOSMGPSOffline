#include <odroid_go.h>
#include <FuGPS.h>
HardwareSerial in(1);
FuGPS fuGPS(in);

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define TILE_SIZE 240

bool gpsAlive = false;
double zoom = 16;
double lat_rad = 0, lon_deg = 0;
double tileX = 0, tileY = 0, tileX_Off = 0, tileY_Off = 0;
double old_lat_rad = 0, old_lon_deg = 0;
double old_zoom = 10;
double old_tileY = 0, old_tileX = 0, old_tileX_Off = 0, old_tileY_Off = 0;
uint8_t brightness = 127;

/*
   GPS Module Connection
   GND|TX|RX|VCC
   GND|04|05|P3V3
   ODROID-GO Header(P2)
*/


void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  GO.begin();
  GO.Speaker.setVolume(0);
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);
  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);
  in.begin(9600, SERIAL_8N1, 15, 4);
  fuGPS.sendCommand(FUGPS_PMTK_SET_NMEA_UPDATERATE_1HZ);
  fuGPS.sendCommand(FUGPS_PMTK_API_SET_NMEA_OUTPUT_RMCGGA);
  GO.battery.setProtection(true);
  GO.lcd.clear();
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("OSM GPS Offline Maps, by Ripper121\n");

  GO.lcd.println("GPS Module Connection");
  GO.lcd.println("GND|TX|RX|VCC");
  GO.lcd.println("GND|04|05|P3V3");
  GO.lcd.println("ODROID-GO Header(P2)\n");

  if (!SD.begin()) {
    GO.lcd.println("Card Mount Failed");
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    GO.lcd.println("No SD card attached");
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    GO.lcd.println("MMC");
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    GO.lcd.println("SDSC");
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    GO.lcd.println("SDHC");
    Serial.println("SDHC");
  } else {
    GO.lcd.println("UNKNOWN");
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  GO.lcd.printf("SD Card Size: %lluMB\n", cardSize);

  delay(5000);

  GO.lcd.clear();
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("Wait for GPS...");
}

//setBrightness(uint8_t brightness),
void loop()
{
  GO.update();
  if (GO.JOY_Y.wasAxisPressed() == 2) {
    tileY_Off--;
  }
  if (GO.JOY_Y.wasAxisPressed() == 1) {
    tileY_Off++;
  }
  if (GO.JOY_X.wasAxisPressed() == 2) {
    tileX_Off--;
  }
  if (GO.JOY_X.wasAxisPressed() == 1) {
    tileX_Off++;
  }

  if (GO.BtnA.wasPressed() == 1) {
    zoom++;
    tileX_Off = tileY_Off = 0;
  }
  if (GO.BtnB.wasPressed() == 1) {
    zoom--;
    tileX_Off = tileY_Off = 0;
  }
  if (GO.BtnSelect.isPressed() == 1) {
    GO.lcd.setBrightness(brightness);
    brightness--;
  }
  if (GO.BtnStart.isPressed() == 1) {
    GO.lcd.setBrightness(brightness);
    brightness++;
  }

  if (GO.BtnMenu.wasPressed() == 1) {
    tileX_Off = tileY_Off = 0;
  }


  if (tileX_Off > 1000)
    tileX_Off = 1000;
  if (tileX_Off < -1000)
    tileX_Off = -1000;
  if (tileY_Off > 1000)
    tileY_Off = 1000;
  if (tileY_Off < -1000)
    tileY_Off = -1000;
  if (zoom > 17)
    zoom = 17;
  if (zoom < 5)
    zoom = 5;
  if (brightness > 254)
    brightness = 254;
  if (brightness < 1)
    brightness = 1;

  // Valid NMEA message
  if (fuGPS.read())
  {
    // We don't know, which message was came first (GGA or RMC).
    // Thats why some fields may be empty.

    gpsAlive = true;

    Serial.print("Quality: ");
    Serial.println(fuGPS.Quality);

    Serial.print("Satellites: ");
    Serial.println(fuGPS.Satellites);

    if (fuGPS.hasFix() == true)
    {
      // Data from GGA message
      Serial.print("Accuracy (HDOP): ");
      Serial.println(fuGPS.Accuracy);

      Serial.print("Altitude (above sea level): ");
      Serial.println(fuGPS.Altitude);

      // Data from GGA or RMC
      Serial.print("Location (decimal degrees): ");
      Serial.println(String(fuGPS.Latitude, 6) + "," + String(fuGPS.Longitude, 6));
      lat_rad = fuGPS.Latitude;
      lon_deg = fuGPS.Longitude;
    }
  }

  // Default is 10 seconds
  if (fuGPS.isAlive() == false)
  {
    if (gpsAlive == true)
    {
      gpsAlive = false;
      Serial.println("GPS module not responding with valid data.");
      Serial.println("Check wiring or restart.");
    }
  }

  //redraw only when something has changed
  if (old_lat_rad != lat_rad || old_lon_deg != lon_deg || old_zoom != zoom || old_tileX_Off != tileX_Off || old_tileY_Off != tileY_Off) {
    double posX, posY, fractpart, intpart;
    //calculate from coordinates to tile numbers
    tileX = long2tilex(lon_deg, zoom);
    tileY = lat2tiley(lat_rad, zoom);
    //fractional part is the position of the your coordinats in the tile
    posX = modf(tileX , &intpart);
    posY = modf(tileY , &intpart);
    posX = (posX * TILE_SIZE) + (abs(DISPLAY_WIDTH - TILE_SIZE));
    posY = (posY * TILE_SIZE);

    //redraw only when something has changed
    if (uint16_t(old_tileX) != uint16_t(tileX) || uint16_t(old_tileY) != uint16_t(tileY)  || old_zoom != zoom || old_tileX_Off != tileX_Off || old_tileY_Off != tileY_Off) {
      String path = "/TILES/" + String(uint16_t(zoom)) + "/" + String(uint32_t(tileX + tileX_Off)) + "/" + String(uint32_t(tileY + tileY_Off)) + ".jpg";
      Serial.println(path);
      if (SD.exists(path)) {
        Serial.println("File found.");
        GO.lcd.clear();
        GO.lcd.setCursor(0, 0);
        //drawJpgFile(fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0, uint16_t maxWidth = 0, uint16_t maxHeight = 0, uint16_t offX = 0, uint16_t offY = 0, jpeg_div_t scale = JPEG_DIV_NONE),
        GO.lcd.drawJpgFile(SD, path.c_str(), (abs(DISPLAY_WIDTH - TILE_SIZE)));
      } else {
        GO.lcd.println("");
        GO.lcd.println("Debug:\nFile not found.");
        Serial.println("File not found.");
      }
    }
    if (tileX_Off == 0 && tileY_Off == 0) {
      GO.lcd.fillCircle(int32_t(posX), int32_t(posY), 4, BLUE);
      GO.lcd.fillCircle(int32_t(posX), int32_t(posY), 2, RED);
    }
    GO.lcd.fillRect(0, 0, abs(DISPLAY_WIDTH - TILE_SIZE), DISPLAY_HEIGHT, BLACK);
    GO.lcd.setCursor(0, 0);
    GO.lcd.println("Battery:" + String(GO.battery.getPercentage()) + "%");
    GO.lcd.println("GPS Fix:" + String(fuGPS.hasFix()));
    GO.lcd.println("Quality:" + String(fuGPS.Quality));
    GO.lcd.println("Satell.:" + String(fuGPS.Satellites));
    GO.lcd.println("Accuracy:");
    GO.lcd.println(fuGPS.Accuracy);
    GO.lcd.println("Altitude:");
    GO.lcd.println(fuGPS.Altitude);
    GO.lcd.println("Lon_deg:");
    GO.lcd.println(String(lon_deg, 6));
    GO.lcd.println("Lat_rad:");
    GO.lcd.println(String(lat_rad, 6));
    GO.lcd.println("Zoom:");
    GO.lcd.println(String(zoom));
    GO.lcd.println("tileXOff:");
    GO.lcd.println(String(tileX_Off));
    GO.lcd.println("tileYOff:");
    GO.lcd.println(String(tileY_Off));
    GO.lcd.println("Speed:");
    GO.lcd.println(fuGPS.Speed);
    //GO.lcd.println("Course:");
    //GO.lcd.println(fuGPS.Course);
    GO.lcd.println("Time:");
    GO.lcd.println(String(fuGPS.Hours) + ":" + String(fuGPS.Minutes) + ":" + String(fuGPS.Seconds));
    GO.lcd.println("");
    GO.lcd.setTextSize(4);
    GO.lcd.setTextColor(GREEN);
    if (fuGPS.Speed*1.852 < 10)
      GO.lcd.println(fuGPS.Speed*1.852, 1);
    else
      GO.lcd.println(fuGPS.Speed*1.852, 0);
    GO.lcd.setTextSize(1);
    GO.lcd.setTextColor(WHITE);

    Serial.println(String(tileX, 6));
    Serial.println(String(tileY, 6));
    Serial.println(String(posX));
    Serial.println(String(posY));
    Serial.println(zoom);
    Serial.println(String(lon_deg, 6));
    Serial.println(String(lat_rad, 6));

    old_tileX = tileX;
    old_tileY = tileY;
  }

  old_lat_rad = lat_rad;
  old_lon_deg = lon_deg;
  old_tileX_Off = tileX_Off;
  old_tileY_Off = tileY_Off;
  old_zoom = zoom;
}

double long2tilex(double lon, double z)
{
  return (double)((lon + 180.0) / 360.0 * pow(2.0, z));
}

double lat2tiley(double lat, double z)
{
  return (double)((1.0 - log( tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z));
}

double tilex2long(int x, int z)
{
  return x / pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, int z)
{
  double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}
