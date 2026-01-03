#include <esp_now.h>
#include <WiFi.h>

// ================= STRUKTUR DATA (HARUS SAMA DENGAN SENDER) =================
typedef struct {
  long distanceCM;
  int percentage;
} struct_message;

struct_message receivedData;

// ================= VARIABEL GLOBAL =================
volatile long receivedDistance = -1;
volatile int receivedPercent = 0;

// ================= PROTOTYPE TASK =================
void displayTask(void *pvParameters);

// ================= CALLBACK ESP-NOW =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len == sizeof(struct_message)) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    receivedDistance = receivedData.distanceCM;
    receivedPercent  = receivedData.percentage;
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Receiver Ready");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  // ================= TASK DISPLAY =================
  xTaskCreatePinnedToCore(
    displayTask,
    "DisplayTask",
    2048,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
  // kosong (FreeRTOS yang bekerja)
}

// ================= TASK DISPLAY =================
void displayTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    Serial.print("Water Height Level received: ");
    Serial.print(11 - receivedDistance);
    Serial.print(" cm | Level: ");
    Serial.print(receivedPercent);
    Serial.println(" %");

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
