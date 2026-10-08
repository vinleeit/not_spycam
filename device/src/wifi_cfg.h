#pragma once
#include <WiFi.h>

struct WifiCfg
{
    String ssid, pass, hostname;
    bool useStatic = false;
    String ip, gw, mask, dns;
};

bool loadCfg(WifiCfg &c);
void saveCfg(const WifiCfg &c);
bool connectSTA(const WifiCfg &c, uint32_t timeoutMs = 15000);
void connectAP();

extern const IPAddress AP_IP;
extern const char *AP_SSID;