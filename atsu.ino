/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * @Dependent Library:
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5Unit-Thermal2: https://github.com/m5stack/M5Unit-Thermal2
 */

#include <M5Unified.h>
#include <M5_Thermal2.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cmath>
#include "WiFiSecrets.h"

M5_Thermal2 thermal2;
WebServer server(80);
M5Canvas info_canvas(&M5.Display);

using TempData = M5_Thermal2::temperature_data_t;

static int g_screen_w = 0;
static int g_screen_h = 0;
static int g_top_h    = 0;
static int g_info_h   = 0;
static bool g_flip_h  = true;
static int g_color_mode = 1;
static uint8_t g_frame_levels[32 * 24];
static bool g_has_frame = false;
static String g_ip_str = "-";
static float g_last_avg = 0.0f;
static float g_last_max = 0.0f;
static float g_last_min = 0.0f;
static bool g_screen_blank = false;
static uint8_t g_saved_brightness = 200;
static int g_brightness_index = 3;
static uint8_t g_led_brightness = 255;
static bool g_easter_egg = false;
static bool g_night_mode = false;
static uint8_t g_prev_brightness = 200;
static bool g_night_led_on = true;
static bool g_wifi_connected = false;
static bool g_hotspot_mode = false;
static String g_ap_ip_str = "-";
static bool g_auto_poweroff_inhibit = false;
static uint32_t g_last_activity_ms = 0;
static bool g_is_charging = false;
static uint32_t g_hotspot_last_activity_ms = 0;
static uint32_t g_batt_last_sample_ms = 0;
static int g_batt_samples[8] = {0};
static int g_batt_sum = 0;
static uint8_t g_batt_sample_count = 0;
static uint8_t g_batt_sample_index = 0;
static int g_batt_avg = 0;
static uint32_t g_blank_last_frame_ms = 0;
static uint8_t g_current_refresh_rate = 6;
static bool g_pending_poweroff = false;
static bool g_low_power_mode = false;
static bool g_sound_active = false;
static uint32_t g_sound_next_ms = 0;
static uint8_t g_sound_index = 0;
static uint8_t g_sound_count = 0;
static uint8_t g_sound_volume = 0;
static int g_sound_notes[4] = {0};
static int g_sound_durations[4] = {0};

static const uint32_t k_auto_poweroff_ms = 120000;
static const uint32_t k_poweroff_warning_ms = 10000;
static const char *k_wifi_placeholder = "Thermal Camera v0.7";
static const char *k_hotspot_ssid = "Thermal";
static const uint32_t k_hotspot_timeout_ms = 5 * 60 * 1000;
static const uint32_t k_blank_frame_interval_ms = 250;
static const uint32_t k_dowsing_frame_interval_ms = 125;
static const uint8_t k_sound_volume_normal = 120;
static const uint8_t k_sound_volume_easter = 180;


static void recordActivity() {
    g_last_activity_ms = millis();
    if (g_hotspot_mode) {
        g_hotspot_last_activity_ms = g_last_activity_ms;
    }
}

static void markHotspotActivity(uint32_t now) {
    if (g_hotspot_mode) {
        g_hotspot_last_activity_ms = now;
    }
}

static int getBatteryPercent() {
    int batt = g_batt_avg;
    if (g_batt_sample_count == 0) {
        batt = static_cast<int>(M5.Power.getBatteryLevel());
    }
    if (batt < 0) batt = 0;
    if (batt > 100) batt = 100;
    return batt;
}

static void applyLowPowerMode(bool enable) {
    if (!enable) return;
    g_auto_poweroff_inhibit = false;
    g_night_mode = false;
    g_easter_egg = false;
    g_led_brightness = 0;
    disableWifi();
    g_saved_brightness = 1;
    M5.Display.setBrightness(1);
    M5.Led.setBrightness(0);
    M5.Led.setAllColor(0, 0, 0);
    thermal2.ledOff();
}

