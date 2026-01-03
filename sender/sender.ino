#include <esp_now.h>
#include <WiFi.h>

// ================= MAC RECEIVER =================
uint8_t receiverMac[] = {0x88, 0x57, 0x21, 0x94, 0x69, 0x70};

// ================= STRUKTUR DATA =================
typedef struct {
  long distanceCM;
  int percentage;
} water_message;

water_message myData;

// ================= PIN =================
#define TRIG_PIN    14
#define ECHO_PIN    27
#define LED_PIN     25
#define BUZZER_PIN  26

// ================= PARAMETER TABUNG =================
#define TANK_HEIGHT_CM 11
#define SAFE_HEIGHT_CM 8

// ================= VARIABEL =================
volatile long distanceCM = -1;
volatile int waterPercent = 0;

// ================= CALLBACK ESP-NOW =================
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ESP-NOW: Send OK" : "ESP-NOW: Send FAIL");
}

// ================= PROTOTYPE TASK =================
void readUltrasonicTask(void *pvParameters);
void sendEspNowTask(void *pvParameters);

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Sender Starting...");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  xTaskCreatePinnedToCore(readUltrasonicTask, "ReadUltrasonic", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(sendEspNowTask, "SendESPNow", 2048, NULL, 1, NULL, 1);
}

void loop() {
}

// ================= TASK BACA ULTRASONIC =================
void readUltrasonicTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    if (duration == 0) {
      distanceCM = -1;
    } else {
      distanceCM = duration / 58;
    }

    if (distanceCM >= 0 && distanceCM <= TANK_HEIGHT_CM) {
      long waterHeight = TANK_HEIGHT_CM - distanceCM;
      waterPercent = (waterHeight * 100) / SAFE_HEIGHT_CM;
    } else {
      waterPercent = 0;
    }

    if (waterPercent < 0) waterPercent = 0;
    if (waterPercent > 100) waterPercent = 100;

    digitalWrite(LED_PIN, waterPercent < 20 ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, waterPercent >= 100 ? HIGH : LOW);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ================= TASK KIRIM ESP-NOW =================
void sendEspNowTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    myData.distanceCM = distanceCM;
    myData.percentage = waterPercent;

    esp_now_send(receiverMac, (uint8_t *)&myData, sizeof(myData));

    Serial.print("Water Height Level: ");
    Serial.print(abs(11 - distanceCM));
    Serial.print(" cm | Level: ");
    Serial.print(waterPercent);
    Serial.println(" %");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}