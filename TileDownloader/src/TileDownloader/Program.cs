using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
namespace TileDownloader
{
    class Program
    {
        static void Main(string[] args)
        {            
            double zoom = 0;

            double left = 0, bottom = 0, right = 0, top = 0;
            String sourceURL = "";
            
            if (args == null)
            {

            }
            else
            {
                if (args.Length <= 0)
                {
                    Console.WriteLine("URL:\n-URL TileServer\nZoom Leve:\n-z ZOOM\n\nBounding Box:\nleft bottom\n-left >min-longitude\n-bottom >min-latitude\nright top\n-right >max-longitude\n-top >max-latitude\n");
                    Console.WriteLine("Example >>TileDownloader.exe -URL https://a.tile.openstreetmap.org/${z}/${x}/${y}.png -z 5 -left -0.489 -botton 51.28 -right 0.236 -top 51.686");
                    Console.WriteLine("Tile servers: https://wiki.openstreetmap.org/wiki/Tile_servers");
                    Environment.Exit(0);
                }
                else
                {
                    byte argCount = 0;
                    for (int i = 0; i < args.Length; i++)
                    {
                        string argument = args[i];
                        if (argument.Equals("-z"))
                        {
                            zoom = Convert.ToDouble(args[i + 1]);
                            argCount++;
                        }

                        if (argument.Equals("-left"))
                        {
                            left = Convert.ToDouble(args[i + 1]);
                            argCount++;
                        }

                        if (argument.Equals("-bottom"))
                        {
                            bottom = Convert.ToDouble(args[i + 1]);
                            argCount++;
                        }

                        if (argument.Equals("-right"))
                        {
                            right = Convert.ToDouble(args[i + 1]);
                            argCount++;
                        }

                        if (argument.Equals("-top"))
                        {
                            top = Convert.ToDouble(args[i + 1]);
                            argCount++;
                        }

                        if (argument.Equals("-URL"))
                        {
                            sourceURL = args[i + 1];
                            argCount++;
                        }
                    }

                    if (argCount < 5)
                    {
                        Console.WriteLine("To few Arguments");
                    }
                    else
                    {
                        Console.WriteLine("Arguments: -URL " + sourceURL + " -z " + zoom + " -left " + left + " -botton " + bottom + " -right " + right + " -top " + top);
                    }
                }
            }
               
            
            Int32 tileLeft = Convert.ToInt32(long2tilex(left, zoom));
            Int32 tileBottom = Convert.ToInt32(lat2tiley(bottom, zoom));
            Int32 tileRight = Convert.ToInt32(long2tilex(right, zoom));
            Int32 tileTop = Convert.ToInt32(lat2tiley(top, zoom));
            double tileCount = 0, downloadCount = 0;

            for (int tileX = tileLeft; tileX <= tileRight; tileX++)
            {
                for (int tileY = tileTop; tileY <= tileBottom; tileY++)
                {
                    tileCount++;
                }
            }
            Console.WriteLine("Download Tile Count:" + tileCount + "? Y/N");

            ConsoleKeyInfo info = Console.ReadKey();
            if (info.KeyChar == 'y' || info.KeyChar == 'Y')
            {
                for (int tileX = tileLeft; tileX <= tileRight; tileX++)
                {
                    for (int tileY = tileTop; tileY <= tileBottom; tileY++)
                    {
                        String URL = sourceURL.Replace("${z}", zoom.ToString());
                        URL = URL.Replace("${x}", tileX.ToString());
                        URL = URL.Replace("${y}", tileY.ToString());

                        if (!Download(URL, zoom, tileX, tileY))
                        {
                            Console.WriteLine("Download Error:" + URL);
                        }
                        else
                        {
                            downloadCount++;
                            Console.WriteLine("Download:" + downloadCount + "/" + tileCount);
                        }
                    }
                }

                Console.WriteLine("Download Finished");
            }
            Environment.Exit(0);

        }

        static bool Download(String URL, double zoom, double tileX, double tileY)
        {
            try
            {
                String filePath = AppDomain.CurrentDomain.BaseDirectory + "TILES/" + zoom + "/" + tileX + "/" + tileY + ".jpg";

                if (!File.Exists(filePath))
                {
                    if (!Directory.Exists(AppDomain.CurrentDomain.BaseDirectory + "/TILES"))
                    {
                        Directory.CreateDirectory("/TILES");
                    }

                    if (!Directory.Exists(AppDomain.CurrentDomain.BaseDirectory + "/TILES/" + zoom))
                    {
                        Directory.CreateDirectory(AppDomain.CurrentDomain.BaseDirectory + "/TILES/" + zoom);
                    }

                    if (!Directory.Exists(AppDomain.CurrentDomain.BaseDirectory + "/TILES/" + zoom + "/" + tileX))
                    {
                        Directory.CreateDirectory(AppDomain.CurrentDomain.BaseDirectory + "/TILES/" + zoom + "/" + tileX);
                    }

                    if (!Directory.Exists(filePath))
                        SaveImage(URL, filePath, ImageFormat.Jpeg, 240, 240, false);
                }
                return true;
            }
            catch (Exception e)
            {
                Console.WriteLine("Error:" + e.Message);
                return false;
            }
        }

        static double long2tilex(double lon, double z)
        {
            return (double)((lon + 180.0) / 360.0 * Math.Pow(2.0, z));
        }

        static double lat2tiley(double lat, double z)
        {
            return (double)((1.0 - Math.Log(Math.Tan(lat * Math.PI / 180.0) + 1.0 / Math.Cos(lat * Math.PI / 180.0)) / Math.PI) / 2.0 * Math.Pow(2.0, z));
        }

        static Image ResizeImage(Image image, int width, int height, bool preserveAspectRatio = true)
        {
            int newWidth;
            int newHeight;
            if (preserveAspectRatio)
            {
                int originalWidth = image.Width;
                int originalHeight = image.Height;
                float percentWidth = (float)width / (float)originalWidth;
                float percentHeight = (float)height / (float)originalHeight;
                float percent = percentHeight < percentWidth ? percentHeight : percentWidth;
                newWidth = (int)(originalWidth * percent);
                newHeight = (int)(originalHeight * percent);
            }
            else
            {
                newWidth = width;
                newHeight = height;
            }
            Image newImage = new Bitmap(newWidth, newHeight);
            using (Graphics graphicsHandle = Graphics.FromImage(newImage))
            {
                graphicsHandle.InterpolationMode = InterpolationMode.HighQualityBicubic;
                graphicsHandle.DrawImage(image, 0, 0, newWidth, newHeight);
            }
            return newImage;
        }

        static void SaveImage(String imageUrl, string filename, ImageFormat format, int width, int height, bool preserveAspectRatio = true)
        {

            WebClient client = new WebClient();
            Stream stream = client.OpenRead(imageUrl);
            Bitmap bitmap; bitmap = new Bitmap(stream);


            Image outputImage = ResizeImage(bitmap, width, height, false);

            if (outputImage != null)
                outputImage.Save(filename, format);

            stream.Flush();
            stream.Close();
            client.Dispose();
        }
    }
}