static void disableWifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    g_wifi_connected = false;
    g_hotspot_mode = false;
    g_ip_str = "-";
    g_ap_ip_str = "-";
}

static void enableHotspot() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(k_hotspot_ssid);
    g_ap_ip_str = WiFi.softAPIP().toString();
    g_hotspot_mode = true;
    g_wifi_connected = false;
    g_ip_str = "-";
    g_hotspot_last_activity_ms = millis();
}

static void updateBatteryAverage(uint32_t now) {
    if (now - g_batt_last_sample_ms < 1000) return;
    g_batt_last_sample_ms = now;
    int batt = static_cast<int>(M5.Power.getBatteryLevel());
    if (batt < 0) batt = 0;
    if (batt > 100) batt = 100;

    if (g_batt_sample_count < (uint8_t)(sizeof(g_batt_samples) / sizeof(g_batt_samples[0]))) {
        g_batt_samples[g_batt_sample_index++] = batt;
        g_batt_sum += batt;
        g_batt_sample_count++;
    } else {
        if (g_batt_sample_index >= (uint8_t)(sizeof(g_batt_samples) / sizeof(g_batt_samples[0]))) {
            g_batt_sample_index = 0;
        }
        g_batt_sum -= g_batt_samples[g_batt_sample_index];
        g_batt_samples[g_batt_sample_index++] = batt;
        g_batt_sum += batt;
    }

    if (g_batt_sample_count > 0) {
        g_batt_avg = g_batt_sum / g_batt_sample_count;
    }
}

static void startToneSequence(const int *notes, const int *durations, uint8_t count,
                              uint8_t volume) {
    if (!M5.Speaker.isEnabled() || count == 0) return;
    if (count > 4) count = 4;
    g_sound_count = count;
    g_sound_index = 0;
    g_sound_volume = volume;
    for (uint8_t i = 0; i < count; ++i) {
        g_sound_notes[i] = notes[i];
        g_sound_durations[i] = durations[i];
    }
    M5.Speaker.setVolume(g_sound_volume);
    g_sound_active = true;
    g_sound_next_ms = 0;
}

static void updateSound(uint32_t now) {
    if (!g_sound_active) return;
    if (g_sound_next_ms != 0 && now < g_sound_next_ms) return;
    if (g_sound_index >= g_sound_count) {
        g_sound_active = false;
        M5.Speaker.setVolume(k_sound_volume_normal);
        if (g_pending_poweroff) {
            M5.Power.powerOff();
        }
        return;
    }
    int note = g_sound_notes[g_sound_index];
    int dur = g_sound_durations[g_sound_index];
    M5.Speaker.tone(note, dur);
    g_sound_next_ms = now + dur + 20;
    g_sound_index++;
}

static void playScreenBlankBeep() {
    if (!M5.Speaker.isEnabled()) return;
    const int notes[] = {1200};
    const int durations[] = {80};
    startToneSequence(notes, durations, 1, k_sound_volume_normal);
}

static void playPoweroffChime() {
    if (!M5.Speaker.isEnabled()) {
        M5.Power.powerOff();
        return;
    }
    const int notes[] = {988, 784, 659};
    const int durations[] = {100, 100, 140};
    startToneSequence(notes, durations, 3, k_sound_volume_normal);
}

static void playBootChime() {
    if (!M5.Speaker.isEnabled()) return;
    M5.Speaker.setVolume(k_sound_volume_normal);
    M5.Speaker.tone(880, 80);
    delay(100);
    M5.Speaker.tone(1047, 120);
    delay(140);
}

static void showLoadingScreen(const char *message) {
    M5.Display.clear();
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(ORANGE);
    M5.Display.setTextSize(2);
    M5.Display.drawString("ATSU", g_screen_w / 2, g_screen_h / 2 - 16);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawString(message, g_screen_w / 2, g_screen_h / 2 + 10);
    M5.Display.setTextDatum(TL_DATUM);
}

