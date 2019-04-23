#include <odroid_go.h>
#include <FuGPS.h>
HardwareSerial in(1);
FuGPS fuGPS(in);

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_SIZE   240
#define TILE_SIZE 256

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

static inline double long2tilex(double lon, double z)
{
  return (double)((lon + 180.0) / 360.0 * pow(2.0, z));
}

static inline double lat2tiley(double lat, double z)
{
  return (double)((1.0 - log( tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z));
}

static inline double tilex2long(int x, int z)
{
  return x / pow(2.0, z) * 360.0 - 180;
}

static inline double tiley2lat(int y, int z)
{
  double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

// draws tile(s) to display at tileX and tileY. It may load one, two X,
// two Y, or four images (total of 4 possibilities):
//   - @0,0  : img(x,y)@0,0-239,239
//   - @0,0  : img(x,y)@128,0-255,239
//     @128,0: img(x+1,y)@0,0-111,239
//   - @0,0  : img(x,y)@0,128-239,255
//     @0,128: img(x,y+1)@0,0-239,111
//   - @0,0    : img(x,y)@128,128-255,255
//     @128,0  : img(x+1,y)@0,128-111,255
//     @0,128  : img(x,y+1)@128,0-255,111
//     @128,128: img(x+1,y+1)@0,0-111,111
static void drawTile(double tileX, double tileY)
{
    int x, y;

    tileX += tileX_Off;
    tileY += tileY_Off;
    tileX = int(tileX * 2 - 1) / 2.0;
    tileY = int(tileY * 2) / 2.0;
    //redraw only when something has changed
    if (old_tileX == tileX && old_tileY == tileY && old_zoom == zoom)
        return;

    old_tileX = tileX;
    old_tileY = tileY;

    // draw the 4 quadrants
    GO.lcd.clear();
    for (y = 0; y < DISPLAY_SIZE; y += TILE_SIZE/2) {
      for (x = 0; x < DISPLAY_SIZE; x += TILE_SIZE/2) {
        String path = "/TILES/" + String(uint16_t(zoom)) + "/" + String(uint32_t(tileX + (double)x / DISPLAY_SIZE)) + "/" + String(uint32_t(tileY + (double)y / DISPLAY_SIZE)) + ".jpg";
        Serial.println(path);
        if (SD.exists(path)) {
          Serial.println("File found");
          GO.lcd.setCursor(0, 0);
          //drawJpgFile(fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0, uint16_t maxWidth = 0, uint16_t maxHeight = 0, uint16_t offX = 0, uint16_t offY = 0, jpeg_div_t scale = JPEG_DIV_NONE),
          //GO.lcd.drawJpgFile(SD, path.c_str(), (abs(DISPLAY_WIDTH - DISPLAY_SIZE)));
          GO.lcd.drawJpgFile(SD, path.c_str(), x + DISPLAY_WIDTH - DISPLAY_SIZE, y,
                             x ? DISPLAY_SIZE-TILE_SIZE/2 : TILE_SIZE/2,
                             y ? DISPLAY_SIZE-TILE_SIZE/2 : TILE_SIZE/2,
                             int(x+tileX*TILE_SIZE)%TILE_SIZE,
                             int(y+tileY*TILE_SIZE)%TILE_SIZE);
        } else {
          Serial.println("File not found : " + path);
        }
      }
    }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  GO.begin();
  if (0) {
    for (int i=0;i<5;i++) {
      delay(500);
      Serial.printf("Booting...%d\n", i);
    }
  }
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

void loop()
{
  GO.update();
  if (0) {
    static int loops;
    Serial.printf("Running...%d\r", loops++);
  }
  if (GO.JOY_Y.wasAxisPressed() == 2) {
    tileY_Off -= 0.5;
  }
  if (GO.JOY_Y.wasAxisPressed() == 1) {
    tileY_Off += 0.5;
  }
  if (GO.JOY_X.wasAxisPressed() == 2) {
    tileX_Off -= 0.5;
  }
  if (GO.JOY_X.wasAxisPressed() == 1) {
    tileX_Off += 0.5;
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
    posX = (posX * TILE_SIZE);
    posY = (posY * TILE_SIZE);

    if (posX < DISPLAY_SIZE/2 - TILE_SIZE/4) {
      tileX -= 0.5;
      posX  += TILE_SIZE/2;
    }
    else if (posX >= DISPLAY_SIZE/2 + TILE_SIZE/4) {
      tileX += 0.5;
      posX  -= TILE_SIZE/2;
    }

    if (posY < DISPLAY_SIZE/2 - TILE_SIZE/4) {
      tileY -= 0.5;
      posY += TILE_SIZE/2;
    }
    else if (posY >= DISPLAY_SIZE/2 + TILE_SIZE/4) {
      tileY += 0.5;
      posY  -= TILE_SIZE/2;
    }

    drawTile(tileX,tileY);

    if (posX - tileX_Off*TILE_SIZE >= 0              &&
        posX - tileX_Off*TILE_SIZE <  DISPLAY_SIZE   &&
        posY - tileY_Off*TILE_SIZE >= 0              &&
        posY - tileY_Off*TILE_SIZE <  DISPLAY_HEIGHT) {
      GO.lcd.fillCircle(int32_t(posX-tileX_Off*TILE_SIZE) + (abs(DISPLAY_WIDTH - DISPLAY_SIZE)), int32_t(posY-tileY_Off*TILE_SIZE), 4, BLUE);
      GO.lcd.fillCircle(int32_t(posX-tileX_Off*TILE_SIZE) + (abs(DISPLAY_WIDTH - DISPLAY_SIZE)), int32_t(posY-tileY_Off*TILE_SIZE), 2, RED);
    }
    GO.lcd.fillRect(0, 0, abs(DISPLAY_WIDTH - DISPLAY_SIZE), DISPLAY_HEIGHT, BLACK);
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
  }

  old_lat_rad = lat_rad;
  old_lon_deg = lon_deg;
  old_tileX_Off = tileX_Off;
  old_tileY_Off = tileY_Off;
  old_zoom = zoom;
}
