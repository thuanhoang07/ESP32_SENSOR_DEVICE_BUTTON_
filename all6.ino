#include "DHT.h"

#define ERA_DEBUG
#define DHTPIN 22 //dht22(chân out)
#define DHTTYPE DHT22 //thư viện này dùng cho nhiều loại dht nên chọn loại dht22
DHT dht(DHTPIN, DHTTYPE);

#define DEFAULT_MQTT_HOST "mqtt1.eoh.io"
#define ERA_AUTH_TOKEN "70344ef5-4228-4a2d-8a69-cd0930ed7867"

#include <Arduino.h>
#include <ERa.hpp>
#include <ERa/ERaTimer.hpp>

const char ssid[] = "EoH";
const char pass[] = "Eoh@2020";  

ERaTimer timer;

#define MQ2 35 //
#define LIGHT 34 //

#define LED 33     //LED
#define BUZZER 32  //

#define buttonPin 18  //MODE
#define BUTTON_PINquat 19  //NN QUAT
#define BUTTON_PINled 21   //NN LED

#define ena 13  //chân cắm vào l298n để đk tốc độ
#define in1 5 // chân 4,5 cắm vào l298n luôn để đk chiều quay
#define in2 4

int mode;

bool currentStateA = true;  // Trạng thái hiện tại: true = A, false = B
bool lastButtonState = HIGH;

bool currentStateBquat = true;  
bool lastButtonStatequat = HIGH;

bool currentStateBled = true;  
bool lastButtonStateled = HIGH;

