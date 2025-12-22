#include <esp_now.h>
#include <WiFi.h>

// ================= ESP-NOW =================
uint8_t broadcastAddress[] = {0x88, 0x57, 0x21, 0x94, 0x69, 0x10};

typedef struct struct_message {
  long distanceCM;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// ================= HC-SR04 =================
#define TRIG_PIN 14
#define ECHO_PIN 27

volatile long distanceCM = 0;

// ================= CALLBACK =================
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ================= TASK =================
void readUltrasonicTask(void *pvParameters);
void sendEspNowTask(void *pvParameters);

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Add peer failed");
    return;
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  xTaskCreatePinnedToCore(readUltrasonicTask, "Read Ultrasonic", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(sendEspNowTask, "Send ESPNow", 2048, NULL, 1, NULL, 1);
}

void loop() {}

// ================= TASK BACA SENSOR =================
void readUltrasonicTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30 ms timeout

    if (duration == 0) {
      distanceCM = -1;
    } else {
      distanceCM = duration / 58; // rumus HC-SR04
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= TASK KIRIM ESP-NOW =================
void sendEspNowTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    myData.distanceCM = distanceCM;

    esp_err_t result = esp_now_send(
      broadcastAddress,
      (uint8_t *)&myData,
      sizeof(myData)
    );

    Serial.print("Distance: ");
    Serial.println(distanceCM);

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
