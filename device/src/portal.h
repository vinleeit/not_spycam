#pragma once
#include <WiFi.h>

void startPortal(IPAddress ip);
void portalLoop();

extern bool portalMode;