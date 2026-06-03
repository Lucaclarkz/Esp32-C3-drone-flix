// led.ino (GPIO 2 & GPIO 10 - Independent Mode Without Externs)
#include <Arduino.h>

const int CALIBRATION_LED = 2;  // Sensor Calibration & Status LED
const int LOW_BATTERY_LED = 10; // Low Battery LED
const int BATTERY_ADC_PIN = 0;  // Flix ရဲ့ ဘက်ထရီဖတ်သော Analog Pin (ဥပမာ ပြောင်းနိုင်သည်)

unsigned long lastBlinkTime = 0;
bool ledState = false;
unsigned long startTime = 0;

void setLED(bool value) {
    digitalWrite(CALIBRATION_LED, value ? HIGH : LOW);
}

void setupLED() {
    pinMode(CALIBRATION_LED, OUTPUT);
    pinMode(LOW_BATTERY_LED, OUTPUT);
    
    digitalWrite(CALIBRATION_LED, LOW);
    digitalWrite(LOW_BATTERY_LED, LOW);
    
    startTime = millis(); // စက်စတင်ပွင့်သည့်အချိန်ကို မှတ်သားခြင်း
}

void updateLED() {
    unsigned long currentTime = millis();

    // ==========================================
    // ၁။ SENSOR CALIBRATION & STATUS LED (GPIO 2)
    // ==========================================
    // ပါဝါဖွင့်ပြီး ပထမ ၄ စက္ကန့် (4000ms) အတွင်း ဆင်ဆာစစ်နေစဉ် မီးအသေလင်းမည်
    if (currentTime - startTime < 8000) {
        setLED(true); 
    } 
    // ၄ စက္ကန့်ကျော်သွားတာနဲ့ (Calibration ပြီးပြီဟု သတ်မှတ်ကာ) မောင်းနေရင်းပါ တဖျတ်ဖျတ် Blink မည်
    else {
        if (currentTime - lastBlinkTime >= 150) { 
            lastBlinkTime = currentTime;
            ledState = !ledState;
            setLED(ledState);
        }
    }

    // ==========================================
    // ၂။ LOW BATTERY LED SYSTEM (GPIO 10)
    // ==========================================
    // အခြားဖိုင်မှ ဗို့အားလှမ်းမယူတော့ဘဲ ပင်မှတိုက်ရိုက် ဖတ်ရှုခြင်း
    int rawAnalog = analogRead(BATTERY_ADC_PIN);
    float localVoltage = (rawAnalog / 4095.0) * 3.3 * 2.0; // Resistor Divider တွက်ချက်မှုပုံသေနည်း

    // ညီလေးသတ်မှတ်ထားသော 3.3V အောက်ရောက်ပါက မီးထလင်းမည်
    if (localVoltage > 0.5 && localVoltage <= 3.30) {
        digitalWrite(LOW_BATTERY_LED, HIGH); 
    } else {
        digitalWrite(LOW_BATTERY_LED, LOW);  
    }
}