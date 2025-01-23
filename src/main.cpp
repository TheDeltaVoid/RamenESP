#include <Arduino.h>
#include <Wire.h>
#include <string.h>

// libaries for components
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

// led strip setup
#define LED_PIN 33
#define LED_COUNT 12
#define LED_BRIGHTNESS 10

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// lcd display setup
#define LCD_COLS 16
#define LCD_ROWS 2
#define I2C_SDA 21
#define I2C_SCL 22

LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

// other pins
#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define JOYSTICK_DOWN 25

#define PAUSE_PIN 32
#define MAIN_MENU_PIN 35
#define SELECT_PIN 34

// utility functions
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
    int x, y;
    x = analogRead(JOYSTICK_X) - 2048;
    y = analogRead(JOYSTICK_Y) - 2048;

    // normalize values
    x = x / 2048;
    y = y / 2048;

    /*
    Serial.print(x);
    Serial.print(", ");
    Serial.println(y);
    */

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
      // calculate curser pos for centerd text
      curser_pos = 8 - (int)(menu_items[selected].length() / 2);
      curser_pos = min(max(curser_pos, 1), 7);

      selected += Utility::get_swich();
      selected %= 4;

      selected = min(max(selected, 0), 3);
    }

    else if (current == 1)
    {
      // pause menu
      // calculate curser pos for pause menu
      curser_pos = 8 - (int)(menu_items[last_current - 2].length() / 2);
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

      current = selected + 2;
      lcd.clear();
    }
  }

  void render()
  {
    if (current == 0)
    {
      // main menu
      // display selected game
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
  //lcd.setCursor(0, 0);
  //lcd.print("test 1");
  delay(100);
  Serial.println(analogRead(26));

  SceneManager::update();
  SceneManager::render();
}
