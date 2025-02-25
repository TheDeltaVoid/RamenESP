#ifndef GAMES_HPP
#define GAMES_HPP

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

#include "Utility.hpp"

// pins
extern const int JOYSTICK_X;
extern const int JOYSTICK_Y;
extern const int JOYSTICK_DOWN;

extern const int PAUSE_PIN;
extern const int MAIN_MENU_PIN;
extern const int SELECT_PIN;

// lcd
extern const int LCD_COLS;
extern const int LCD_ROWS;

extern LiquidCrystal_I2C lcd;

// led strip
extern const int LED_COUNT;

extern Adafruit_NeoPixel strip;

// base game class
// other game classes should be derived from this one
class BaseGame
{
public:
	// both are called once per tick
	virtual void init()
	{
	}

	virtual void update()
	{
	}

	virtual void render()
	{
	}

	virtual void resume()
	{
	}
};

// all Games
namespace Games
{
	/*
	class TestGame : public BaseGame
	{
	private:
	  // put game variables / functions here
	  int some_var;

	public:
	  TestGame()
	  {
		this->init();
	  }

	  // to allow easier reset functionality
	  void init() override
	  {
		some_var = 0;
	  }

	  // usage of virtual functions to store all classes in one list (useful for game swicher)
	  void update() override
	  {
	  }

	  void render() override
	  {
		lcd.setCursor(0, 0);
		lcd.print("im test game");
	  }

	  void resume() override
	  {
	  	// render again after pause menu because display has been cleared
	  }
	};
	*/

	/*
	class Shoot : public BaseGame
	{
	private:
		int map[16][2];
		int player_pos[2];

	public:
		Shoot()
		{
			this->init();
		}

		void init() override
		{
			player_pos = {2, 0};

			for (int x = 0; x < 16; x++)
			{
				map[x][0] = 0;
				map[x][1] = 0;
			}
		}

		void update() override
		{
			String direction = Utility::get_direction();

			if (direction != "center")
			{
				if (direction == "down") // right (youre supposed to turn the console)
				{
					player_pos[1] += 1;
				}

				if (direction == "up") // left
				{
					player_pos[1] -= 1;
				}

				player_pos[1] = Utility::min(Utility::max(player_pos[1], 0), 1);
			}
		}

		void render() override
		{

		}

		void resume() override
		{

		}
	};
	*/

	class LuckyCollector : public BaseGame
	{
	private:
		// map tiles: 0 = empty, 1 = player, 2 = enemy, 3 = star
		int map[16][2];
		int last_map[16][2];
		bool changed;

		// probabilities for map generation (0-100)
		int enemy_prob = 40;
		int star_prob = 60;

		int player_pos[2];

		// some counters for scores
		int star_count;

		int level_count;
		int level_count_gaol;

		int death_count;
		int lives;

		bool lost = false;

		// vars for movement
		String direction;
		bool walked = false;

		int *randomPos()
		{
			int x = random(17);
			int y = random(2);

			static int pos[2] = {x, y};

			return pos;
		}

		void generateMap()
		{
			// clear map
			for (int x = 0; x < 16; x++)
			{
				map[x][0] = 0;
				map[x][1] = 0;
			}

			// generate top of the map
			for (int x = 0; x < 16; x++)
			{
				if (map[x][0] != 1)
				{
					if (Utility::chance(star_prob))
					{
						map[x][0] = 3;
					}

					if (Utility::chance(enemy_prob))
					{
						map[x][0] = 2;
					}
				}
			}

			// generate bottom of the map
			for (int x = 0; x < 16; x++)
			{
				if (map[x][1] != 1)
				{
					if (random(star_prob) == 0)
					{
						map[x][1] = 3;
					}

					// makes shure you can go anywhere on the map
					else if (map[x][0] != 2 && map[max(x - 1, 0)][0] != 2 && map[min(x + 1, 15)][0] != 2)
					{
						map[x][1] = 2;
					}
				}
			}

			// set current player pos on the map
			map[player_pos[0]][player_pos[1]] = 1;
		}

		void generateMapWithPlayer()
		{
			int *pos = this->randomPos();
			player_pos[0] = pos[0];
			player_pos[1] = pos[1];

			this->generateMap();
		}

		void wonRound()
		{
			Utility::flash(strip.Color(0, 255, 0), 200, 3);

			Utility::blinkText("   WON  ROUND   ", 3, 200);

			this->generateMapWithPlayer();
		}

		void wonGame()
		{
			Utility::flash(strip.Color(0, 255, 0), 200, 3);

			Utility::blinkText("    WON GAME    ", 4, 300);

			this->generateMapWithPlayer();
		}

