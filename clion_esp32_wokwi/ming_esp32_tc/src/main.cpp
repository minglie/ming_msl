#include <Arduino.h>
#include "MingMsl.h"
#include "UInt32FIFO.h"

UInt32FIFO s_fifo;

#define MASTER_PIN 15
#define SLAVE_PIN 23

#define MSL0_DEBUG_PIN  12
#define TIME0_TICK_DEBUG_PIN 13
#define TIME1_TICK_DEBUG_PIN 25
#define MSL1_DEBUG_PIN 26

void mslDigitalWriteDebug(uint8_t id, uint8_t v) {
    if(id==0){
        digitalWrite(MSL0_DEBUG_PIN,v);
    } else{
        digitalWrite(MSL1_DEBUG_PIN,v);
    }

}

int mslDigitalRead(uint8_t id) {
    int pin=MASTER_PIN;
    if(id==1){
        pin=SLAVE_PIN;
    }
    return digitalRead(pin);
}

// 实现MslPinWrite函数 - 写入引脚状态
void mslDigitalWrite(uint8_t id, uint8_t v) {
    int pin=MASTER_PIN;
    if(id==1){
        pin=SLAVE_PIN;
    }
    digitalWrite(pin, v);
}

// 实现MslPinDir函数 - 设置引脚方向
void mslPinMode(uint8_t id, uint8_t v) {
    int pin=MASTER_PIN;
    if(id==1){
        pin=SLAVE_PIN;
    }
    if(v==0){
        pinMode(pin, OUTPUT);
    } else{
        pinMode(pin, INPUT_PULLUP);
    }
}

static uint32_t master_sendData = 0;
static uint32_t slave_sendData = 0;
void onMslEvent(uint8_t id,MSL_Event_TypeDef eventType, uint32_t data) {
   if(id==1){
       //将从机接收的数据放入队列
       if(eventType==MSL_EVENT_RECEIVE){
           s_fifo.push(data);
           master_sendData++;
       }
   }
}


MingMsl masterMing_msl;
MingMsl slaveMing_msl;
void IRAM_ATTR onTimer0() {
    static uint8_t v=0;
    v=!v;
    digitalWrite(TIME0_TICK_DEBUG_PIN,v);
    masterMing_msl.OnTick();
}

void IRAM_ATTR onTimer1() {
    static uint8_t v=0;
    v=!v;
    digitalWrite(TIME1_TICK_DEBUG_PIN,v);
    slaveMing_msl.OnTick();
}


hw_timer_t *timer0 = NULL;
hw_timer_t *timer1 = NULL;
void setup() {
    Serial.begin(115200);
    pinMode(MASTER_PIN, INPUT_PULLUP);
    pinMode(SLAVE_PIN, INPUT_PULLUP);

    pinMode(MSL0_DEBUG_PIN, OUTPUT);
    pinMode(TIME0_TICK_DEBUG_PIN, OUTPUT);
    dacWrite(25, 0);
    dacWrite(26, 0);
    pinMode(25, OUTPUT);
    pinMode(26, OUTPUT);

    MingMsl::readFun=mslDigitalRead;
    MingMsl::writeFun=mslDigitalWrite;
    MingMsl::writeDebugFun=mslDigitalWriteDebug;
    MingMsl::dirFun=mslPinMode;
    MingMsl::emitEvent=onMslEvent;

    masterMing_msl.Init(0,MSL_MODE_MASTER_ONLY_SEND,32);
    slaveMing_msl.Init(1,MSL_MODE_SLAVE_ONLY_RECEIVE,32);




    // 初始化定时器：定时器编号 0，预分频器为 80，计数模式递增
    // ESP32 APB 时钟为 80 MHz，除以 80 则为 1 MHz（即 1 tick = 1 us）
    timer0 = timerBegin(0, 80, true);
    // 设定每 1000 us（即 1 ms）触发中断
    timerAttachInterrupt(timer0, &onTimer0, true);       // 绑定 ISR
    timerAlarmWrite(timer0, 1000, true);                // 每 1000us 自动重载
    timerAlarmEnable(timer0);


    timer1 = timerBegin(2, 80, true);
    // 设定每 1000 us（即 1 ms）触发中断
    timerAttachInterrupt(timer1, &onTimer1, true);       // 绑定 ISR
    timerAlarmWrite(timer1, 1000, true);                // 每 1000us 自动重载
    timerAlarmEnable(timer1);

}
void loop() {

    static uint32_t recveiveData = 0;
    masterMing_msl.ExchangeData(master_sendData,&recveiveData);
    slaveMing_msl.ExchangeData( slave_sendData,&recveiveData);
    while (s_fifo.pop(recveiveData)){
        Serial.printf("slaveMing_msl receiveData:%lx \r\n",recveiveData);
    }
}
