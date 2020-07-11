/*代码只实现了功能，并未做优化和封装，可能会有bug！*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define ssid      "LBm"       //这里改成你的设备当前环境下WIFI名字
#define password  "11111111"     //这里改成你的设备当前环境下WIFI密码

#define UP_BUTTON 14//GOIO14=D5
#define DOWN_BUTTON 12//GOIO12=D6

#define SCREEN_WIDTH 128 // OLED宽度像素
#define SCREEN_HEIGHT 64 // OLED高度像素

// 0.96寸SSD1306，I2C协议 (SDA, SCL 标号)
#define OLED_RESET     3 // Reset pin ,虽然用不上但是还是要设置
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint8_t CPU_X = 12;
const uint8_t PLAYER_X = 115;
const uint8_t PADDLE_HEIGHT = 24;
uint8_t oldball_x;
uint8_t oldball_y;

uint8_t oldcpu_y;
uint8_t oldplayer_y;

int oldscoreCPU;
int oldscoreUSER;


////master发来的信息////
String oneLine = " ";//master发来的信息

////拆分后的master发来的信息//
int rstate = 0;//游戏状态。0为
int rclearall = 0;   //清理屏幕标志位
uint8_t rball_x = 64;   //球的x坐标
uint8_t rball_y = 32;   //球的y坐标
uint8_t rcpu_y = 16;//master玩家球拍的y坐标
uint8_t rplayer_y = 16;//本地玩家球拍的y坐标
int rscoreCPU = 0;//master的分数
int rscoreUSER = 0;//本地玩家的分数

WiFiUDP Udp;//实例化WiFiUDP对象
unsigned int localUdpPort = 4321;  // 自定义本地监听端口
unsigned int remoteUdpPort = 1234;  // 自定义远程监听端口
char incomingPacket[30];  // 保存Udp工具发过来的消息

void Split();
void send();
void getmasterdata();
void drawCourt();
void splash();
void printScreen();
void SinglePlayer();


void setup() {
  delay(50);

  ////串口初始化////
  Serial.begin(115200);//打开串口
  Serial.println();


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);//使用I2C地址0x3C初始化（对于128x64）
  display.clearDisplay();
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  digitalWrite(UP_BUTTON, 1);
  digitalWrite(DOWN_BUTTON, 1);
  splash();

  display.clearDisplay();

  ////wifiSTA模式初始化和连接////
  Serial.printf("正在连接 %s ", ssid);
  WiFi.begin(ssid, password);//连接到wifi
  while (WiFi.status() != WL_CONNECTED)//等待连接
  {
    delay(500);
    Serial.print(".");
    display.setTextColor(WHITE);
    centerPrint("Connecting...", 24, 1);
    display.display();
  }
  Serial.println("连接成功");


  ////UDP监听服务初始化////
  if (Udp.begin(localUdpPort)) { //启动Udp监听服务
    Serial.println("监听成功");
    //WiFi.localIP().toString().c_str()用于将获取的本地IP地址转化为字符串
    Serial.printf("现在收听IP：%s, UDP端口：%d\n", WiFi.localIP().toString().c_str(), localUdpPort);
  } else {
    Serial.println("监听失败");
  }

  display.clearDisplay();

  drawCourt();
  printScores();

  //  while (millis() - start < 2000);
  // ball_update = millis();
  //  paddle_update = ball_update;



}

void loop() {

  ////按键触发////
  if (digitalRead(UP_BUTTON) == LOW) {
    send("UPUP");
    //  Serial.println("已发送ONN");
  }
  if (digitalRead(DOWN_BUTTON) == LOW) {
    send("DOWN");
    //   Serial.println("已发送OFF");
  }

  getmasterdata();//收取master发来的信息

  SinglePlayer();

  printScores();

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
  int rscoreCPUWidth = 5 * SIZE;
  if (rscoreCPU > 9) rscoreCPUWidth += 6 * SIZE;
  if (rscoreCPU > 99) rscoreCPUWidth += 6 * SIZE;
  if (rscoreCPU > 999) rscoreCPUWidth += 6 * SIZE;
  if (rscoreCPU > 9999) rscoreCPUWidth += 6 * SIZE;

  if (rscoreCPU - oldscoreCPU != 0 || rscoreUSER - oldscoreUSER != 0) {
    display.fillRect(20, 10, 40, 10, BLACK);
    display.fillRect(68, 10, 40, 10, BLACK);
    display.display();
  }

  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH / 2 - SCORE_PADDING - rscoreCPUWidth, 10);
  display.print(rscoreCPU);
  display.setCursor(SCREEN_WIDTH / 2 + SCORE_PADDING + 2, 10); //+1 because of dotted line.
  display.print(rscoreUSER);

  oldscoreCPU = rscoreCPU;
  oldscoreUSER = rscoreUSER;



  display.display();
}





void SinglePlayer() {

  //  bool update = false;
  //   if (time > ball_update) {

  display.drawPixel(oldball_x, oldball_y, BLACK);
  display.drawPixel(rball_x, rball_y, WHITE);
  oldball_x = rball_x;
  oldball_y = rball_y;

  //    update = true;
  //   }

  //  if (time > paddle_update) {
  //    paddle_update += PADDLE_RATE;

  // CPU paddle
  display.drawFastVLine(CPU_X, oldcpu_y + 1, PADDLE_HEIGHT, BLACK);
  display.drawFastVLine(CPU_X, rcpu_y + 1, PADDLE_HEIGHT, WHITE);
  oldcpu_y = rcpu_y;

  // Player paddle
  display.drawFastVLine(PLAYER_X, oldplayer_y + 1, PADDLE_HEIGHT, BLACK);
  display.drawFastVLine(PLAYER_X, rplayer_y + 1, PADDLE_HEIGHT, WHITE);
  oldplayer_y = rplayer_y;

  //    update = true;
  //   }

  //  if (update)
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
void getmasterdata() {
  int packetSize = Udp.parsePacket();//获得解析包
  if (packetSize)//解析包不为空
  {
   
    //收到Udp数据包
    //Udp.remoteIP().toString().c_str()用于将获取的远端IP地址转化为字符串
    //Serial.printf("收到来自远程IP：%s（远程端口：%d）的数据包字节数：%d\n", Udp.remoteIP().toString().c_str(), Udp.remotePort(), packetSize);
    // 读取Udp数据包并存放在incomingPacket
    int len = Udp.read(incomingPacket, 30);//返回数据包字节数
   
    if (len > 0)
    {
      
      incomingPacket[len] = 0;//清空缓存
      oneLine = incomingPacket;
 
      Split();//拆分master发来的信息
      
    }
  }
}

////拆分master发来的字符串////
void Split() {
  
  int masterdata[8], r = 0, t = 0;
  
  for (int i = 0; i < oneLine.length(); i++)
  {
     /*if (oneLine.charAt(i) == '.')
     {
      break;
      }
      */
    if (oneLine.charAt(i) == ';')
   
    { 
      masterdata[t] = oneLine.substring(r, i).toInt();
      r = (i + 1);
      t++;
    }
    
  }
  rstate = masterdata[0];
  rclearall = masterdata[1];
  rball_x = masterdata[2];
  rball_y = masterdata[3];
  rcpu_y  = masterdata[4];
  rplayer_y = masterdata[5];
  rscoreCPU = masterdata[6];
  rscoreUSER = masterdata[7];
  /*
    Serial.println(rstate);
    Serial.println(rclearall);
    Serial.println(rball_x);
    Serial.println(rball_y);
    Serial.println(rcpu_y);
    Serial.println(rplayer_y);
    Serial.println(rscoreCPU);
    Serial.println(rscoreUSER);
  */
}
