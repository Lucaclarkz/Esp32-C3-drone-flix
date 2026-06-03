// wifi.ino (Flix Hybrid Code - Both Wi-Fi AP & ESP-NOW Active Together)

#include <freertos/queue.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include "Preferences.h"

extern Preferences storage; 

// ကွန်မန်းဒေတာအတွက် Structure (Remote ဘက်နှင့် တူရမည်)
typedef struct struct_message {
    float throttle;
    float yaw;
    float pitch;
    float roll;
    bool armStatus;
    bool wifiToggleRequest;
} struct_message;

struct_message rxData;

// Flix ရဲ့ Core Variables များကို လှမ်းယူခြင်း
extern float controlRoll, controlPitch, controlYaw, controlThrottle;
extern bool armed;
extern bool mavlinkConnected; 

const int W_DISABLED = 0, W_AP = 1, W_STA = 2;
int wifiMode = W_AP;
int udpLocalPort = 14550;
int udpRemotePort = 14550;
IPAddress udpRemoteIP = "255.255.255.255";
WiFiUDP udp;

uint8_t broadcastAddress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
bool espnowInitialized = false;

// ESP-NOW မှ ဒေတာဝင်လာလျှင် နောက်ကွယ်ကနေ ဖတ်မည့် Hardware Interrupt
void IRAM_ATTR onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len == sizeof(struct_message)) {
        memcpy(&rxData, data, sizeof(rxData));

        // --- ဖုန်းချိတ်ထားသော Device မရှိမှသာ Remote (ESP-NOW) ဂျွိုင်းစတစ်ကို အသက်သွင်းမည် ---
        if (WiFi.softAPgetStationNum() == 0 && !WiFi.isConnected()) {
            controlThrottle = rxData.throttle;
            controlYaw      = rxData.yaw;
            controlPitch    = rxData.pitch;
            controlRoll     = rxData.roll;
            armed           = rxData.armStatus; 
        }
    }
}

void setupWiFi() {
    print("Setup Hybrid Network (Wi-Fi & ESP-NOW)\n");
    WiFi.setSleep(false);

    // ၁။ Wi-Fi AP စနစ်ကို အရင်ဖွင့်ခြင်း
    if (wifiMode == W_AP) {
        WiFi.mode(WIFI_AP_STA); // AP ရော STA (ESP-NOW) ပါ ပြိုင်တူရရန် mode ပြောင်းသည်
        WiFi.softAP(storage.getString("WIFI_AP_SSID", "flix").c_str(), storage.getString("WIFI_AP_PASS", "flixwifi").c_str());
    }
    udp.begin(udpLocalPort);

    // ၂။ ESP-NOW စနစ်ကို ထပ်မံပူးတွဲဖွင့်ခြင်း
    if (esp_now_init() == ESP_OK) {
        espnowInitialized = true;
        esp_now_register_recv_cb(onReceive);
        
        // Peer အသေသတ်မှတ်ခြင်း
        esp_now_peer_info_t peerInfo;
        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 1; 
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
        
        print("EspNow: init OK\n");
    } else {
        print("EspNow: init failed\n");
    }
}

// ဒရုန်းဘက်က MAVLink ဒေတာများကို လေလှိုင်းထဲ ပြန်လွှင့်ထုတ်ခြင်း
void sendWiFi(const uint8_t *buf, int len) {
    // ဖုန်းချိတ်ထားရင် Wi-Fi UDP ပုံစံဖြင့် ပို့မည်
    if (WiFi.softAPgetStationNum() > 0 || WiFi.isConnected()) {
        udp.beginPacket(udpRemoteIP, udpRemotePort);
        udp.write(buf, len);
        udp.endPacket();
    } 
    // ဖုန်းမချိတ်ထားရင် ဒေတာများကို ESP-NOW ဖြင့် လွှင့်ထုတ်မည်
    else if (espnowInitialized) {
        (void) esp_now_send(broadcastAddress, buf, len);
    }
}

// ပင်မ Loop မှ MAVLink ဒေတာ ဖတ်သည့်အပိုင်း
int receiveWiFi(uint8_t *buf, int len) {
    // ဝိုင်ဖိုင်မှာ ဖုန်းချိတ်ထားတာ ရှိနေရင် မူရင်း UDP အတိုင်း ဖတ်ရှုမောင်းနှင်မည်
    if (WiFi.softAPgetStationNum() > 0 || WiFi.isConnected()) {
        udp.parsePacket();
        if (udp.remoteIP()) udpRemoteIP = udp.remoteIP();
        return udp.read(buf, len); 
    }
    
    // ဖုန်းမချိတ်ထားရင် Main loop အစား အပေါ်က onReceive ကနေ ဂျွိုင်းစတစ် တိုက်ရိုက်ကျွေးနေမည်ဖြစ်၍ 0 သာ ပြန်မည်
    return 0; 
}

void printWiFiInfo() {
    print("Mode: Hybrid (Wi-Fi AP & ESP-NOW Active)\n");
    print("SSID: %s\n", WiFi.softAPSSID().c_str());
    print("Clients: %d\n", WiFi.softAPgetStationNum());
}

void configWiFi(bool ap, const char *ssid, const char *password) {
    if (ap) {
        storage.putString("WIFI_AP_SSID", ssid);
        storage.putString("WIFI_AP_PASS", password);
    } else {
        storage.putString("WIFI_STA_SSID", ssid);
        storage.putString("WIFI_STA_PASS", password);
    }
    print("✓ Reboot to apply new settings\n");
}