void setup() {
  /* Setup debug console */
  Serial.begin(115200);
  ERa.begin(ssid, pass);
  /* Setup timer called function every second */
  pinMode(ena, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(MQ2, INPUT);
  pinMode(LIGHT, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  dht.begin();

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(BUTTON_PINquat, INPUT_PULLUP);
  pinMode(BUTTON_PINled, INPUT_PULLUP);

  analogWrite(LED, 0);  //
  analogWrite(ena, 0);  //

  digitalWrite(in1, HIGH);  //
  digitalWrite(in2, LOW);   //

  ERa.virtualWrite(V0, 0);   //mq2
  ERa.virtualWrite(V1, 0);   //cảm biến ánh sáng
  ERa.virtualWrite(V2, 0);   //độ sáng led
  ERa.virtualWrite(V3, 0);   //tốc độ quạt
  ERa.virtualWrite(V4, 0);   //còi
  ERa.virtualWrite(V5, 0);   //nn quạt
  ERa.virtualWrite(V6, 0);   //nhiệt độ
  ERa.virtualWrite(V7, 0);   //nn led
  ERa.virtualWrite(V8, 0);   //trạng thái(0:thủ công, 1:tự động) mode
  ERa.virtualWrite(V9, 0);   //nn mode bật auto
  ERa.virtualWrite(V10, 0);  //trạng thái led
  ERa.virtualWrite(V11, 0);  //trạng thái quạt
}

ERA_WRITE(V7)  //CHỈ WRITE ĐƯỢC TỪ NÚT NHẤN TRÊN ERA XUỐNG ESP!
{
  int p = param.getInt();     //LẤY GIÁ TRỊ CỦA CHÂN ẢO
  if (mode = 0) {             // mode thủ công
    ERa.virtualWrite(V2, p);  // ĐỒNG BỘ TRẠNG THÁI BUTTON VÀ CHÂN ẢO TRÊN ERA
    analogWrite(LED, p);
  }
  if (p > 0) {
    currentStateBled = true;
  } else {
    currentStateBled = false;
  }
}

ERA_WRITE(V5) {
  int p1 = param.getInt();  //LẤY GIÁ TRỊ CỦA CHÂN ẢO
  if (mode = 0) {           // mode thủ công thì mới đồng bộ
    analogWrite(ena, p1);
    ERa.virtualWrite(V3, p1);  // ĐỒNG BỘ TRẠNG THÁI BUTTON VÀ CHÂN ẢO TRÊN ERA
  }
  if (p1 > 0) {
    currentStateBquat = true; //khi p1(mode thủ công) > 0 thì chạy thực hiện các lệnh if dòng 195-199
  } else {
    currentStateBquat = false; //p1<0 thì ko lọt vào, thực hiện lệnh else dòng 200-205
  }
}

ERA_WRITE(V9) {
  int p2 = param.getInt();   //LẤY GIÁ TRỊ CỦA CHÂN ẢO
  ERa.virtualWrite(V8, p2);  // ĐỒNG BỘ TRẠNG THÁI BUTTON VÀ CHÂN ẢO TRÊN ERA
  if (p2 > 0) {
    currentStateA = true; //chạy vào dòng 177
  } else {
    currentStateA = false;  //chạy vào dòng 183
  }
}

void processStateA() {
  int pwmWavelight = map(analogRead(LIGHT), 0, 4095, 0, 255);
  if (pwmWavelight <= 30) { //dưới 30 thì tắt hẳn led luôn
    ERa.virtualWrite(V10, 0);
    ERa.virtualWrite(V2, 0);
    ERa.virtualWrite(V7, 0);
    analogWrite(LED, 0);
  } else {
    ERa.virtualWrite(V10, 255);
    ERa.virtualWrite(V2, pwmWavelight);
    ERa.virtualWrite(V7, 255);
    analogWrite(LED, pwmWavelight);
  }

  //dht22
  if (dht.readTemperature() < 25) { //nhiệt độ <25 là thì tắt quạt
    analogWrite(ena, 0);  // Tắt động cơ
    ERa.virtualWrite(V3, 0);
    ERa.virtualWrite(V5, 0);
    ERa.virtualWrite(V11, 0);

  } else if (dht.readTemperature() >= 25 && dht.readTemperature() <= 35) { //25-35: bật hơi hơi quạt
    analogWrite(ena, 128);  // Tốc độ trung bình
    ERa.virtualWrite(V3, 128);
    ERa.virtualWrite(V5, 255);
    ERa.virtualWrite(V11, 255);

  } else {
    analogWrite(ena, 255);  // bật full >35
    ERa.virtualWrite(V3, 255);
    ERa.virtualWrite(V5, 255);
    ERa.virtualWrite(V11, 255);
  }
  
  delay(200);
}

void loop() {
  ERa.run();
  timer.run();
  //mq2
  ERa.virtualWrite(V0, analogRead(MQ2));
  ERa.virtualWrite(V4, digitalRead(BUZZER));
  if (analogRead(MQ2) > 2500) {
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(BUZZER, LOW);
  }
  //light sensor
  ERa.virtualWrite(V1, analogRead(LIGHT));
  ERa.virtualWrite(V6, dht.readTemperature());

  bool buttonState = !digitalRead(buttonPin);
  bool buttonStatequat = !digitalRead(BUTTON_PINquat);
  bool buttonStateled = !digitalRead(BUTTON_PINled);

  // Kiểm tra nút đã được nhấn và trạng thái trước đó là không nhấn
  if (buttonState == LOW && lastButtonState == HIGH) {
    currentStateA = !currentStateA;
    Serial.println(currentStateA ? "A" : "B");
  }

  // Nếu trạng thái hiện tại là A, thực thi các lệnh trong trạng thái A
  if (currentStateA) {
    mode = 1;
    processStateA();
    ERa.virtualWrite(V8, 1);
    ERa.virtualWrite(V9, 1);
  } else {
    mode = 0;
    ERa.virtualWrite(V8, 0);
    ERa.virtualWrite(V9, 0);
    if (buttonStatequat == LOW && lastButtonStatequat == HIGH) {
      currentStateBquat = !currentStateBquat;
    }
    if (currentStateBquat) {
      ERa.virtualWrite(V11, 255);
      ERa.virtualWrite(V3, 255);
      ERa.virtualWrite(V5, 255);
      analogWrite(ena, 255);
    } else {
      ERa.virtualWrite(V11, 0);
      ERa.virtualWrite(V3, 0);
      ERa.virtualWrite(V5, 0);
      analogWrite(ena, 0);
    }
    lastButtonStatequat = buttonStatequat;

    if (buttonStateled == LOW && lastButtonStateled == HIGH) {
      currentStateBled = !currentStateBled;
    }
    if (currentStateBled) {
      ERa.virtualWrite(V10, 255);
      ERa.virtualWrite(V2, 255);
      ERa.virtualWrite(V7, 255);
      analogWrite(LED, 255);
    } else {
      ERa.virtualWrite(V10, 0);
      ERa.virtualWrite(V2, 0);
      ERa.virtualWrite(V7, 0);
      analogWrite(LED, 0);
    }
    lastButtonStateled = buttonStateled;
    delay(200);
  }

  lastButtonState = buttonState;
  delay(200);
}