static uint32_t getColorFromLevel(int level) {
    level = (level < 0) ? 0 : (level > 255) ? 255 : level;

    switch (g_color_mode) {
        case 0: {  // Grayscale
            return M5.Display.color888(level, level, level);
        }
        case 1: {  // Iron (black -> red -> yellow -> white)
            uint8_t r = (level < 128) ? (level * 2) : 255;
            uint8_t g = (level < 128) ? 0 : (level - 128) * 2;
            uint8_t b = (level < 192) ? 0 : (level - 192) * 4;
            return M5.Display.color888(r, g, b);
        }
        default: {  // Rainbow (blue -> cyan -> green -> yellow -> red)
            uint8_t r = 0, g = 0, b = 0;
            if (level < 64) {
                r = 0;
                g = level * 4;
                b = 255;
            } else if (level < 128) {
                r = 0;
                g = 255;
                b = 255 - (level - 64) * 4;
            } else if (level < 192) {
                r = (level - 128) * 4;
                g = 255;
                b = 0;
            } else {
                r = 255;
                g = 255 - (level - 192) * 4;
                b = 0;
            }
            return M5.Display.color888(r, g, b);
        }
    }
}

static uint32_t lerpColor(uint32_t a, uint32_t b, float t) {
    uint8_t ar = (a >> 16) & 0xFF;
    uint8_t ag = (a >> 8) & 0xFF;
    uint8_t ab = a & 0xFF;
    uint8_t br = (b >> 16) & 0xFF;
    uint8_t bg = (b >> 8) & 0xFF;
    uint8_t bb = b & 0xFF;
    uint8_t r = static_cast<uint8_t>(ar + (br - ar) * t);
    uint8_t g = static_cast<uint8_t>(ag + (bg - ag) * t);
    uint8_t bch = static_cast<uint8_t>(ab + (bb - ab) * t);
    return (r << 16) | (g << 8) | bch;
}

static uint32_t getThermalColorFromNorm(float t) {
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
    uint8_t r = 0, g = 0, b = 0;
    if (t < 0.25f) {
        float k = t / 0.25f;
        r = 0;
        g = static_cast<uint8_t>(k * 255);
        b = 255;
    } else if (t < 0.5f) {
        float k = (t - 0.25f) / 0.25f;
        r = 0;
        g = 255;
        b = static_cast<uint8_t>(255 - k * 255);
    } else if (t < 0.75f) {
        float k = (t - 0.5f) / 0.25f;
        r = static_cast<uint8_t>(k * 255);
        g = 255;
        b = 0;
    } else {
        float k = (t - 0.75f) / 0.25f;
        r = 255;
        g = static_cast<uint8_t>(255 - k * 255);
        b = 0;
    }
    return (r << 16) | (g << 8) | b;
}

static void playEasterEggChime() {
    if (!M5.Speaker.isEnabled()) return;

    const int notes[] = {659, 784, 988, 1319};
    const int durations[] = {120, 120, 120, 240};
    startToneSequence(notes, durations, 4, k_sound_volume_easter);
}

static void applyThermalIndicator(float avg_temp, float max_temp, float min_temp,
                                  bool update_screen_led) {
    float temp_norm = (avg_temp - 15.0f) / 25.0f;
    if (temp_norm < 0.0f) temp_norm = 0.0f;
    if (temp_norm > 1.0f) temp_norm = 1.0f;
    float contrast = max_temp - min_temp;
    float contrast_norm = contrast / 10.0f;
    if (contrast_norm < 0.0f) contrast_norm = 0.0f;
    if (contrast_norm > 1.0f) contrast_norm = 1.0f;

    uint32_t base = getThermalColorFromNorm(temp_norm);
    uint32_t mixed = lerpColor(0xFFFFFF, base, contrast_norm);

    uint8_t r = (mixed >> 16) & 0xFF;
    uint8_t g = (mixed >> 8) & 0xFF;
    uint8_t b = mixed & 0xFF;
    float brightness = temp_norm * (g_led_brightness / 255.0f);
    if (brightness > 1.0f) brightness = 1.0f;
    r = static_cast<uint8_t>(r * brightness);
    g = static_cast<uint8_t>(g * brightness);
    b = static_cast<uint8_t>(b * brightness);

    thermal2.setLed(r, g, b);
    thermal2.ledOn();

    if (update_screen_led) {
        M5.Led.setBrightness(static_cast<uint8_t>(brightness * 255.0f));
        M5.Led.setAllColor(r, g, b);
    }
}

