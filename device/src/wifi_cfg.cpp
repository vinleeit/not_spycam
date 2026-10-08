#include "wifi_cfg.h"

#include <Preferences.h>

extern const IPAddress AP_IP(192, 168, 0, 69);
static const IPAddress AP_GW(192, 168, 0, 69);
static const IPAddress AP_MASK(255, 255, 255, 0);

extern const char *AP_SSID = "ESP32-CAM-Setup";
static const char *AP_PASS = "12345678"; // >= 8 chars, or nullptr for open

Preferences prefs;

bool loadCfg(WifiCfg &c)
{
    prefs.begin("wifi_cfg", true);
    c.ssid = prefs.getString("ssid", "");
    c.pass = prefs.getString("pass", "");
    c.hostname = prefs.getString("host", "esp32-cam");
    c.useStatic = prefs.getBool("static", false);
    c.ip = prefs.getString("ip", "");
    c.gw = prefs.getString("gw", "");
    c.mask = prefs.getString("mask", "255.255.255.0");
    c.dns = prefs.getString("dns", "");
    prefs.end();
    return c.ssid.length() > 0;
}

void saveCfg(const WifiCfg &c)
{
    prefs.begin("wifi_cfg", false);
    prefs.putString("ssid", c.ssid);
    prefs.putString("pass", c.pass);
    prefs.putString("host", c.hostname);
    prefs.putBool("static", c.useStatic);
    prefs.putString("ip", c.ip);
    prefs.putString("gw", c.gw);
    prefs.putString("mask", c.mask);
    prefs.putString("dns", c.dns);
    prefs.end();
}

// ---------- Station connect ----------
bool connectSTA(const WifiCfg &c, uint32_t timeoutMs)
{
    WiFi.setHostname(c.hostname.c_str()); // must be set before begin()
    WiFi.mode(WIFI_STA);

    if (c.useStatic)
    {
        IPAddress ip, gw, mask, d;
        if (ip.fromString(c.ip) && gw.fromString(c.gw) && mask.fromString(c.mask))
        {
            if (!d.fromString(c.dns))
                d = gw; // fall back to gateway as DNS
            WiFi.config(ip, gw, mask, d);
        }
        else
        {
            Serial.println("Bad static IP settings, using DHCP");
        }
    }

    WiFi.begin(c.ssid.c_str(), c.pass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs)
        delay(250);

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("Connected, IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    return false;
}

void connectAP()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK); // fixed 192.168.0.69
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(100);
}