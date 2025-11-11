#include "temp_hum_sensor.h"
#include "global.h"
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
const int DHTPIN = 2;
const int DHTTYPE = DHT22;

DHT dht(DHTPIN, DHTTYPE);

void temp_hum_monitor(void *pvParameters)
{
    lcd.init();
    lcd.backlight();
    dht.begin();
    SensorData data;

    while (1)
    {
        data.humidity = dht.readHumidity();
        data.temperature = dht.readTemperature();
        xQueueOverwrite(qTempHumi, &data);

        lcd.clear();
        // --- Temperature display ---
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.setCursor(7, 0);
        lcd.print(data.temperature, 1);
        lcd.print(" C");

        // --- Humidity display ---
        lcd.setCursor(0, 1);
        lcd.print("Hum : ");
        lcd.setCursor(7, 1);
        lcd.print(data.humidity, 1);
        lcd.print(" %");

        // --- Condition lines ---
        // Humidity condition
        lcd.setCursor(0, 2);
        if (data.humidity < 50)
            lcd.print("Humidity: DRY");
        else if (data.humidity <= 75)
            lcd.print("Humidity: OK ");
        else
            lcd.print("Humidity: WET");

        // Temperature condition
        lcd.setCursor(0, 3);
        if (data.temperature < 20)
            lcd.print("Temp: COLD   ");
        else if (data.temperature <= 30)
            lcd.print("Temp: NORMAL ");
        else
            lcd.print("Temp: HOT    ");

        // Debug print
        Serial.print("Temp: ");
        Serial.print(data.temperature);
        Serial.print(" C | Hum: ");
        Serial.print(data.humidity);
        Serial.println(" %");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

