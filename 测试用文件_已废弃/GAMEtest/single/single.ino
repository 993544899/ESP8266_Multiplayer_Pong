
/*
  A simple Pong game:
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define UP_BUTTON 14//GOIO14=D5
#define DOWN_BUTTON 12//GOIO12=D6

const unsigned long PADDLE_RATE = 33;
const unsigned long BALL_RATE = 16;
const uint8_t PADDLE_HEIGHT = 24;

int scoreCPU = 0;
int scoreUSER = 0;

#define SCREEN_WIDTH 128 // OLED宽度像素
#define SCREEN_HEIGHT 64 // OLED高度像素

// 0.96寸SSD1306，I2C协议 (SDA, SCL 标号)
#define OLED_RESET     3 // Reset pin ,虽然用不上但是还是要设置
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void drawCourt();
void splash();
void printScreen();
void SinglePlayer();

uint8_t ball_x = 64, ball_y = 32;
uint8_t ball_x0 = 64, ball_y0 = 32;
uint8_t ball_dir_x = 1, ball_dir_y = 1;
unsigned long ball_update;
unsigned long paddle_update;
const uint8_t CPU_X = 12;
uint8_t cpu_y = 16;

const uint8_t PLAYER_X = 115;
uint8_t player_y = 16;

void setup() {
  delay(100);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);//使用I2C地址0x3C初始化（对于128x64）
  display.clearDisplay();
  unsigned long start = millis();
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  digitalWrite(UP_BUTTON, 1);
  digitalWrite(DOWN_BUTTON, 1);
  splash();
  drawCourt();
  printScores();

  while (millis() - start < 2000);
  ball_update = millis();
  paddle_update = ball_update;



}

void loop() {

  SinglePlayer();

}


void drawCourt() {
  display.drawRect(0, 0, 128, 64, WHITE);

  for (int i = 0; i < SCREEN_HEIGHT; i += 4) {
    display.drawFastVLine(SCREEN_WIDTH / 2, i, 2, WHITE);
  }

}

void splash()
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  centerPrint("PONG", 0, 3);
  centerPrint("By BlackBox114", 24, 1);
  centerPrint("Code by", 33, 1);
  centerPrint("TaichiMacker", 42, 1);
  display.fillRect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, WHITE);
  display.setTextColor(BLACK);
  centerPrint("Press key to start!", SCREEN_HEIGHT - 9, 1);

  display.display();


  while (true) {
    ESP.wdtFeed();//长时间循环会触发看门狗复位，因此需要喂狗
    if (digitalRead(DOWN_BUTTON) + digitalRead(UP_BUTTON) < 2) {
      break;
    }
    display.clearDisplay();
  }

  // soundStart();
 
}

void centerPrint(char *text, int y, int size)
{
  display.setTextSize(size);
  display.setCursor(SCREEN_WIDTH / 2 - ((strlen(text)) * 6 * size) / 2, y);
  display.print(text);
}

void printScores() {

  //print scores
  int SIZE = 2;
  int SCORE_PADDING = 10;
  //backwards indent score CPU. This is dirty, but it works ... ;)
  int scoreCPUWidth = 5 * SIZE;
  if (scoreCPU > 9) scoreCPUWidth += 6 * SIZE;
  if (scoreCPU > 99) scoreCPUWidth += 6 * SIZE;
  if (scoreCPU > 999) scoreCPUWidth += 6 * SIZE;
  if (scoreCPU > 9999) scoreCPUWidth += 6 * SIZE;
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH / 2 - SCORE_PADDING - scoreCPUWidth, 10);
  display.print(scoreCPU);

  display.setCursor(SCREEN_WIDTH / 2 + SCORE_PADDING + 2, 10); //+1 because of dotted line.
  display.print(scoreUSER);

  display.display();
}



// void MultiPlayer(){}

void SinglePlayer() {
  while(1){

    bool update = false;
    unsigned long time = millis();

    static bool up_state = false;
    static bool down_state = false;

    up_state |= (digitalRead(UP_BUTTON) == LOW);
    down_state |= (digitalRead(DOWN_BUTTON) == LOW);

    if (time > ball_update) {
      uint8_t new_x = ball_x + ball_dir_x;
      uint8_t new_y = ball_y + ball_dir_y;

    // 如果球撞到电脑的墙
      if (new_x == 0) {
        display.clearDisplay();
        drawCourt();
        scoreUSER++;
        printScores();
        delay(1000);
        new_x = ball_x0 + ball_dir_x;
        new_y = ball_y0 + ball_dir_y;

      }

    // 如果球撞到玩家的墙
      if (new_x == 127) {
        display.clearDisplay();
        drawCourt();
        scoreCPU++;
        printScores();
        delay(1000);
        new_x = ball_x0 + ball_dir_x;
        new_y = ball_y0 + ball_dir_y;


      }

    // 如果球撞到水平的墙
      if (new_y == 0 || new_y == 63) {
        ball_dir_y = -ball_dir_y;
        new_y += ball_dir_y + ball_dir_y;


      }

    //如果球撞到电脑的拍子上
      if (new_x == CPU_X && new_y >= cpu_y && new_y <= cpu_y + PADDLE_HEIGHT) {
        ball_dir_x = -ball_dir_x;
        new_x += ball_dir_x + ball_dir_x;
      }

    // 如果球撞到玩家的拍子上
      if (new_x == PLAYER_X
        && new_y >= player_y
        && new_y <= player_y + PADDLE_HEIGHT)
      {
        ball_dir_x = -ball_dir_x;
        new_x += ball_dir_x + ball_dir_x;
      }

      display.drawPixel(ball_x, ball_y, BLACK);
      display.drawPixel(new_x, new_y, WHITE);
      ball_x = new_x;
      ball_y = new_y;

      ball_update += BALL_RATE;

      update = true;
    }

    if (time > paddle_update) {
      paddle_update += PADDLE_RATE;

    // CPU paddle
      display.drawFastVLine(CPU_X, cpu_y, PADDLE_HEIGHT, BLACK);
      const uint8_t half_paddle = PADDLE_HEIGHT >> 1;
      if (cpu_y + half_paddle > ball_y) {
        cpu_y -= 1;
      }
      if (cpu_y + half_paddle < ball_y) {
        cpu_y += 1;
      }
      if (cpu_y < 1) cpu_y = 1;
      if (cpu_y + PADDLE_HEIGHT > 63) cpu_y = 63 - PADDLE_HEIGHT;
      display.drawFastVLine(CPU_X, cpu_y, PADDLE_HEIGHT, WHITE);

    // Player paddle
      display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, BLACK);
      if (up_state) {
        player_y -= 1;
      }
      if (down_state) {
        player_y += 1;
      }
      up_state = down_state = false;
      if (player_y < 1) player_y = 1;
      if (player_y + PADDLE_HEIGHT > 63) player_y = 63 - PADDLE_HEIGHT;
      display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE);

      update = true;
    }

    if (update)
      display.display();


  }

}
