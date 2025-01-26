#ifndef UTILITY_CPP
#define UTILITY_CPP

#include <Arduino.h>
#include <Wire.h>
#include <string.h>

// libaries for components
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

extern const int JOYSTICK_X;
extern const int JOYSTICK_Y;
extern const int JOYSTICK_DOWN;

extern LiquidCrystal_I2C lcd;

extern const int LED_COUNT;

extern Adafruit_NeoPixel strip;

namespace Utility
{
  // displays blinking text with some parameters
  // text should be 16 chas long and centerd: "      TEST      "
  // 											"1234567890123456"
  void blinkText(String text, int times, int d)
  {
    lcd.clear();

    for (int i = 0; i < times; i++)
    {
      lcd.setCursor(0, 0);
      lcd.print(text);
      delay(d);
      lcd.clear();
      delay(d);
    }
  }

  // compares two int arrays (2d) (16x2)
  // returns true if equal or false if not equal
  bool compare2dIntArrays(int arr1[16][2], int arr2[16][2])
  {
    for (int x = 0; x < 16; x++)
    {
      if (arr1[x][0] != arr2[x][0] || arr1[x][1] != arr2[x][1])
      {
        return false;
      }
    }

    return true;
  }

  // checks if the joistick is pressed
  const int last_count = 5;
  int last_read[last_count];
  bool is_down(float threshold = 10)
  {
    float read = analogRead(JOYSTICK_DOWN);
    float temp;

    float sum = 0;

    // shift all elements over and insert the most recent one in the beginning
    for (int i = 0; i < last_count; i++)
    {
      sum += temp;

      temp = last_read[i];
      last_read[i] = read;
      read = temp;
    }

    if (sum > threshold * last_count)
    {
      return true;
    }

    return false;
  }

  // gets the direction of the Joystick
  String get_direction(float threshold = 0.6f)
  {
    // threshold from 0 - 1

    // get axis (-2048 to 2048)
    float x, y;
    x = analogRead(JOYSTICK_X) - 2048;
    y = analogRead(JOYSTICK_Y) - 2048;

    // normalize values
    x = x / 2048;
    y = y / 2048;

    // Serial.print(x);
    // Serial.print(", ");
    // Serial.println(y);

    // apply threshold
    if (abs(x) < threshold && abs(y) < threshold)
    {
      return "center";
    }

    if (abs(x) > abs(y))
    {
      if (x >= 0)
      {
        return "down";
      }
      else
      {
        return "up";
      }
    }
    else
    {
      if (y >= 0)
      {
        return "left";
      }
      else
      {
        return "right";
      }
    }
  }

  // for menu swicher to change selected game
  bool is_not_center = false;
  int get_swich(float threshold = 0.6f)
  {
    String direction = get_direction(threshold);

    if (direction == "center")
    {
      is_not_center = false;
      return 0;
    }

    if (is_not_center)
    {
      return 0;
    }

    if (direction == "right")
    {
      is_not_center = true;
      return 1;
    }

    else if (direction == "left")
    {
      is_not_center = true;
      return -1;
    }

    return 0;
  }

  // lights up the strip in a color for a certain time
  void flash(uint32_t color, int d, int times = 1)
  {
    for (int i = 0; i < times; i++)
    {
      strip.fill(color, 0, LED_COUNT);
      delay(d);
      strip.fill(strip.Color(0, 0, 0));
    }
  }

  // lights up the strip one by one in a color with a certain delay between
  void colorWipe(uint32_t color, int d, int start = 0, int count = LED_COUNT)
  {
    strip.fill(strip.Color(0, 0, 0));

    for (int i = start; i < count; i++)
    {
      strip.setPixelColor(i, color);
      strip.show();
      delay(d);
    }
  }
};

#endif