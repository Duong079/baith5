#include <Arduino.h>
#include <DHT.h>
#include <DHT11_inferencing.h>  // Đổi tên nếu thư viện bạn đặt khác

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Buffer chứa 1 mẫu đầu vào (nhiệt độ + độ ẩm)
float features[EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME];

// Hàm callback để Edge Impulse lấy dữ liệu
int get_data(size_t offset, size_t length, float *out_ptr) {
  for (size_t i = 0; i < length; i++) {
    out_ptr[i] = features[offset + i];
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  delay(2000); // Đợi cảm biến ổn định
  Serial.printf("Model expects %d input values (features)\n", EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
  Serial.println("ESP32 started with Raw Data model...");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(1000);
    return;
  }

  // Gán dữ liệu vào features (theo đúng thứ tự model đã train)
  features[0] = t;  // Temperature
  features[1] = h;  // Humidity

  // Chuẩn bị tín hiệu để suy luận
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
  signal.get_data = get_data;

  ei_impulse_result_t result = {0};

  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
  if (res != EI_IMPULSE_OK) {
    Serial.printf("Lỗi khi chạy mô hình: %d\n", res);
    delay(1000);
    return;
  }

  // In kết quả dự đoán
  Serial.println("== Kết quả dự đoán ==");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    Serial.printf("%s: %.2f\n", result.classification[ix].label, result.classification[ix].value);
  }
  Serial.println("====================\n");

  delay(2000); // Đợi trước lần suy luận tiếp theo
}
