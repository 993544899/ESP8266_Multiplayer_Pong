/**********************************************************************
  项目名称/Project          : 【已废弃】
  程序名称/Program name     : 【已废弃】
  团队/Team                 : 太极创客团队 / Taichi-Maker (www.taichi-maker.com)
  作者/Author               : Blackbox114
  日期/Date（YYYYMMDD）     : 20200714
  程序目的/Purpose          :
  实现pong的联机游戏游戏【已废弃，未使用定时器，卡顿严重，数据传输不稳定】
  使用的第三方库/Library
  Adafruit_GFX库
  Adafruit_SSD1306库
  下载请前往
  http://www.taichi-maker.com/homepage/download/#library-download
  -----------------------------------------------------------------------
  接线说明：：
  D1(8266)<------------>SCL(oled)
  D2(8266)<------------>SDA(oled)
  D5(8266)<------------>按键UP引脚
  D6(8266)<------------>按键DOWN引脚
  3V3(8266)<----------->VCC(oled)
  GND(8266)<----------->GND(oled)
  备注：使用的两个按键另一端均为接地（GND）
  具体链接请参照接线图
***********************************************************************/
/*代码只实现了功能，并未做优化和封装，有bug！*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define AP_ssid   "mastergame" //这里改成你设置的接入点名字
#define password  "11111111"  //这里改成你设置的接入点密码

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


IPAddress local_IP(192, 168, 1, 14); //手动设置的开启的网络的ip地址
IPAddress gateway(192, 168, 4, 9); //手动设置的网关IP地址
IPAddress subnet(255, 255, 255, 0); //手动设置的子网掩码

////发送给slave的消息////
String oneLine = " ";//发送给slave的信息

////合并前的信息//
int sstate = 0;//游戏状态。0为
int sclearall = 0;   //清理屏幕标志位
int sball_x = 64;   //球的x坐标
int sball_y = 32;   //球的y坐标
int scpu_y = 16;//master玩家球拍的y坐标
int splayer_y = 16;//本地玩家球拍的y坐标
int sscoreCPU = 0;//master的分数
int sscoreUSER = 0;//本地玩家的分数

WiFiUDP Udp;//实例化WiFiUDP对象
unsigned int localUdpPort = 1234;  // 自定义本地监听端口
unsigned int remoteUdpPort = 4321;  // 自定义远程监听端口
char incomingPacket[4];  // 保存Udp工具发过来的消息

char char_array[30];

uint8_t ball_x = 64, ball_y = 32;
uint8_t ball_x0 = 64, ball_y0 = 32;
uint8_t ball_dir_x = 1, ball_dir_y = 1;
unsigned long ball_update;
unsigned long paddle_update;
const uint8_t CPU_X = 12;
uint8_t cpu_y = 16;

int slaverpadup = 0;
int slaverpaddown = 0;

const uint8_t PLAYER_X = 115;
uint8_t player_y = 16;


void drawCourt();
void splash();
void printScreen();
void SinglePlayer();
void getslaverdata();
void send();
void line();

void setup() {
  //delay(100);

  ////串口初始化////
  Serial.begin(115200);//打开串口
  Serial.println();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);//使用I2C地址0x3C初始化（对于128x64）
  display.clearDisplay();
  unsigned long start = millis();

  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  digitalWrite(UP_BUTTON, 1);
  digitalWrite(DOWN_BUTTON, 1);

  splash();

  ////wifiAP模式初始化////
  WiFi.mode(WIFI_AP);
  //启动AP，并设置账号和密码
  Serial.printf("设置接入点中 ... ");
  //配置接入点的IP，网关IP，子网掩码
  WiFi.softAPConfig(local_IP, gateway, subnet);
  //启动校验式网络（需要输入账号密码的网络）
  WiFi.softAP(AP_ssid, password);

  while (WiFi.softAPgetStationNum() != 1) //等待连接
  {
    delay(500);
    Serial.print(".");
    display.setTextColor(WHITE);
    centerPrint("Waiting for ", 24, 1);
    centerPrint("connection...", 33, 1);
    display.display();
  }
  display.clearDisplay();
  Serial.println("连接成功");

  ////UDP监听服务初始化////
  if (Udp.begin(localUdpPort)) { //启动Udp监听服务
    Serial.println("监听成功");
    //打印本地的ip地址，在UDP工具中会使用到
    //WiFi.localIP().toString().c_str()用于将获取的本地IP地址转化为字符串
    Serial.printf("现在监听本地IP：%s, 本地UDP端口：%d\n", WiFi.softAPIP().toString().c_str(), localUdpPort);
  } else {
    Serial.println("监听失败");
  }


  drawCourt();
  printScores();

  while (millis() - start < 2000);
  ball_update = millis();
  paddle_update = ball_update;



}

void loop() {

  getslaverdata();
  SinglePlayer();
  line();
  tochar();
  send(char_array);




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





void SinglePlayer() {

  bool update = false;
  unsigned long time = millis();

  static bool up_state = false;
  static bool down_state = false;

  static bool sup_state = false;
  static bool sdown_state = false;

  up_state |= (digitalRead(UP_BUTTON) == LOW);
  down_state |= (digitalRead(DOWN_BUTTON) == LOW);

  sup_state |= (slaverpadup == 1);
  sdown_state |= (slaverpaddown == 1);

  if (time > ball_update) {
    uint8_t new_x = ball_x + ball_dir_x;
    uint8_t new_y = ball_y + ball_dir_y;



    // 如果球撞到电脑的墙
    if (new_x == 0) {
      display.clearDisplay();
      drawCourt();
      scoreUSER++;
      sscoreUSER = scoreUSER;
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
      sscoreCPU = scoreCPU;
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

    sball_x = ball_x;
    sball_y = ball_y;
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
    if (sup_state) {
      cpu_y -= 2;
    }
    if (sdown_state) {
      cpu_y += 2;
    }
    sup_state = sdown_state = false;
    if (cpu_y < 2) cpu_y = 2;
    if (cpu_y + PADDLE_HEIGHT > 63) cpu_y = 63 - PADDLE_HEIGHT;
    scpu_y = cpu_y;
    display.drawFastVLine(CPU_X, cpu_y, PADDLE_HEIGHT, WHITE);
    slaverpadup = 0;
    slaverpaddown = 0;


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
    splayer_y = player_y;
    display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE);

    update = true;
  }

  if (update)
    display.display();


}

//udp发送消息////
void send(const char *buffer)
{
  Udp.beginPacket(Udp.remoteIP(), remoteUdpPort);//配置远端ip地址和端口
  Udp.write(buffer); //把数据写入发送缓冲区
  Udp.endPacket(); //发送数据
  //  Serial.printf("本机IP：%s", WiFi.localIP().toString().c_str());
  //  Serial.printf("发送给远程IP：%s", Udp.remoteIP().toString().c_str());
}


////解析Udp数据包////
void getslaverdata() {
  int packetSize = Udp.parsePacket();//获得解析包
  if (packetSize)//解析包不为空
  {

    //收到Udp数据包
    //Udp.remoteIP().toString().c_str()用于将获取的远端IP地址转化为字符串
  //  Serial.printf("收到来自远程IP：%s（远程端口：%d）的数据包字节数：%d\n", Udp.remoteIP().toString().c_str(), Udp.remotePort(), packetSize);
    // 读取Udp数据包并存放在incomingPacket
    int len = Udp.read(incomingPacket, 4);//返回数据包字节数
    if (len > 0)
    {
      incomingPacket[len] = 0;//清空缓存
   //   Serial.printf("UDP数据包内容为: %s\n", incomingPacket);//向串口打印信息
      if (strcmp(incomingPacket, "UPUP") == 0) // 收到slavepad向上的信息
      {
        slaverpadup = 1;
      }
      else if (strcmp(incomingPacket, "DOWN") == 0) //收到slavepad向下的信息
      {
        slaverpaddown = 1;
      }
      else // 如果指令错误，调用sendCallBack
      {
        delay(1);
        Serial.printf("error");//向串口打印信息
      }

    }
  }
}

////连接字符串////
void line() {

  String string1, string2, string3, string4, string5, string6, string7, string8;

  // oneLine = sstate + ';' + sclearall + ';' + sball_x + ';' + sball_y + ';' + scpu_y + ';' + splayer_y + ';' + sscoreCPU + ';' + sscoreUSER + ';' + '.'  ;
  string1 = sstate;
  string2 = sclearall;
  string3 = sball_x;
  string4 = sball_y;
  string5 = scpu_y;
  string6 = splayer_y;
  string7 = sscoreCPU;
  string8 = sscoreUSER;

  oneLine = string1 + ';' + string2 + ';' + string3 + ';' + string4 + ';' + string5 + ';' + string6 + ';' + string7 + ';' + string8 + ';' + '.';

  // Serial.println(oneLine);
}

void tochar() {

  String str = oneLine; 
 // Serial.println("字符串为：");
  //Serial.println(oneLine);
int str_len = str.length() + 1; 
char char_array[str_len];
str.toCharArray(char_array, str_len);
 // Serial.println("转换后为：");
  //Serial.println(char_array);
  send(char_array);
}
