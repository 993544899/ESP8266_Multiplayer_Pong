/**********************************************************************
  项目名称/Project          : ESP8266局域网复古乒乓球对战
  程序名称/Program name     : master_Ponggame主机
  团队/Team                 : 太极创客团队 / Taichi-Maker (www.taichi-maker.com)
  作者/Author               : Blackbox114
  日期/Date（YYYYMMDD）     : 20200715
  程序目的/Purpose          :
  和slave从机联网进行复古乒乓球游戏。有简单的联网对战和计分功能。代码存在不足，随着
  游戏进行，误差会积累，可能会导致主机和从机游戏状态出现差异。后续版本持续优化中...
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

////引入的库文件的头文件////////
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>
//////////////////////////////

////作为AP_master需要的参数////
#define AP_ssid   "master" //这里改成你的设备当前环境下接入点名字
#define password  "11111111"          //这里改成你的设备当前环境下接入点密码
IPAddress local_IP(192, 168, 1, 14); //手动设置的开启的网络的ip地址
IPAddress gateway(192, 168, 4, 9); //手动设置的网关IP地址
IPAddress subnet(255, 255, 255, 0); //手动设置的子网掩码
//////////////////////////////

///////UDP传输相关参数////////
WiFiUDP Udp;//实例化WiFiUDP对象
unsigned int localUdpPort = 1234;  // 自定义本地监听端口
unsigned int remoteUdpPort = 4321;  // 自定义远程监听端口
int slavepad = 16;  //slave机的球拍位置
int masterpad = 16; //master机的球拍位置
Ticker changedata;//实例化一个定时器对象
//////////////////////////////

//////游戏运行的各项参数///////
uint8_t ball_x = 64, ball_y = 32;
uint8_t ball_x0 = 64, ball_y0 = 32;
uint8_t ball_dir_x = 1, ball_dir_y = 1;
unsigned long ball_update;
unsigned long paddle_update;
const uint8_t CPU_X = 12;
uint8_t cpu_y = 16;
const uint8_t PLAYER_X = 115;
uint8_t player_y = 16;
const unsigned long PADDLE_RATE = 33;
const unsigned long BALL_RATE = 16;
const uint8_t PADDLE_HEIGHT = 24;
int scoreCPU = 0;
int scoreUSER = 0;
int startflag = 0;
int keyflag = 0;
//int end_x = 0;
uint8_t new_x = 1;
uint8_t new_y = 1;
//////////////////////////////

/////////按键引脚定义/////////
#define UP_BUTTON 14//GOIO14=D5
#define DOWN_BUTTON 12//GOIO12=D6
//////////////////////////////

/////////oled显示定义/////////
#define SCREEN_WIDTH 128 // OLED宽度像素
#define SCREEN_HEIGHT 64 // OLED高度像素
// 0.96寸SSD1306，I2C协议 (SDA, SCL 标号)
#define OLED_RESET     3 // Reset pin ,虽然用不上但是还是要设置
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//////////////////////////////

///////初始化使用的函数////////
void drawCourt();//画出球场
void splash();//初始界面
void printScreen();//打印双方分数
void Multiplayer();//游戏主函数。多人联机是由单人游戏扩展而来。单人游戏文件可在github测试用文件中找到
void UDPsetup();//初始化UDP链接
void UDPgetstart();//同步来自对方的开始游戏信息
void UDPgetdata();//接受和发送游戏进行时传输的数据
void UDPsend();//UDP发送信息函数
void gamestart();//确定游戏开始的函数
void tochar();//进行数值和字符串转换并发送的函数
void change();//交换数据的函数。此函数包含tochar()和UDPgetdata()
void wifiAPconnect();//作为master机的AP热点开启初始化
//void gamerestart();
//////////////////////////////

void setup() {
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
  UDPsetup();//UDP链接初始化
  changedata.attach(0.02, change);//开启定时器，每0.02秒就同步一次数据
  splash();//开机初始化界面
  wifiAPconnect();//作为master的接入点初始化
  gamestart();//确认开始游戏界面
  drawCourt();//画球场的框线
  printScores();//屏幕显示分数

  while (millis() - start < 2000);
  ball_update = millis();
  paddle_update = ball_update;
}

void loop() {

  Multiplayer();
}

//
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

  //打印分数
  int SIZE = 2;
  int SCORE_PADDING = 10;
  //以分数位置为基准，修改整体宽度
  int scoreCPUWidth = 5 * SIZE;
  if (scoreCPU > 9) scoreCPUWidth += 6 * SIZE;
  if (scoreCPU > 99) scoreCPUWidth += 6 * SIZE;
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH / 2 - SCORE_PADDING - scoreCPUWidth, 10);
  display.print(scoreCPU);
  display.setCursor(SCREEN_WIDTH / 2 + SCORE_PADDING + 2, 10); //+1 because of dotted line.
  display.print(scoreUSER);
  display.display();
}

void Multiplayer() {
  bool update = false;
  unsigned long time = millis();

  static bool up_state = false;
  static bool down_state = false;

  up_state |= (digitalRead(UP_BUTTON) == LOW);
  down_state |= (digitalRead(DOWN_BUTTON) == LOW);

  if (time > ball_update) {
    //  uint8_t new_x = ball_x + ball_dir_x;
    //  uint8_t new_y = ball_y + ball_dir_y;

    new_x = ball_x + ball_dir_x;
    new_y = ball_y + ball_dir_y;
    // 如果球撞到slave玩家的墙
    if (new_x == 0) {
      //   UDPsend("0");
      display.clearDisplay();
      drawCourt();
      scoreUSER++;
      printScores();
      delay(1000);
      new_x = ball_x0 + ball_dir_x;
      new_y = ball_y0 + ball_dir_y;

    }

    // 如果球撞到本机玩家的墙
    if (new_x == 127) {
      //   UDPsend("0");
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

    //如果球撞到slave玩家的拍子上
    if (new_x == CPU_X && new_y >= cpu_y && new_y <= cpu_y + PADDLE_HEIGHT) {
      ball_dir_x = -ball_dir_x;
      new_x += ball_dir_x + ball_dir_x;
    }

    // 如果球撞到本机玩家的拍子上
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
    ///  end_x = new_x;
    ball_update += BALL_RATE;

    update = true;
  }
  if (time > paddle_update) {
    paddle_update += PADDLE_RATE;

    // slave玩家的球拍
    display.drawFastVLine(CPU_X, cpu_y, PADDLE_HEIGHT, BLACK);
    const uint8_t half_paddle = PADDLE_HEIGHT >> 1;
    cpu_y = slavepad;
    if (cpu_y < 1) cpu_y = 1;
    if (cpu_y + PADDLE_HEIGHT > 63) cpu_y = 63 - PADDLE_HEIGHT;
    display.drawFastVLine(CPU_X, cpu_y, PADDLE_HEIGHT, WHITE);

    // 本机玩家的球拍
    display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, BLACK);
    if (up_state) {
      player_y -= 1;
    }
    if (down_state) {
      player_y += 1;
    }
    up_state = down_state = false;
    if (player_y < 1) player_y = 1;
    masterpad = player_y;
    if (player_y + PADDLE_HEIGHT > 63) player_y = 63 - PADDLE_HEIGHT;
    display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE);

    update = true;
  }

  if (update)
    display.display();

}

////作为master机AP点的wifi初始化
void wifiAPconnect() {
  ////wifiAP模式初始化////
  WiFi.mode(WIFI_AP);
  //启动AP，并设置账号和密码
  Serial.println("设置接入点中 ... ");
  //配置接入点的IP，网关IP，子网掩码
  WiFi.softAPConfig(local_IP, gateway, subnet);
  //启动校验式网络（需要输入账号密码的网络）
  WiFi.softAP(AP_ssid, password);
  Serial.print("等待连接");
  while (WiFi.softAPgetStationNum() != 1) //等待连接
  {
    delay(500);
    Serial.print(".");
    display.setTextColor(WHITE);
    centerPrint("Waiting for ", 24, 1);
    centerPrint("connection...", 33, 1);
    display.display();
  }
  //  delay(500);
  display.clearDisplay();
  Serial.println("wifi连接成功");
}

////UDP监听服务初始化////
void UDPsetup() {
  if (Udp.begin(localUdpPort)) { //启动Udp监听服务
    Serial.println("监听成功");
    //打印本地的ip地址，在UDP工具中会使用到
    //WiFi.localIP().toString().c_str()用于将获取的本地IP地址转化为字符串
    Serial.printf("现在监听本地IP：%s, 本地UDP端口：%d\n", WiFi.localIP().toString().c_str(), localUdpPort);
  } else {
    Serial.println("监听失败");
  }
}

////udp发送消息////
void UDPsend(char *buffer)
{
  Udp.beginPacket(Udp.remoteIP(), remoteUdpPort);//配置远端ip地址和端口
  Udp.write(buffer); //把数据写入发送缓冲区
  Udp.endPacket(); //发送数据
  //  Serial.printf("本机IP：%s", WiFi.localIP().toString().c_str());
  //Serial.printf("发送给远程IP：%s", Udp.remoteIP().toString().c_str());
}



////解析游戏开始的Udp数据包////
void UDPgetstart() {
  char startmessage[5];  // 游戏开始的信息
  int packetSize = Udp.parsePacket();//获得解析包
  if (packetSize)//解析包不为空
  {
    //收到Udp数据包
    //Udp.remoteIP().toString().c_str()用于将获取的远端IP地址转化为字符串
    //  Serial.printf("收到来自远程IP：%s（远程端口：%d）的数据包字节数：%d\n", Udp.remoteIP().toString().c_str(), Udp.remotePort(), packetSize);
    // 读取Udp数据包并存放在startmessage
    int len = Udp.read(startmessage, 5);//返回数据包字节数
    if (len > 0)
    {
      startmessage[len] = 0;//清空缓存
      //   Serial.printf("UDP数据包内容为: %s\n", startmessage);//向串口打印信息

      //strcmp函数是string compare(字符串比较)的缩写，用于比较两个字符串并根据比较结果返回整数。
      //基本形式为strcmp(str1,str2)，若str1=str2，则返回零；若str1<str2，则返回负数；若str1>str2，则返回正数。

      if (strcmp(startmessage, "start") == 0) // 收到开始游戏的信息
      {
        startflag = 1; // 开始游戏标志位置1
      }
      else
      {
        Serial.printf("start error");//向串口打印错误信息
      }
    }
  }
}

////解析游戏运行的Udp数据包////
void UDPgetdata() {
  char padmessage[3];  // slave的球拍位置
  String message;
  int packetSize = Udp.parsePacket();//获得解析包
  if (packetSize)//解析包不为空
  {
    // 读取Udp数据包并存放在startmessage
    int len = Udp.read(padmessage, 5);//返回数据包字节数
    if (len > 0)
    {
      padmessage[len] = 0;//清空缓存
      message = padmessage;
      slavepad = message.toInt();
      //  if (slavepad == 0) {
      //     gamerestart();
      //  }
    }
  }
}

////开始游戏的触发函数////
void gamestart() {

  while (keyflag + startflag  < 2) //等待master和slave确认游戏开始
  {
    if (digitalRead(UP_BUTTON) == LOW || digitalRead(DOWN_BUTTON) == LOW) {
      UDPsend("start");
      keyflag = 1;
    }

    UDPgetstart();
    display.setTextColor(WHITE);
    centerPrint("PRESS KEY TO START", 24, 1);
    centerPrint("Ready?", 33, 1);
    display.display();

    /*
        Serial.println("keyflag");
        Serial.println(keyflag);
           Serial.println("startflag");
          Serial.println(startflag);
    */
  }
  UDPsend("start");
  UDPsend("start");
  UDPsend("start");
  UDPsend("start");
  UDPsend("start");
  display.clearDisplay();
  Serial.println("游戏开始");
}

void tochar() {
  String str;
  str = masterpad;
  int str_len = str.length() + 1;
  char char_array[str_len];
  str.toCharArray(char_array, str_len);
  UDPsend(char_array);
}

void change() {
  tochar();
  UDPgetdata();
}
/*
  void gamerestart() {
  display.clearDisplay();
  drawCourt();
  if (end_x  < 64) {
    scoreCPU++;
  }
  else {
    scoreUSER++;
  }
  printScores();
  delay(1000);
  new_x = ball_x0 + ball_dir_x;
  new_y = ball_y0 + ball_dir_y;
  }*/
