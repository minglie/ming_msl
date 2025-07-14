#ifndef MING_ESP32_TC_MINGMSL_H
#define MING_ESP32_TC_MINGMSL_H

#include "stdint.h"

#define MSL_DEBUG_ENABLE  1

// ͨ��ģʽö��
typedef enum {
    MSL_MODE_MASTER = 0,        // ����ģʽ
    MSL_MODE_SLAVE,             // �ӻ�ģʽ
    MSL_MODE_MASTER_ONLY_SEND,         // ������ģʽ
    MSL_MODE_SLAVE_ONLY_RECEIVE       // ������ģʽ
} MslMode_TypeDef;

// ���ݷ���ö��
typedef enum {
    MSL_DIR_OUTPUT = 0,         // �������
    MSL_DIR_INPUT              // ���뷽��
} MSL_Direction_TypeDef;


//���ݷ���ö��
typedef enum {
    MSL_EVENT_SEND = 0,         // ��������¼�
    MSL_EVENT_RECEIVE ,         // ��������¼�
    MSL_EVENT_ERROR             // �����¼�
} MSL_Event_TypeDef;


typedef int(MslPinRead)(uint8_t id);
typedef void (MslPinWrite)(uint8_t id,uint8_t v);
typedef void (MslPinDir)(uint8_t id,uint8_t v);
//�����¼�
typedef void (MslOutEvent)(uint8_t id,MSL_Event_TypeDef eventType, uint32_t data);

class MingMsl {
private:
    uint8_t  m_id;
    //msl��4�ֹ���ģʽ,0:����,1:�ӻ�,2:�����ⷢ 3:�������
    MslMode_TypeDef  m_mslMode;
    //����ͨ����Чλ����ֻ��ȡ2��32��ż��
    uint8_t  m_bitLength;
    //���λ����ֵ
    uint32_t  m_maxBitValue;
    uint8_t m_rdy;
    uint8_t m_enable;
    uint8_t m_delay;  //��ʱ����
    uint8_t m_state; //״̬������
    //��ǰλ
    uint8_t m_datCur;
    //���߳����߼���
    uint8_t m_waitComHCount;
    //���߳����ͼ���
    uint8_t m_waitComLCount;
    //���ͼĴ���
    uint32_t m_txData;
    //���ͼĴ����ݴ�
    uint32_t m_txDataTemp;
    //���ռĴ���
    uint32_t m_rxData;
    //���ռĴ����ݴ�
    uint32_t m_rxDataTemp;
    //���ߵ�ƽ����ʱ�����
    uint8_t m_levelCount;
    //�ӻ����ظ�
    uint8_t  m_noBack;
    //���ߵ�ƽ�ߵ�״̬
    uint8_t  m_bitLevelS;
    //ͨѶ�����־
    uint8_t  m_comErr;
    uint8_t m_debugPinVal;
    //��ʱ����
    void  MslDelay(uint8_t ticks);
    //�������ں���
    void  MasterOnTick();
    //�ӻ����ں���
    void  SlaveOnTick();
public:
    //�����ߺ���
    static MslPinRead   *   readFun;
    //д���ߺ���
    static MslPinWrite  *   writeFun;
    //������
    static MslPinWrite  *   writeDebugFun;
    //�������߷���
    static MslPinDir    *   dirFun;
    //�����¼�
    static MslOutEvent  *   emitEvent;
    //��ʼ��
    void     Init(uint8_t id,MslMode_TypeDef mslMode,uint8_t bitLength);
    //ʹ��
    void     SetEnable(uint8_t enable);
    //��ȡ��������
    uint32_t GetReceiveData();
    //��������
    void     SendData(uint32_t sendData);
    //��ȡ���ߴ����־
    uint8_t  GetComErr();
    //���ݽ���
    bool  ExchangeData(uint32_t sendData,uint32_t * receiveData);
    //Ҫ��֤���ӵ�������һ��,������1ms
    void   OnTick();
};


#endif