static float getTextRotationDeg() {
    if (g_low_power_mode) return 0.0f;
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    if (!M5.Imu.getAccel(&ax, &ay, &az)) {
        return 0.0f;
    }

    if (fabsf(ax) > fabsf(ay)) {
        if (ax > 0.5f) return 90.0f;
        if (ax < -0.5f) return -90.0f;
        return 0.0f;
    }

    if (ay < -0.5f) return 180.0f;
    return 0.0f;
}

static void handleRoot() {
    markHotspotActivity(millis());
    const char *html =
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>Thermal2 Stream</title>"
        "<style>body{font-family:sans-serif;background:#111;color:#eee;margin:0;display:flex;justify-content:center;}"
        "#wrap{padding:16px;}canvas{width:320px;height:240px;image-rendering:pixelated;border:1px solid #333;}"
        "</style></head><body><div id='wrap'>"
        "<h3>Thermal2 Stream</h3><canvas id='c' width='32' height='24'></canvas>"
        "<div><button id='cap'>Capture</button></div>"
        "<div id='info'></div></div>"
        "<script>"
        "const c=document.getElementById('c');const ctx=c.getContext('2d');"
        "const info=document.getElementById('info');"
        "function pal(level,mode){level=Math.max(0,Math.min(255,level));"
        "if(mode===0){return [level,level,level];}"
        "if(mode===1){let r=level<128?level*2:255;let g=level<128?0:(level-128)*2;"
        "let b=level<192?0:(level-192)*4;return [r,g,b];}"
        "let r=0,g=0,b=0;"
        "if(level<64){r=0;g=level*4;b=255;}"
        "else if(level<128){r=0;g=255;b=255-(level-64)*4;}"
        "else if(level<192){r=(level-128)*4;g=255;b=0;}"
        "else{r=255;g=255-(level-192)*4;b=0;}"
        "return [r,g,b];}"
        "document.getElementById('cap').onclick=()=>{const url=c.toDataURL('image/png');window.open(url,'_blank');};"
        "async function tick(){try{const res=await fetch('/frame');if(!res.ok) throw new Error();"
        "const f=await res.json();const img=ctx.createImageData(32,24);"
        "for(let i=0;i<768;i++){const [r,g,b]=pal(f.levels[i],f.mode);"
        "img.data[i*4]=r;img.data[i*4+1]=g;img.data[i*4+2]=b;img.data[i*4+3]=255;}"
        "ctx.putImageData(img,0,0);"
        "info.textContent=`avg:${f.avg.toFixed(1)} max:${f.max.toFixed(1)} min:${f.min.toFixed(1)} mode:${f.mode} flip:${f.flip}`;"
        "}catch(e){}setTimeout(tick,150);}tick();"
        "</script></body></html>";
    server.send(200, "text/html", html);
}

static void handleFrame() {
    markHotspotActivity(millis());
    if (!g_has_frame) {
        server.send(503, "text/plain", "no frame");
        return;
    }

    String json;
    json.reserve(4096);
    json += "{\"w\":32,\"h\":24,\"mode\":";
    json += g_color_mode;
    json += ",\"flip\":";
    json += g_flip_h ? 1 : 0;
    json += ",\"avg\":";
    json += String(g_last_avg, 1);
    json += ",\"max\":";
    json += String(g_last_max, 1);
    json += ",\"min\":";
    json += String(g_last_min, 1);
    json += ",\"levels\":[";

    for (int i = 0; i < 32 * 24; ++i) {
        json += g_frame_levels[i];
        if (i != (32 * 24 - 1)) json += ',';
    }
    json += "]}";

    server.send(200, "application/json", json);
}

