#ifndef MING_ESP32_TC_MINGMSL_H
#define MING_ESP32_TC_MINGMSL_H

#include "stdint.h"

#define MSL_DEBUG_ENABLE  1

// 通信模式枚举
typedef enum {
    MSL_MODE_MASTER = 0,        // 主机模式
    MSL_MODE_SLAVE,             // 从机模式
    MSL_MODE_MASTER_ONLY_SEND,         // 仅发送模式
    MSL_MODE_SLAVE_ONLY_RECEIVE       // 仅接收模式
} MslMode_TypeDef;

// 数据方向枚举
typedef enum {
    MSL_DIR_OUTPUT = 0,         // 输出方向
    MSL_DIR_INPUT              // 输入方向
} MSL_Direction_TypeDef;


//数据方向枚举
typedef enum {
    MSL_EVENT_SEND = 0,         // 发送完成事件
    MSL_EVENT_RECEIVE ,         // 接收完成事件
    MSL_EVENT_ERROR             // 错误事件
} MSL_Event_TypeDef;


typedef int(MslPinRead)(uint8_t id);
typedef void (MslPinWrite)(uint8_t id,uint8_t v);
typedef void (MslPinDir)(uint8_t id,uint8_t v);
//发出事件
typedef void (MslOutEvent)(uint8_t id,MSL_Event_TypeDef eventType, uint32_t data);

class MingMsl {
private:
    uint8_t  m_id;
    //msl的4种工作模式,0:主机,1:从机,2:单向外发 3:单向接收
    MslMode_TypeDef  m_mslMode;
    //配置通信有效位数，只能取2到32的偶数
    uint8_t  m_bitLength;
    //最大位的数值
    uint32_t  m_maxBitValue;
    uint8_t m_rdy;
    uint8_t m_enable;
    uint8_t m_delay;  //延时计数
    uint8_t m_state; //状态机步骤
    //当前位
    uint8_t m_datCur;
    //总线持续高计数
    uint8_t m_waitComHCount;
    //总线持续低计数
    uint8_t m_waitComLCount;
    //发送寄存器
    uint32_t m_txData;
    //发送寄存器暂存
    uint32_t m_txDataTemp;
    //接收寄存器
    uint32_t m_rxData;
    //接收寄存器暂存
    uint32_t m_rxDataTemp;
    //总线电平持续时间计数
    uint8_t m_levelCount;
    //从机不回复
    uint8_t  m_noBack;
    //总线电平高低状态
    uint8_t  m_bitLevelS;
    //通讯错误标志
    uint8_t  m_comErr;
    uint8_t m_debugPinVal;
    //延时函数
    void  MslDelay(uint8_t ticks);
    //主机周期函数
    void  MasterOnTick();
    //从机周期函数
    void  SlaveOnTick();
public:
    //读总线函数
    static MslPinRead   *   readFun;
    //写总线函数
    static MslPinWrite  *   writeFun;
    //调试用
    static MslPinWrite  *   writeDebugFun;
    //设置总线方向
    static MslPinDir    *   dirFun;
    //发出事件
    static MslOutEvent  *   emitEvent;
    //初始化
    void     Init(uint8_t id,MslMode_TypeDef mslMode,uint8_t bitLength);
    //使能
    void     SetEnable(uint8_t enable);
    //获取接收数据
    uint32_t GetReceiveData();
    //发送数据
    void     SendData(uint32_t sendData);
    //获取总线错误标志
    uint8_t  GetComErr();
    //数据交换
    bool  ExchangeData(uint32_t sendData,uint32_t * receiveData);
    //要保证主从调用周期一致,建议用1ms
    void   OnTick();
};


#endif
