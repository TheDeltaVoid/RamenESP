#include <Arduino.h>
#include <Wire.h>
#include <string.h>

// libaries for components
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

// other scource files
#include "Utility.hpp"

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
    int enemy_prob = 10;
    int star_prob = 20;

    int player_pos[2];

    // some counters for scores
    int star_count;

    int level_count;
    int level_count_gaol;

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
        if (random(star_prob) == 0)
        {
          map[x][0] = 3;
        }

        if (random(enemy_prob) == 0)
        {
          map[x][0] = 2;
        }
      }

      // generate bottom of the map
      for (int x = 0; x < 16; x++)
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

    void generateMapWithPlayer()
    {
      this->generateMap();

      int *pos = this->randomPos();
      player_pos[0] = pos[0];
      player_pos[1] = pos[1];
    }

    void wonRound()
    {
      int leds = (int)(level_count / level_count_gaol * LED_COUNT);

      Utility::colorWipe(strip.Color(0, 255, 0), 50, 0, leds);

      Utility::blinkText("   WON  ROUND   ", 3, 200);

      this->generateMapWithPlayer();
    }

    void wonGame()
    {
      Utility::flash(strip.Color(0, 255, 0), 200, 3);

      Utility::blinkText("    WON GAME    ", 4, 300);

      this->generateMapWithPlayer();
    }

    void lostGame()
    {
      Utility::flash(strip.Color(255, 0, 0), 100, 5);

      Utility::blinkText("   LOST  GAME   ", 3, 200);

      this->generateMap();

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
      level_count_gaol = 10;

      // generate random map
      this->generateMap();

      // select random pos for player
      int *pos = this->randomPos();
      player_pos[0] = pos[0];
      player_pos[1] = pos[1];
    }

    void update() override
    {
      direction = Utility::get_direction();

      if (direction == "center")
      {
        walked = false;
      }

      else if (!walked)
      {
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
        this->lostGame();
      }

      else if (tile == 3)
      {
        star_count += 1;
      }

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
  };

  BaseGame *games =
      {
          new LuckyCollector()};

  void resetGame(int index)
  {
    games[index].init();
  }
}

// scene manager / main menu
namespace SceneManager
{
  int current = 0;
  int last_current = 0;

  bool pressed_select = false;

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

      // current = selected + 2;
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

    else if (current == 2)
    {
      Games::games[0].render();
    }
  }
}

void setup()
{
  Serial.begin(9600);

  // initialize led strip
  strip.begin();
  strip.setBrightness(10);

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
