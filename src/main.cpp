#include <Arduino.h>
#include <Wire.h>
#include <string.h>

// libaries for components
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

// other scource files
#include "Utility.hpp"
#include "Games.hpp"

// led strip setup
const int LED_PIN = 33;
const int LED_COUNT = 12;
const int LED_BRIGHTNESS = 10;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// lcd display setup
const int LCD_COLS = 16;
const int LCD_ROWS = 2;
const int I2C_SDA = 21;
const int I2C_SCL = 22;

LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

// other pins
const int JOYSTICK_X = 27;
const int JOYSTICK_Y = 26;
const int JOYSTICK_DOWN = 25;

const int PAUSE_PIN = 32;
const int MAIN_MENU_PIN = 35;
const int SELECT_PIN = 34;

// all custom chars
namespace CustomChar
{
	byte player[] = {
		B01110,
		B01110,
		B00100,
		B01110,
		B10101,
		B00100,
		B01010,
		B10001};

	byte enemy[] = {
		B00000,
		B10001,
		B01010,
		B00100,
		B01010,
		B10001,
		B00000,
		B00000};

	byte star[] = {
		B00000,
		B00100,
		B01110,
		B11111,
		B01110,
		B00100,
		B00000,
		B00000};

	byte arrow_left[] = {
		B00000,
		B00100,
		B01100,
		B11111,
		B11111,
		B01100,
		B00100,
		B00000};

	byte arrow_right[] = {
		B00000,
		B00100,
		B00110,
		B11111,
		B11111,
		B00110,
		B00100,
		B00000};

	int char_count = 5;
	byte *chars[] = {
		arrow_left,
		arrow_right,
		player,
		enemy,
		star};
};

// scene manager / main menu
namespace SceneManager
{
	// swicher vars
	int current = 0;
	int last_current = 0;

	bool pressed_select = false;
	bool pressed_home = false;

	// pause menu
	bool pressed_paused = false;

	void resume_animation(int d)
	{
		String text = " - - PAUSED - - ";

		for (int i = 0; i < 16; i++)
		{
			lcd.clear();
			lcd.setCursor(i, 0);

			lcd.print(text);

			delay(d);
		}
	}

	// main menu
	int selected = 0;
	int last_selected = 0;
	int curser_pos = 0;

	// "current" value should correspond to the index + 2 of this list
	String menu_items[] = {
		"test game",
		"noch game",
		"lol",
		"theoretisch"};

	bool menu_items_background[] = {
		false,
		false,
		false,
		false};

	void update()
	{
		if (current == 0)
		{
			// main menu
			last_selected = selected;

			selected += Utility::get_swich();
			selected = ((selected % 4) + 4) % 4;

			selected = min(max(selected, 0), 3);

			// calculate curser pos for centerd text
			curser_pos = 7 - (int)(menu_items[selected].length() / 2);
			curser_pos = min(max(curser_pos, 1), 7);
		}

		else if (current == 1)
		{
			// pause menu
			// calculate curser pos for pause menu
			curser_pos = 7 - (int)(menu_items[last_current - 2].length() / 2);
			curser_pos = min(max(curser_pos, 1), 7);
		}

		else if (current == 2)
		{
			Games::games[0].update();
		}

		else
		{
			// reset scene to main menu if none is selected
			current = 0;
			last_current = 0;
		}

		// check for button inputs
		// control of pause menu / resume animation
		if (pressed_paused && !digitalRead(PAUSE_PIN))
		{
			pressed_paused = false;
		}

		if (digitalRead(PAUSE_PIN) && !pressed_paused)
		{
			pressed_paused = true;

			if (current == 0)
			{
			}

			else if (current != 1)
			{
				last_current = current;
				current = 1;
				lcd.clear();
			}

			else
			{
				resume_animation(100);
				current = last_current;
				lcd.clear();
				Games::games[current - 2].resume();
			}
		}

		// select button
		if (pressed_select && !digitalRead(SELECT_PIN))
		{
			pressed_select = false;
		}

		if (digitalRead(SELECT_PIN) && !pressed_select)
		{
			pressed_select = true;

			current = selected + 2;
			lcd.clear();

			if (!menu_items_background[selected])
			{
				menu_items_background[selected] = true;
			}

			else
			{
				Games::games[current - 2].resume();
			}
		}

		// home button
		if (pressed_home && !digitalRead(MAIN_MENU_PIN))
		{
			pressed_home = false;
		}

		if (digitalRead(MAIN_MENU_PIN) && !pressed_home)
		{
			pressed_home = true;

			current = 0;
			lcd.clear();
		}
	}

	void render()
	{
		if (current == 0)
		{
			// main menu
			// display selected game
			if (selected != last_selected)
			{
				lcd.clear();
			}

			lcd.setCursor(curser_pos, 1);
			lcd.print(menu_items[selected]);

			// draw arrows
			lcd.setCursor(0, 0);
			lcd.write(byte(0));

			lcd.setCursor(15, 0);
			lcd.write(byte(1));
		}

		else if (current == 1)
		{
			// pause menu
			lcd.setCursor(0, 0);
			lcd.print(" - - PAUSED - - ");

			lcd.setCursor(curser_pos, 1);
			lcd.print(menu_items[last_current - 2]);
		}

		else if (current > 1)
		{
			Games::games[current - 2].render();
		}
	}
}

void setup()
{
	Serial.begin(9600);

	// initialize led strip
	// strip.begin();
	// strip.setBrightness(10);

	// inialize lcd display
	lcd.init(I2C_SDA, I2C_SCL);
	lcd.backlight();

	// create all custom chars
	for (int i = 0; i < CustomChar::char_count; i++)
	{
		lcd.createChar(i, CustomChar::chars[i]);
	}

	// set random seed
	randomSeed(analogRead(3));

	// configure other pins
	pinMode(PAUSE_PIN, INPUT);
	pinMode(MAIN_MENU_PIN, INPUT);
	pinMode(SELECT_PIN, INPUT);
}

void loop()
{
	SceneManager::update();
	SceneManager::render();
}
