#include <esp_now.h>
#include <WiFi.h>

// ================= STRUCT DATA =================
typedef struct struct_message {
  long distanceCM;
} struct_message;

struct_message receivedData;
volatile long receivedDistance = -1;

// ================= PROTOTYPE TASK =================
void displayTask(void *pvParameters);

// ================= CALLBACK ESP-NOW =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len == sizeof(struct_message)) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    receivedDistance = receivedData.distanceCM;
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

  xTaskCreatePinnedToCore(
    displayTask,
    "Display",
    2048,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
  // kosong
}

// ================= TASK DISPLAY =================
void displayTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    Serial.print("Distance received: ");
    Serial.print(receivedDistance);
    Serial.println(" cm");

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