static void drawFrame(const TempData &temp_data, bool render) {
    float lowest_temp  = temp_data.getLowestTemperature();
    float highest_temp = temp_data.getHighestTemperature();
    float temp_diff    = highest_temp - lowest_temp;
    if (temp_diff < 0.01f) temp_diff = 0.01f;

    const int scale_w = g_screen_w / 32;
    const int scale_h = g_top_h / 24;
    int scale         = (scale_w < scale_h) ? scale_w : scale_h;
    if (scale < 1) scale = 1;

    const int draw_w  = 32 * scale;
    const int draw_h  = 24 * scale;
    const int origin_x = (g_screen_w - draw_w) / 2;
    const int origin_y = (g_top_h - draw_h) / 2;

    for (int idx = 0; idx < 384; ++idx) {
        int y = idx >> 4;
        int x = ((idx & 15) << 1) + ((y & 1) != temp_data.getSubPage());

        float t   = temp_data.getPixelTemperature(idx) - lowest_temp;
        int level = static_cast<int>(t * 255.0f / temp_diff);
        level     = (level < 0) ? 0 : (level > 255) ? 255 : level;

        if (g_flip_h) {
            x = 31 - x;
        }

        g_frame_levels[y * 32 + x] = static_cast<uint8_t>(level);
        if (render) {
            uint32_t color = getColorFromLevel(level);
            M5.Display.fillRect(origin_x + x * scale, origin_y + y * scale,
                                scale, scale, color);
        }
    }

    if (render) {
        M5.Display.drawRect(origin_x + temp_data.getLowestX() * scale,
                            origin_y + temp_data.getLowestY() * scale, scale,
                            scale, TFT_BLUE);
        M5.Display.drawRect(origin_x + temp_data.getHighestX() * scale,
                            origin_y + temp_data.getHighestY() * scale, scale,
                            scale, TFT_YELLOW);
    }
}

void setup(void) {
    M5.begin();
    if (M5.Display.width() > M5.Display.height()) {
        M5.Display.setRotation(M5.Display.getRotation() ^ 1);
    }

    g_screen_w = M5.Display.width();
    g_screen_h = M5.Display.height();
    g_top_h    = g_screen_h / 2;
    g_info_h   = g_screen_h - g_top_h;

    showLoadingScreen("Loading...");
    playBootChime();

    // Initialize external I2C for Unit devices (Grove port)
    M5.Ex_I2C.begin();

    // Initialize Thermal2 (defaults to address 0x32)
    thermal2.begin();
    thermal2.setRefreshRate(6);  // 32Hz
    thermal2.setNoiseFilterLevel(8);
    g_current_refresh_rate = 6;

    g_saved_brightness = M5.Display.getBrightness();
    if (g_saved_brightness == 0) g_saved_brightness = 200;

    info_canvas.setColorDepth(16);
    info_canvas.createSprite(g_screen_w, g_info_h);
    info_canvas.setTextWrap(false, false);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t wifi_start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - wifi_start) < 15000) {
        delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
        g_wifi_connected = true;
        g_ip_str = WiFi.localIP().toString();
    } else {
        disableWifi();
    }

    server.on("/", handleRoot);
    server.on("/frame", handleFrame);
    server.begin();

    M5.Display.clear();
    g_last_activity_ms = millis();
    g_hotspot_last_activity_ms = g_last_activity_ms;
}

