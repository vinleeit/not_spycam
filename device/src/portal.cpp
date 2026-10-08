#include "portal.h"
#include "wifi_cfg.h"

#include <WebServer.h>
#include <DNSServer.h>

WebServer server(80);
DNSServer dnsServer; // makes phones auto-open the page
bool portalMode = false;

// ---------- Helpers ----------
String htmlEscape(const String &s)
{
    String o;
    for (char c : s)
    {
        switch (c)
        {
        case '&':
            o += "&amp;";
            break;
        case '<':
            o += "&lt;";
            break;
        case '>':
            o += "&gt;";
            break;
        case '"':
            o += "&quot;";
            break;
        case '\'':
            o += "&#39;";
            break;
        default:
            o += c;
        }
    }
    return o;
}

// ---------- Web pages ----------
void handleRoot()
{
    WifiCfg c;
    loadCfg(c);

    String html = R"rawhtml(
        <!DOCTYPE html>
        <html>
            <head>
                <meta charset=utf-8>
                <meta name=viewport content='width=device-width,initial-scale=1'>
                <title>WiFi Setup</title>

                <style>
                    body{font-family:sans-serif;max-width:420px;margin:20px auto;padding:0 12px}
                    label{display:block;margin-top:12px;font-weight:600}
                    input,button{width:100%;padding:8px;margin-top:4px;box-sizing:border-box}
                    button{margin-top:20px;padding:12px}
                </style>
            </head>
            <body>
                <h2>WiFi Setup</h2>
                <form method=POST action=/save>
                    <label>SSID</label>
                    <input name=ssid maxlength=32 required value="%SSID%">

                    <label>Password</label>
                    <input type=password name=pass maxlength=64 placeholder="<leave blank for open network>">

                    <label>Hostname</label>
                    <input name=host maxlength=32 value="%HOSTNAME%">
                    
                    <label>
                        <input type=checkbox name=static value=1 style='width:auto'
                           onchange="document.getElementById('st').style.display=this.checked?'block':'none' %IS_CHECKED%">
                        Use static IP
                    </label>

                    <div id=st style="display:%IS_SHOWN%">
                        <label>IP address</label>
                        <input name=ip placeholder=192.168.0.1 value="%IP%">

                        <label>Gateway</label>
                        <input name=gw placeholder=192.168.1.1 value="%GW%">

                        <label>Subnet mask</label>
                        <input name=mask value="%MASK%">

                        <label>NS</label>
                        <input name=dns placeholder=8.8.8.8 value="%DNS%">

                    </div>

                    <button type=submit>Save &amp; reboot</button>
                </form>
            </body>
        </html>
    )rawhtml";

    html.replace("\%SSID\%", htmlEscape(c.ssid));
    html.replace("\%HOSTNAME\%", htmlEscape(c.hostname));
    html.replace("\%IP\%", htmlEscape(c.ip));
    html.replace("\%GW\%", htmlEscape(c.gw));
    html.replace("\%MASK\%", htmlEscape(c.mask));
    html.replace("\%DNS\%", htmlEscape(c.dns));
    html.replace("\%IS_CHECKED\%", c.useStatic ? "checked" : "");
    html.replace("\%IS_SHOWN\%", c.useStatic ? "block" : "none");

    server.send(200, "text/html", html);
}

void handleSave()
{
    WifiCfg c;
    c.ssid = server.arg("ssid");
    c.pass = server.arg("pass");
    c.hostname = server.arg("host");
    c.useStatic = server.hasArg("static");
    c.ip = server.arg("ip");
    c.gw = server.arg("gw");
    c.mask = server.arg("mask");
    c.dns = server.arg("dns");

    if (c.ssid.length() == 0)
    {
        server.send(400, "text/plain", "SSID is required");
        return;
    }
    if (c.hostname.length() == 0)
        c.hostname = "esp32-cam";

    saveCfg(c);
    server.send(200, "text/html",
                "<html><body style='font-family:sans-serif'><h3>Saved.</h3>"
                "<p>Rebooting and connecting to <b>" +
                    htmlEscape(c.ssid) + "</b>...</p></body></html>");
    delay(1500);
    ESP.restart();
}

void handleNotFound()
{
    // Captive portal: send any other URL to the setup page
    server.sendHeader("Location", "http://192.168.0.69/", true);
    server.send(302, "text/plain", "");
}

void startPortal(IPAddress ip)
{
    portalMode = true;
    dnsServer.start(53, "*", ip); // answer every DNS query with our IP

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(handleNotFound);
    server.begin();

    // Serial.printf("Portal up: join \"%s\", open http://%s\n",
    //               AP_SSID, WiFi.softAPIP().toString().c_str());
}

void portalLoop()
{
    dnsServer.processNextRequest();
    server.handleClient();
}