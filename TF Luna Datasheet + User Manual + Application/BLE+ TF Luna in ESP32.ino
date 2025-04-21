#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <BLE2902.h>

#define LED_PIN 2
HardwareSerial tfSerial(2);  // UART2 (RX=16, TX=17)

bool deviceConnected = false;
BLECharacteristic* pNotifyCharacteristic;
BLEAdvertising* pAdvertising;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("✅ Thiết bị đã kết nối BLE!");
    deviceConnected = true;
    digitalWrite(LED_PIN, HIGH);
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("⚠️ Thiết bị đã ngắt kết nối BLE!");
    deviceConnected = false;
    digitalWrite(LED_PIN, LOW);
    delay(500);
    pAdvertising->start();
  }
};

void setLongRangeMode() {
  byte cmd[9] = {0x5A, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  tfSerial.write(cmd, sizeof(cmd));
  Serial.println("📡 TF-Luna chuyển sang chế độ long-range");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init("ESP32_LiDAR");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService("12345678-1234-1234-1234-1234567890ab");

  pNotifyCharacteristic = pService->createCharacteristic(
    "abcdefab-1234-5678-1234-56789abcdef0",
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pNotifyCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("12345678-1234-1234-1234-1234567890ab");
  pAdvertising->start();
  Serial.println("📣 Đang quảng bá BLE...");

  tfSerial.begin(115200, SERIAL_8N1, 16, 17);
  Serial.println("🔍 Đang khởi động đọc dữ liệu TF-Luna...");
  setLongRangeMode();
}

void loop() {
  // Đọc gói dữ liệu hợp lệ từ TF-Luna
  if (tfSerial.available() >= 9) {
    if (tfSerial.read() == 0x59 && tfSerial.read() == 0x59) {
      uint8_t data[9];
      tfSerial.readBytes(data, 9);

      int distance = data[0] + data[1] * 256;
      int strength = data[2] + data[3] * 256;

      Serial.printf("📏 Khoảng cách: %d cm\t💡 Cường độ: %d\n", distance, strength);

      if (deviceConnected) {
        char payload[20];
        sprintf(payload, "%d", distance);  // Có thể thay bằng JSON nếu muốn thêm strength
        pNotifyCharacteristic->setValue(payload);
        pNotifyCharacteristic->notify();
      }
    }
  }
}