		void lostRound()
		{
			Utility::flash(strip.Color(255, 0, 0), 200, 3);

			Utility::blinkText("   LOST ROUND   ", 3, 200);

			this->generateMapWithPlayer();
		}

		void lostGame()
		{
			Utility::flash(strip.Color(255, 0, 0), 200, 3);

			Utility::blinkText("    LOST  GAME    ", 3, 200);

			this->generateMapWithPlayer();
		}

	public:
		LuckyCollector()
		{
			this->init();
		}

		void init() override
		{
			// reset score
			star_count = 0;
			level_count = 0;
			level_count_gaol = 5;
			
			death_count = 0;
			lives = 5;

			// generate random map
			this->generateMap();

			// select random pos for player
			int *pos = this->randomPos();
			player_pos[0] = pos[0];
			player_pos[1] = pos[1];
		}

		void update() override
		{
			if (lost)
			{
				lost = false;

				if (death_count >= lives)
				{
					death_count = 0;
					this->lostGame();
				}

				else
				{
					this->lostRound();
				}
			}

			direction = Utility::get_direction();

			if (direction == "center")
			{
				walked = false;
			}

			else if (!walked)
			{
				// clear the players last pos
				map[player_pos[0]][player_pos[1]] = 0;

				// update player pos
				if (direction == "right")
				{
					player_pos[0] += 1;
				}

				else if (direction == "left")
				{
					player_pos[0] -= 1;
				}

				else if (direction == "up")
				{
					player_pos[1] -= 1;
				}

				else if (direction == "down")
				{
					player_pos[1] += 1;
				}

				walked = true;
			}

			// keep player on screen
			player_pos[0] = min(max(player_pos[0], 0), 15);
			player_pos[1] = min(max(player_pos[1], 0), 1);

			// check what tile the player is standing on
			int tile = map[player_pos[0]][player_pos[1]];
			if (tile == 2)
			{
				level_count = 0;
				star_count = 0;

				death_count += 1;
				lost = true;
			}

			else if (tile == 3)
			{
				star_count += 1;
			}

			// move the player on the map
			map[player_pos[0]][player_pos[1]] = 1;

			// check if player has won the round
			bool won = true;
			for (int x = 0; x < 16; x++)
			{
				if (map[x][0] == 3 || map[x][1] == 3)
				{
					won = false;
					break;
				}
			}

			if (won)
			{
				level_count += 1;
				star_count = 0;
				this->wonRound();
			}

			// check if player has won the game
			if (level_count >= level_count_gaol)
			{
				level_count = 0;
				star_count = 0;
				death_count = 0;
				this->wonGame();
			}

			if (!Utility::compare2dIntArrays(map, last_map))
			{
				changed = true;

				for (int x = 0; x < 16; x++)
				{
					last_map[x][0] = map[x][0];
					last_map[x][1] = map[x][1];
				}
			}
		}

		void render() override
		{
			// display stats on LED ring
			Utility::drawLED(strip.Color(255, 255, 255), 0);
			Utility::drawLED(strip.Color(255, 255, 255), 5);

			Utility::colorWipe(strip.Color(0, 255, 0), 0, 1, level_count);
			Utility::colorWipe(strip.Color(255, 0, 0), 0, 6, death_count);

			// only redraw if something has changed
			if (changed)
			{
				changed = false;

				lcd.clear();
				for (int x = 0; x < 16; x++)
				{
					for (int y = 0; y < 2; y++)
					{
						if (map[x][y] == 1)
						{
							lcd.setCursor(x, y);
							lcd.write(byte(2));
						}

						if (map[x][y] == 2)
						{
							lcd.setCursor(x, y);
							lcd.write(byte(3));
						}

						if (map[x][y] == 3)
						{
							lcd.setCursor(x, y);
							lcd.write(byte(4));
						}
					}
				}
			}
		}

		void resume() override
		{
			// to re-render after pause menu
			changed = true;
		}
	};
	
	class System : public BaseGame
	{
	private:
		String items[1];
		int selected;
		int last_selected;
		int item_count;

	public:
		System()
		{
			this->init();
		}

		void init() override
		{
			items[0] = {"Neustart"};

			selected = 0;
			last_selected = 0;
			item_count = 1;
		}

		void update() override
		{
			last_selected = selected;

			selected += Utility::get_swich();
			selected = ((selected % item_count) + item_count) % item_count;

			if (Utility::is_down())
			{
				// reboot
				if (selected == 0)
				{
					ESP.restart();
				}
			}
		}

		void render() override
		{
			if (selected != last_selected)
			{
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.print(items[selected]);
			}
		}
	};

	BaseGame *games[] = {
		new LuckyCollector(),
		new System()};

	void resetGame(int index)
	{
		games[index]->init();
	}
}

#endif