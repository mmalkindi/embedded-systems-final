#include <esp_now.h>
#include <WiFi.h>

// ================= MAC RECEIVER =================
uint8_t receiverMac[] = {0x88, 0x57, 0x21, 0x94, 0x69, 0x10};

// ================= STRUKTUR DATA =================
typedef struct {
  long distanceCM;
  int percentage;   // persentase terhadap BATAS AMAN
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// ================= PIN =================
#define TRIG_PIN    14
#define ECHO_PIN    27
#define LED_PIN     25
#define BUZZER_PIN  26

// ================= PARAMETER TABUNG =================
#define TANK_HEIGHT_CM 12   // tinggi fisik tabung
#define SAFE_HEIGHT_CM 8    // batas aman pengisian

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
  // kosong (FreeRTOS yang jalan)
}

// ================= TASK BACA ULTRASONIC =================
void readUltrasonicTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    // Trigger ultrasonic
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

    // ================= HITUNG KETINGGIAN AIR =================
    if (distanceCM >= 0 && distanceCM <= TANK_HEIGHT_CM) {
      long waterHeight = TANK_HEIGHT_CM - distanceCM;

      // Persentase terhadap BATAS AMAN
      waterPercent = (waterHeight * 100) / SAFE_HEIGHT_CM;
    } else {
      waterPercent = 0;
    }

    // ================= LOGIKA LED & BUZZER =================
    // LED ON jika air < 20% batas aman
    digitalWrite(LED_PIN, waterPercent < 20 ? HIGH : LOW);

    // BUZZER ON jika melewati batas aman
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

    Serial.print("Distance: ");
    Serial.print(distanceCM);
    Serial.print(" cm | Level: ");
    Serial.print(waterPercent);
    Serial.println(" %");

    vTaskDelay(pdMS_TO_TICKS(300));
  }
}
