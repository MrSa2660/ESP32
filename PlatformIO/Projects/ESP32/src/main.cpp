#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <time.h>
#include "config.h"

// ── Pin mapping ───────────────────────────────────────────────
//   BTN[0] very-happy   GPIO 13   LED[0] GPIO 25
//   BTN[1] happy        GPIO 27   LED[1] GPIO 26
//   BTN[2] unhappy      GPIO 32   LED[2] GPIO 22
//   BTN[3] very-unhappy GPIO 33   LED[3] GPIO 23
const gpio_num_t BTN[4] = {GPIO_NUM_13, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33};
const int        LED[4] = {25, 26, 22, 23};
const char*    LABEL[4] = {"very_happy", "happy", "unhappy", "very_unhappy"};

// ── Timing ────────────────────────────────────────────────────
const unsigned long DEBOUNCE_MS    = 10;
const unsigned long LOCKOUT_MS     = 4000;
const unsigned long SLEEP_AFTER_MS = 10000;

// ── Button state ──────────────────────────────────────────────
static int           lastReading[4];
static int           btnState[4];
static unsigned long lastDebounce[4];
static int           lockedBtn    = -1;
static unsigned long lockStart    = 0;
static unsigned long lastActivity = 0;

// ── Network state machine ─────────────────────────────────────
enum NetState { WIFI_CONNECTING, NTP_SYNCING, MQTT_CONNECTING, NET_READY };
static NetState      netState      = WIFI_CONNECTING;
static unsigned long netStateStart = 0;
static int           pendingPress  = -1;

static WiFiClientSecure net;
static PubSubClient     mqtt(net);

// ── Helpers ───────────────────────────────────────────────────
static void allLedsOff() {
    for (int i = 0; i < 4; i++) digitalWrite(LED[i], LOW);
}

static String getTimestamp() {
    struct tm ti;
    if (!getLocalTime(&ti)) return "1970-01-01T00:00:00";
    char buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &ti);
    return String(buf);
}

static void publishPress(int idx);

// ── Network ───────────────────────────────────────────────────
static void startWiFi() {
    Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    netState      = WIFI_CONNECTING;
    netStateStart = millis();
}

static void startNTP() {
    Serial.printf("[WiFi] OK  IP=%s  GW=%s  DNS=%s\n",
        WiFi.localIP().toString().c_str(),
        WiFi.gatewayIP().toString().c_str(),
        WiFi.dnsIP().toString().c_str());
    configTime(0, 0, "0.dk.pool.ntp.org", "pool.ntp.org", "time.google.com");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    Serial.println("[NTP]  Syncing...");
    netState      = NTP_SYNCING;
    netStateStart = millis();
}

static void startMQTT() {
    net.setInsecure();
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    Serial.printf("[MQTT] Connecting to %s:%d...\n", MQTT_HOST, MQTT_PORT);
    netState      = MQTT_CONNECTING;
    netStateStart = millis();
}

static void updateNetwork() {
    unsigned long now = millis();
    switch (netState) {

        case WIFI_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                startNTP();
            } else if (now - netStateStart > 15000) {
                Serial.println("[WiFi] Timeout — retrying...");
                WiFi.reconnect();
                netStateStart = now;
            }
            break;

        case NTP_SYNCING: {
            struct tm ti;
            if (getLocalTime(&ti)) {
                Serial.printf("[NTP]  OK  %s\n", getTimestamp().c_str());
                startMQTT();
            } else if (now - netStateStart > 20000) {
                Serial.println("[NTP]  FAILED — continuing without time sync");
                startMQTT();
            }
            break;
        }

        case MQTT_CONNECTING:
            if (mqtt.connected()) {
                Serial.println("[MQTT] OK");
                netState = NET_READY;
                if (pendingPress >= 0) {
                    publishPress(pendingPress);
                    pendingPress = -1;
                }
            } else if (now - netStateStart > 500) {
                mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS);
                netStateStart = now;
                if (!mqtt.connected())
                    Serial.printf("[MQTT] state=%d retrying...\n", mqtt.state());
            }
            break;

        case NET_READY:
            mqtt.loop();
            if (!mqtt.connected() && WiFi.status() == WL_CONNECTED) {
                Serial.println("[MQTT] Lost connection — reconnecting...");
                netState      = MQTT_CONNECTING;
                netStateStart = millis();
            }
            break;
    }
}

