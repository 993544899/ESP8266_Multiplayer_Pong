#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define ssid      "master"       //这里改成你的设备当前环境下WIFI名字
#define password  "11111111"     //这里改成你的设备当前环境下WIFI密码

#define UP_BUTTON 14//GOIO14=D5
#define DOWN_BUTTON 12//GOIO12=D6

WiFiUDP Udp;//实例化WiFiUDP对象
unsigned int localUdpPort = 4321;  // 自定义本地监听端口
unsigned int remoteUdpPort = 1234;  // 自定义远程监听端口
char incomingPacket[3];  // 保存Udp工具发过来的消息
//String gameval;

void setup()
{
  ////按键初始化////
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  digitalWrite(UP_BUTTON, 1);
  digitalWrite(DOWN_BUTTON, 1);

  ////LED初始化////
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // 熄灭LED

  ////串口初始化////
  Serial.begin(115200);//打开串口
  Serial.println();

  ////wifiSTA模式初始化和连接////
  Serial.printf("正在连接 %s ", ssid);
  WiFi.begin(ssid, password);//连接到wifi
  while (WiFi.status() != WL_CONNECTED)//等待连接
  {
    delay(500);
    Serial.print(".");
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
}

void loop()
{
  ////按键触发////
  if (digitalRead(UP_BUTTON) == LOW) {
    send("ONN");
    Serial.println("已发送ONN");
  }

  if (digitalRead(DOWN_BUTTON) == LOW) {
    send("OFF");
    Serial.println("已发送OFF");
  }

  ////解析Udp数据包////
  int packetSize = Udp.parsePacket();//获得解析包
  if (packetSize)//解析包不为空
  {
    //收到Udp数据包
    //Udp.remoteIP().toString().c_str()用于将获取的远端IP地址转化为字符串
    Serial.printf("收到来自远程IP：%s（远程端口：%d）的数据包字节数：%d\n", Udp.remoteIP().toString().c_str(), Udp.remotePort(), packetSize);

   // gameval = Udp.readString(); 
    
    // 读取Udp数据包并存放在incomingPacket
    int len = Udp.read(incomingPacket, 3);//返回数据包字节数
    if (len > 0)
    {
      incomingPacket[len] = 0;//清空缓存
      Serial.printf("UDP数据包内容为: %s\n", incomingPacket);//向串口打印信息

      //strcmp函数是string compare(字符串比较)的缩写，用于比较两个字符串并根据比较结果返回整数。
      //基本形式为strcmp(str1,str2)，若str1=str2，则返回零；若str1<str2，则返回负数；若str1>str2，则返回正数。

      if (strcmp(incomingPacket, "OFF") == 0) // 命令LED_OFF
      {
        digitalWrite(LED_BUILTIN, HIGH); // 熄灭LED

      }
      else if (strcmp(incomingPacket, "ONN") == 0) //如果收到LED_ON
      {
        digitalWrite(LED_BUILTIN, LOW); // 点亮LED
      }
    }
  }
}

//udp发送消息
void send(const char *buffer)
{
  Udp.beginPacket(Udp.remoteIP(), remoteUdpPort);//配置远端ip地址和端口
  Udp.write(buffer); //把数据写入发送缓冲区
  Udp.endPacket(); //发送数据
 Serial.printf("本机IP：%s", WiFi.localIP().toString().c_str());
 Serial.printf("发送给远程IP：%s", Udp.remoteIP().toString().c_str());
}
