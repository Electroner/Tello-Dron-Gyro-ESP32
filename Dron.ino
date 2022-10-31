#include <ESPTelloCLI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

ESPTelloCLI TelloCLI;
Adafruit_MPU6050 mpu;

bool Connected;
bool takeoff;
bool commandSent;
unsigned int delvar;
char flipside;

void WiFiEvent(WiFiEvent_t event)
{
	switch (event)
	{
#ifdef ESP8266
	case WIFI_EVENT_STAMODE_GOT_IP:
#else
	case ARDUINO_EVENT_WIFI_STA_GOT_IP:
#endif
		Serial.print("WiFi connected! IP address: ");
		Serial.println(WiFi.localIP());
		TelloCLI.begin();
		// Turn off telemetry if not needed.
		TelloCLI.setTelemetry(false);
		Connected = true;
		break;
#ifdef ESP8266
	case WIFI_EVENT_STAMODE_DISCONNECTED:
#else
	case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#endif
		Serial.println("WiFi lost connection");
		TelloCLI.end();
		Connected = false;
		break;
	default:
		break;
	}
}

void setup()
{
	Serial.begin(115200);
	Serial.setTimeout(0);

	delvar = 50;
	Connected = false;
	commandSent = false;
	takeoff = false;
	flipside = 'f'; //Front

	WiFi.mode(WIFI_STA);
	WiFi.onEvent(WiFiEvent);

	if (!mpu.begin())
	{
		Serial.println("Sensor init failed");
		while (1)
			yield();
	}

	mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
	mpu.setGyroRange(MPU6050_RANGE_500_DEG);
	mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

	Serial.println("INICIALIZANDO");
}

void loop()
{
	sensors_event_t a, g, temp;
	mpu.getEvent(&a, &g, &temp);

	if (!takeoff)
	{
		if (Serial.available())
		{
			String command = Serial.readStringUntil('\n');
			char SSID[65];
			if (command.length() == 0)
			{
				command = Serial.readStringUntil('\r');
			}
			if (command.length() > 0)
			{
				command.trim();
				if (command.startsWith("delay"))
				{
					command.replace("delay", "");
					command.trim();
					delvar = command.toInt();
					Serial.println(delvar);
				}
				else if (command.equalsIgnoreCase("connect?"))
				{
					if (Connected)
					{
						Serial.print("ok\r\n");
					}
					else
					{
						Serial.print("error\r\n");
					}
				}
				else if (command.startsWith("connect"))
				{
					command.replace("connect", "");
					command.trim();
					Serial.println("COMMAND:");
					Serial.println(command.c_str());
					int blank = command.indexOf(' ');
					if (blank == -1)
					{
						strcpy(SSID, command.c_str());
						WiFi.begin(SSID);
					}
					else
					{
						strcpy(SSID, command.substring(0, blank).c_str());
						command.replace(SSID, "");
						command.trim();
						WiFi.begin(SSID, command.c_str());
					}
					Serial.print("ok\r\n");
				}
				else if (command.equalsIgnoreCase("telemetryon"))
				{
					TelloCLI.setTelemetry(true);
					Serial.print("ok\r\n");
				}
				else if (command.equalsIgnoreCase("telemetryoff"))
				{
					TelloCLI.setTelemetry(false);
					Serial.print("ok\r\n");
				}
				else
				{
					if (Connected)
					{
						if (command.equalsIgnoreCase("takeoff"))
						{
							takeoff = true;
							Serial.println(command);
							TelloCLI.write(command.c_str(), command.length());
						}
						else
						{
							Serial.println(command);
							TelloCLI.write(command.c_str(), command.length());
						}
					}
				}
			}
		}
	}
	else if (takeoff)
	{
		if (touchRead(4) <= 30)
		{
			String command = "rc 0 0 0 0";
			TelloCLI.write(command.c_str(), command.length());
			delay(30);
			command = "land";
			TelloCLI.write(command.c_str(), command.length());
			takeoff = false;
		}
		else if (touchRead(13) <= 30)
		{
			String command = "flip ";
			command += flipside;
			TelloCLI.write(command.c_str(), command.length());
		}
		else if (touchRead(33) <= 30)
		{
			String command = "up 40";
			TelloCLI.write(command.c_str(), command.length());
		}
		else if (touchRead(32) <= 30)
		{
			String command = "down 40";
			TelloCLI.write(command.c_str(), command.length());
		}
		else
		{
			double eje_x = a.acceleration.x * -1 / 9.8;
			double eje_y = a.acceleration.y * 1 / 9.8;

			String X = "0";
			String Y = "0";

			if (eje_x > 0.35 eje_x < -0.35)
			{
				//Direccion x positiva: hacia la derecha, negativa: hacia la izquierda
				X = String(eje_x * 60);

				if(eje_x > 0){
					flipside = 'r';
				}
				else
				{
					flipside = 'l';
				}
			}
			if (eje_y > 0.35 eje_y < -0.35)
			{
				//Direccion y positiva: hacia adelante, negativa: hacia atras
				Y = String(eje_y * 60);

				if(eje_y > 0){
					flipside = 'f';
				}
				else
				{
					flipside = 'b';
				}
			}
			String command = "rc " + Y + " " + X + " 0 0";
			TelloCLI.write(command.c_str(), command.length());
			commandSent = true;
		}
		if (commandSent)
		{
			commandSent = false;
			delay(delvar);
		}
	}
}