// ── Press / lockout ───────────────────────────────────────────
static void publishPress(int idx) {
    char topic[80];
    snprintf(topic, sizeof(topic), "%s/%s", MQTT_TOPIC, LABEL[idx]);

    char payload[160];
    snprintf(payload, sizeof(payload),
        "{\"timestamp\":\"%s\",\"device\":\"%s\"}",
        getTimestamp().c_str(), DEVICE_ID);

    if (netState != NET_READY || !mqtt.connected()) {
        Serial.printf("[MQTT] Not ready — queued: %s\n", topic);
        pendingPress = idx;
        return;
    }
    bool ok = mqtt.publish(topic, payload);
    Serial.printf("[MQTT] Publish %s  %s  %s\n", ok ? "OK" : "FAILED", topic, payload);
}

static void startLockout(int idx) {
    allLedsOff();
    digitalWrite(LED[idx], HIGH);
    lockedBtn    = idx;
    lockStart    = millis();
    lastActivity = millis();
    Serial.printf("[BTN]  Lockout start — %s (LED %d on for %lus)\n",
        LABEL[idx], LED[idx], LOCKOUT_MS / 1000);
}

static void enterDeepSleep() {
    allLedsOff();

    // Wait for all buttons to be released so the device does not instantly wake.
    bool anyHeld = true;
    while (anyHeld) {
        anyHeld = false;
        for (int i = 0; i < 4; i++)
            if (digitalRead((int)BTN[i]) == HIGH) { anyHeld = true; break; }
        if (anyHeld) delay(50);
    }

    mqtt.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // EXT1 uses RTC GPIO settings while the ESP is asleep.
    for (int i = 0; i < 4; i++) {
        rtc_gpio_pullup_dis(BTN[i]);
        rtc_gpio_pulldown_en(BTN[i]);
    }

    uint64_t wakeupMask = (1ULL << (int)BTN[0]) | (1ULL << (int)BTN[1]) |
                          (1ULL << (int)BTN[2]) | (1ULL << (int)BTN[3]);
    esp_sleep_enable_ext1_wakeup(wakeupMask, ESP_EXT1_WAKEUP_ANY_HIGH);

    Serial.flush();
    esp_deep_sleep_start();
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n========== Smiley Panel boot ==========");

    // After deep sleep, RTC pins must be returned to normal GPIO.
    for (int i = 0; i < 4; i++)
        rtc_gpio_deinit(BTN[i]);

    for (int i = 0; i < 4; i++) {
        pinMode((int)BTN[i], INPUT_PULLDOWN);
        pinMode(LED[i], OUTPUT);
        digitalWrite(LED[i], LOW);
        lastReading[i]  = LOW;
        btnState[i]     = LOW;
        lastDebounce[i] = 0;
    }

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        // Use wakeup status register to identify which button fired.
        uint64_t status = esp_sleep_get_ext1_wakeup_status();
        int pressed = -1;
        for (int i = 0; i < 4; i++) {
            if (status & (1ULL << (int)BTN[i])) { pressed = i; break; }
        }
        if (pressed < 0) {
            for (int i = 0; i < 4; i++) {
                if (digitalRead((int)BTN[i]) == HIGH) { pressed = i; break; }
            }
        }
        if (pressed < 0) {
            enterDeepSleep();
        }
        Serial.printf("[WAKE] Button wake — %s\n", LABEL[pressed]);
        digitalWrite(LED[pressed], HIGH);
        pendingPress = pressed;
        startLockout(pressed);
    } else {
        Serial.println("[BOOT] Cold boot");
    }

    lastActivity = millis();
    startWiFi();
    Serial.println("[READY] Listening (network connecting in background)...");
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
    updateNetwork();

    unsigned long now = millis();

    if (lockedBtn >= 0 && now - lockStart >= LOCKOUT_MS) {
        Serial.printf("[BTN]  Lockout expired — %s ready again\n", LABEL[lockedBtn]);
        allLedsOff();
        lockedBtn = -1;
    }

    if (lockedBtn < 0) {
        for (int i = 0; i < 4; i++) {
            int reading = digitalRead((int)BTN[i]);
            if (reading != lastReading[i]) lastDebounce[i] = now;
            if (now - lastDebounce[i] > DEBOUNCE_MS && reading != btnState[i]) {
                btnState[i] = reading;
                if (btnState[i] == HIGH) {
                    Serial.printf("[BTN]  Pressed: %s (GPIO %d)\n", LABEL[i], (int)BTN[i]);
                    publishPress(i);
                    startLockout(i);
                }
            }
            lastReading[i] = reading;
        }
    }

    if (lockedBtn < 0 && now - lastActivity >= SLEEP_AFTER_MS) {
        enterDeepSleep();
    }
}
