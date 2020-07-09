#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define AP_ssid   "master" //这里改成你的设备当前环境下接入点名字
#define password  "11111111"          //这里改成你的设备当前环境下接入点密码

#define UP_BUTTON 14//GOIO14=D5
#define DOWN_BUTTON 12//GOIO12=D6

IPAddress local_IP(192,168,1,14);//手动设置的开启的网络的ip地址
IPAddress gateway(192,168,4,9);  //手动设置的网关IP地址
IPAddress subnet(255,255,255,0); //手动设置的子网掩码

WiFiUDP Udp;//实例化WiFiUDP对象
unsigned int localUdpPort = 1234;  // 自定义本地监听端口
unsigned int remoteUdpPort = 4321;  // 自定义远程监听端口
char incomingPacket[3];  // 保存Udp工具发过来的消息

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

  ////wifiAP模式初始化////
  WiFi.mode(WIFI_AP);
  //启动AP，并设置账号和密码
  Serial.printf("设置接入点中 ... ");
  //配置接入点的IP，网关IP，子网掩码
  WiFi.softAPConfig(local_IP, gateway, subnet);
  //启动校验式网络（需要输入账号密码的网络）
  WiFi.softAP(AP_ssid, password);
   ////UDP监听服务初始化////
  if (Udp.begin(localUdpPort)) { //启动Udp监听服务
    Serial.println("监听成功");
    //打印本地的ip地址，在UDP工具中会使用到
    //WiFi.localIP().toString().c_str()用于将获取的本地IP地址转化为字符串
    Serial.printf("现在监听本地IP：%s, 本地UDP端口：%d\n", WiFi.softAPIP().toString().c_str(), localUdpPort);
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
