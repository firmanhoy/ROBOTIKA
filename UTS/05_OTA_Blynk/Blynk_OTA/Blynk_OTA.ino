// ============================================================================
// PRAKTIKUM BLYNK.AIR (OTA) + MOTOR DUAL TIMING INTERVAL (VERSI 0.1.1 - TARGET OTA)
// ============================================================================

#define BLYNK_TEMPLATE_ID "TMPL6R4txqTf8"
#define BLYNK_TEMPLATE_NAME "Blynk OTA"
#define BLYNK_AUTH_TOKEN "d1BjZc6kuTzzlsrNB5nUSYvYwGSH_DOP"

// Naikkan versi firmware menjadi 0.1.1 untuk Blynk.Air
#define BLYNK_FIRMWARE_VERSION "0.1.2"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <Update.h>

// Konfigurasi Wi-Fi
char ssid[] = "ayam goreng";
char pass[] = "12345678";

// ----------------------------------------------------------------------------
// Konfigurasi Pin Motor DC & LED (Kit iMCLab)
// ----------------------------------------------------------------------------
const int ledPin     = 2;
const int motor1Pin1 = 27;
const int motor1Pin2 = 26;
const int enable1Pin = 12;

// Properti PWM LEDC ESP32 Core 2.x
const int freq        = 30000;
const int pwmChannel  = 0;
const int resolution  = 8;
const int dutyRunning = 240; // Versi 0.1.1: Kecepatan dinaikkan ke 240 (~94%)

// ----------------------------------------------------------------------------
// Variabel State & Timing Non-Blocking
// ----------------------------------------------------------------------------
bool ledState      = LOW;
bool motorSystemOn = false;   // Dikontrol via Switch V1
bool motorState    = false;   // Status aktif/mati motor

// Versi 0.1.1: Interval Unik Baru (Pulsing Cepat: 300 ms ON, 400 ms OFF)
const unsigned long MOTOR_ON_TIME  = 300;
const unsigned long MOTOR_OFF_TIME = 400;

unsigned long intervalBlink = 500;  // Default kedip LED lebih responsif
unsigned long lastBlinkTime = 0;
unsigned long lastMotorTime = 0;
unsigned long lastPrintTime = 0;

// ============================================================================
// Kontrol Motor DC
// ============================================================================
void setMotor(bool hidup)
{
  motorState = hidup;
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);

  if (hidup) {
    ledcWrite(pwmChannel, dutyRunning);
  } else {
    ledcWrite(pwmChannel, 0);
  }
}

// ============================================================================
// Blynk Handlers
// ============================================================================

// Slider V0: Mengatur interval kedip LED (100 - 2000 ms)
BLYNK_WRITE(V0)
{
  int nilaiSlider = param.asInt();
  intervalBlink = constrain(nilaiSlider, 100, 2000);
  Serial.print("[WEB DASHBOARD] Interval LED diubah: ");
  Serial.print(intervalBlink);
  Serial.println(" ms");
}

// Switch V1: Tombol Start / Stop Siklus Motor
BLYNK_WRITE(V1)
{
  motorSystemOn = (param.asInt() == 1);
  if (!motorSystemOn) {
    setMotor(false);
  }
  Serial.print("[WEB DASHBOARD] Sistem Motor: ");
  Serial.println(motorSystemOn ? "START (AKTIF)" : "STOP (NONAKTIF)");
}

// Sinkronisasi status Widget saat ESP32 terhubung
BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V0);
  Blynk.syncVirtual(V1);
}

// ============================================================================
// OTA Handler (Blynk.Air)
// ============================================================================
BLYNK_WRITE(InternalPinOTA)
{
  String otaUrl = param.asString();
  Serial.println("\n[OTA] Menerima firmware baru dari Blynk.Air...");
  Serial.println("[OTA] Mengunduh: " + otaUrl);

  setMotor(false);
  digitalWrite(ledPin, LOW);
  Blynk.disconnect();

  HTTPClient http;
  http.begin(otaUrl);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    if (contentLength > 0 && Update.begin(contentLength))
    {
      WiFiClient& client = http.getStream();
      size_t written = Update.writeStream(client);
      if (written == contentLength && Update.end() && Update.isFinished())
      {
        Serial.println("[OTA] Update 100% Berhasil! Reboot ESP32...");
        delay(1000);
        ESP.restart();
      }
    }
  }
  Serial.println("[OTA] Gagal mengunduh firmware, reboot ESP32...");
  delay(1000);
  ESP.restart();
}

// ============================================================================
// Setup & Loop
// ============================================================================
void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(ledPin, OUTPUT);
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  // Inisialisasi PWM LEDC (Core 2.x)
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);

  setMotor(false);
  digitalWrite(ledPin, LOW);

  Serial.println("\nMenghubungkan ke Wi-Fi & Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop()
{
  Blynk.run();

  unsigned long currentMillis = millis();

  // 1. Logika Kedip LED (Berdasarkan Slider V0)
  if (currentMillis - lastBlinkTime >= intervalBlink)
  {
    lastBlinkTime = currentMillis;
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
  }

  // 2. Logika Interval Motor Versi 0.1.1 (300ms ON / 400ms OFF)
  if (motorSystemOn)
  {
    unsigned long durasiAktif = motorState ? MOTOR_ON_TIME : MOTOR_OFF_TIME;
    if (currentMillis - lastMotorTime >= durasiAktif)
    {
      lastMotorTime = currentMillis;
      setMotor(!motorState);
    }
  }

  // 3. Telemetri Serial Monitor setiap 1 detik
  if (currentMillis - lastPrintTime >= 1000)
  {
    lastPrintTime = currentMillis;
    Serial.print("FW: ");
    Serial.print(BLYNK_FIRMWARE_VERSION);
    Serial.print(" (UPDATED) | Motor: ");
    if (!motorSystemOn) {
      Serial.print("STANDBY");
    } else {
      Serial.print(motorState ? "PULSE (300ms)" : "DIAM (400ms)");
    }
    Serial.print(" | LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  }
}