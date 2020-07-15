# ESP8266_Multiplayer_Pong

<div align="center"><img width="100%" src="imgs/pongtitle.jpg"> </div>

### 总览

基于ESP8266的网络多人乒乓游戏 / Multiplayer Pong Game based on ESP8266 Network是一个基于wifi，使用UDP协议进行数据传输的双人对战游戏。双方准备好后按下按键即可开始游戏。内置的游戏是一个简单的PONG(乒乓)游戏，有判断胜负的逻辑和计分的功能。硬件仅使用0.96寸，支持I2C协议的oled和ESP8266-NodeMCU开发板，以及按键开关组成。



### 材料清单
|         材料          | 数量 |
| :-------------------: | :--: |
| ESP8266-NodeMCU开发板 |  2   |
|     0.96寸OLED屏      |  2   |
|       按键开关        |  4   |
|         跳线          | 若干 |
|        面包板         |  2   |



### 接线图

请按照接线图搭建两套硬件用于联机对战。

<div align="center"><img width="100%" src="imgs/pong_hardware.png"> </div>




### 代码和使用库
代码位于/src文件夹下。请分别烧录进两套硬件中。
使用的第三方库有：

- Adafruit_GFX库
- Adafruit_SSD1306库

请前往[太极创客第三方库下载页面](http://www.taichi-maker.com/homepage/download/#library-download)进行下载。

如果对ESP8266-NodeMCU开发板的使用有疑问，请参看[此处](http://www.taichi-maker.com/homepage/esp8266-nodemcu-iot/iot-c/esp8266-iot-basics/)的说明

如果对添加第三方库库有疑问，请参看[此处](http://www.taichi-maker.com/homepage/reference-index/arduino-library-index/install-arduino-library/)的说明

如果对烧录代码到8266有疑问，请参看[此处](http://www.taichi-maker.com/homepage/esp8266-nodemcu-iot/iot-c/esp8266-iot-basics/)的说明



### 更多信息

这个小制作只是展示了8266网络对战的效果，用到的物联网相关应用知识并不困难。如果您想学习更多物联网开发相关知识，请访问[太极创客官网](http://www.taichi-maker.com/)获取更多教程。