void loop(void) {
    M5.update();
    uint32_t now = millis();
    bool had_activity = false;
    auto charge_state = M5.Power.isCharging();
    g_is_charging = (charge_state == m5::Power_Class::is_charging_t::is_charging);

    updateBatteryAverage(now);
    updateSound(now);

    int batt_percent = getBatteryPercent();
    bool low_power = (batt_percent <= 20);
    if (low_power && !g_low_power_mode) {
        g_low_power_mode = true;
        applyLowPowerMode(true);
    } else if (!low_power && g_low_power_mode) {
        g_low_power_mode = false;
    }

    if (M5.BtnPWR.wasDoubleClicked()) {
        if (!g_low_power_mode) {
            g_auto_poweroff_inhibit = !g_auto_poweroff_inhibit;
        }
        had_activity = true;
    }

    if (M5.BtnB.wasDoubleClicked()) {
        if (!g_low_power_mode) {
            g_night_mode = !g_night_mode;
        }
        had_activity = true;
        if (g_night_mode) {
            g_prev_brightness = g_saved_brightness;
            g_saved_brightness = 1;
            if (!g_screen_blank) {
                M5.Display.setBrightness(g_saved_brightness);
            }
            g_led_brightness = 255;
            g_night_led_on = true;
            g_easter_egg = false;
            if (g_color_mode == 2) g_color_mode = 0;
        } else {
            g_saved_brightness = g_prev_brightness;
            if (!g_screen_blank) {
                M5.Display.setBrightness(g_saved_brightness);
            }
        }
    } else if (M5.BtnB.wasHold()) {
        if (g_low_power_mode) {
            had_activity = true;
        } else {
        if (g_wifi_connected) {
            disableWifi();
        } else if (!g_hotspot_mode) {
            enableHotspot();
        } else {
            disableWifi();
        }
        had_activity = true;
        }
    } else if (M5.BtnB.wasSingleClicked()) {
        had_activity = true;
        if (g_low_power_mode) {
            // Brightness control disabled in low power mode.
        } else if (g_night_mode) {
            g_night_led_on = !g_night_led_on;
        } else {
            static const uint8_t k_levels[] = {40, 80, 120, 160, 200, 255};
            g_brightness_index = (g_brightness_index + 1) % (int)(sizeof(k_levels) / sizeof(k_levels[0]));
            if (g_screen_blank || g_easter_egg) {
                g_led_brightness = k_levels[g_brightness_index];
                if (g_easter_egg) {
                    g_saved_brightness = k_levels[g_brightness_index];
                    M5.Display.setBrightness(g_saved_brightness);
                }
            } else {
                g_saved_brightness = k_levels[g_brightness_index];
                M5.Display.setBrightness(g_saved_brightness);
            }
        }
    }
    if (M5.BtnA.wasPressed()) {
        had_activity = true;
        if (g_night_mode) {
            g_color_mode = (g_color_mode == 0) ? 1 : 0;
        } else {
            g_color_mode = (g_color_mode + 1) % 3;
        }
    }
    if (g_color_mode == 2 && M5.BtnA.wasHold()) {
        if (!g_low_power_mode) {
            g_easter_egg = !g_easter_egg;
        }
        had_activity = true;
        if (!g_easter_egg) {
            M5.Led.setBrightness(0);
            M5.Led.setAllColor(0, 0, 0);
            thermal2.ledOff();
        } else if (!g_screen_blank) {
            M5.Display.setBrightness(g_saved_brightness);
            playEasterEggChime();
        }
    }
    if (M5.BtnPWR.wasPressed()) {
        g_screen_blank = !g_screen_blank;
        had_activity = true;
        if (g_screen_blank) {
            M5.Display.setBrightness(0);
            M5.Display.clear();
            playScreenBlankBeep();
        } else {
            M5.Display.setBrightness(g_saved_brightness);
            M5.Led.setBrightness(0);
            M5.Led.setAllColor(0, 0, 0);
        }
    }

    bool dowsing_mode = g_screen_blank && !g_night_mode && !g_easter_egg && !g_low_power_mode;
    uint8_t desired_rate = 6;
    if (g_easter_egg) {
        desired_rate = 6;
    } else if (g_screen_blank) {
        desired_rate = dowsing_mode ? 2 : 1;
    }
    if (desired_rate != g_current_refresh_rate) {
        thermal2.setRefreshRate(desired_rate);
        g_current_refresh_rate = desired_rate;
    }

    bool allow_frame = true;
    uint32_t blank_interval = dowsing_mode ? k_dowsing_frame_interval_ms : k_blank_frame_interval_ms;
    if (g_screen_blank && (now - g_blank_last_frame_ms) < blank_interval) {
        allow_frame = false;
    }

    if (allow_frame && thermal2.update()) {
        g_blank_last_frame_ms = now;
        auto temp_data = thermal2.getTemperatureData();
        g_has_frame = true;
        g_last_avg = temp_data.getAverageTemperature();
        g_last_max = temp_data.getHighestTemperature();
        g_last_min = temp_data.getLowestTemperature();

        if (thermal2.btnWasClicked()) {
            g_flip_h = !g_flip_h;
            had_activity = true;
        }

        if (g_low_power_mode) {
            thermal2.ledOff();
            M5.Led.setBrightness(0);
            M5.Led.setAllColor(0, 0, 0);
        } else if (g_night_mode) {
            if (g_night_led_on) {
                uint8_t r = 255, g = 255, b = 255;
                if (g_color_mode == 1) {
                    r = 255;
                    g = 120;
                    b = 0;
                }
                thermal2.setLed(r, g, b);
                thermal2.ledOn();
                M5.Led.setBrightness(255);
                M5.Led.setAllColor(r, g, b);
            } else {
                thermal2.ledOff();
                M5.Led.setBrightness(0);
                M5.Led.setAllColor(0, 0, 0);
            }
        } else if (g_screen_blank || g_easter_egg) {
            applyThermalIndicator(g_last_avg, g_last_max, g_last_min,
                                  g_screen_blank);
        } else {
            thermal2.ledOff();
            M5.Led.setBrightness(0);
            M5.Led.setAllColor(0, 0, 0);
        }

        if (!g_screen_blank) {
            M5.Display.startWrite();
            drawFrame(temp_data, true);

            info_canvas.fillSprite(BLACK);

            if (g_night_mode) {
                info_canvas.setTextSize(1);
                info_canvas.setTextColor(TFT_PURPLE);
                info_canvas.setTextDatum(TC_DATUM);
                info_canvas.drawString("NIGHT MODE", g_screen_w / 2, 0);
                info_canvas.setTextDatum(TL_DATUM);
            } else if (g_easter_egg) {
                info_canvas.setTextSize(1);
                info_canvas.setTextColor(WHITE);
                info_canvas.setTextDatum(TC_DATUM);
                info_canvas.drawString("github.com/fitoori", g_screen_w / 2, 0);
                info_canvas.setTextDatum(TL_DATUM);
            } else if (g_auto_poweroff_inhibit && !g_low_power_mode) {
                info_canvas.setTextSize(1);
                info_canvas.setTextColor(TFT_RED);
                info_canvas.setTextDatum(TC_DATUM);
                info_canvas.drawString("INSOMNIA MODE", g_screen_w / 2, 0);
                info_canvas.setTextDatum(TL_DATUM);
            }

            if (!g_auto_poweroff_inhibit && !g_is_charging) {
                uint32_t elapsed = now - g_last_activity_ms;
                if (elapsed < k_auto_poweroff_ms) {
                    uint32_t remaining = k_auto_poweroff_ms - elapsed;
                    if (remaining <= k_poweroff_warning_ms) {
                        uint32_t secs = (remaining + 999) / 1000;
                        info_canvas.setTextSize(1);
                        info_canvas.setTextColor(ORANGE);
                        info_canvas.setTextDatum(TC_DATUM);
                        info_canvas.drawString(String("OFF in ") + secs + "s",
                                               g_screen_w / 2, 12);
                        info_canvas.setTextDatum(TL_DATUM);
                    }
                }
            }

            info_canvas.setTextColor(ORANGE);
            info_canvas.setTextSize(2);
            int big_h = info_canvas.fontHeight();
            int batt = batt_percent;
            String batt_str = String(batt) + "%";
            if (g_is_charging) {
                batt_str += "+";
            }

            info_canvas.setTextColor(WHITE);
            info_canvas.setTextSize(2);
            int line_h = info_canvas.fontHeight();

            info_canvas.setTextSize(1);
            int ip_h = info_canvas.fontHeight();

            int temp_gap = 6;
            int block_h = big_h + temp_gap + line_h * 3 + 6;
            int avail_h = g_info_h - ip_h - 6;
            if (avail_h < block_h) avail_h = block_h;
            int start_y = (avail_h - block_h) / 2;
            int cx_text = g_screen_w / 2;

            info_canvas.setTextDatum(MC_DATUM);
            if (batt >= 80) {
                info_canvas.setTextColor(TFT_GREEN);
            } else if (batt >= 60) {
                info_canvas.setTextColor(TFT_YELLOW);
            } else if (batt >= 40) {
                info_canvas.setTextColor(ORANGE);
            } else if (batt >= 20) {
                info_canvas.setTextColor(TFT_RED);
            } else {
                info_canvas.setTextColor(M5.Display.color888(160, 0, 0));
            }
            info_canvas.setTextSize(2);
            info_canvas.drawString(batt_str, cx_text, start_y + big_h / 2);

            info_canvas.setTextSize(2);
            int y1 = start_y + big_h + temp_gap + line_h / 2;
            info_canvas.setTextColor(WHITE);
            info_canvas.drawString("avg:" + String(g_last_avg, 1), cx_text, y1);
            int y2 = y1 + line_h + 2;
            info_canvas.setTextColor(ORANGE);
            info_canvas.drawString("max:" + String(g_last_max, 1), cx_text, y2);
            int y3 = y2 + line_h + 2;
            info_canvas.setTextColor(M5.Display.color888(80, 160, 255));
            info_canvas.drawString("min:" + String(g_last_min, 1), cx_text, y3);

            info_canvas.setTextSize(1);
            info_canvas.setTextDatum(BC_DATUM);
            int bottom_y = g_info_h - 2;
            if (g_hotspot_mode) {
                info_canvas.setTextColor(ORANGE);
                info_canvas.drawString(String("IP: ") + g_ap_ip_str, cx_text, bottom_y);
                info_canvas.drawString(String("SSID: ") + k_hotspot_ssid,
                                       cx_text, bottom_y - ip_h - 2);
            } else if (g_wifi_connected) {
                info_canvas.setTextColor(WHITE);
                info_canvas.drawString(g_ip_str, cx_text, bottom_y);
            } else {
                info_canvas.setTextColor(ORANGE);
                info_canvas.drawString(k_wifi_placeholder, cx_text, bottom_y);
            }

            info_canvas.setTextDatum(TL_DATUM);

            float rotation = getTextRotationDeg();
            if (rotation == 0.0f) {
                info_canvas.pushSprite(0, g_top_h);
            } else {
                float cx = g_screen_w / 2.0f;
                float cy = g_top_h + g_info_h / 2.0f;
                info_canvas.pushRotateZoom(cx, cy, rotation, 1.0f, 1.0f);
            }
            M5.Display.endWrite();
        } else {
            drawFrame(temp_data, false);
        }
    } else {
        delay(1);
    }

    server.handleClient();

    if (had_activity) {
        recordActivity();
    }
    uint32_t now_check = millis();
    if (g_hotspot_mode && !g_is_charging) {
        if (now_check - g_hotspot_last_activity_ms >= k_hotspot_timeout_ms) {
            disableWifi();
        }
    }
    if (!g_auto_poweroff_inhibit && !g_is_charging) {
        if (!g_pending_poweroff && now_check - g_last_activity_ms >= k_auto_poweroff_ms) {
            g_pending_poweroff = true;
            playPoweroffChime();
        }
    }
}

