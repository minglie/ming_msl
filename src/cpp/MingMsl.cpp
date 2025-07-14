#include "MingMsl.h"

#define CON_TIMR_TICK_DEBUG 1

MslPinRead * MingMsl::readFun= nullptr;
MslPinWrite * MingMsl::writeFun= nullptr;
MslPinDir * MingMsl::dirFun= nullptr;
MslPinWrite * MingMsl::writeDebugFun= nullptr;
MslOutEvent * MingMsl::emitEvent= nullptr;

void MingMsl::Init(uint8_t id,MslMode_TypeDef mslMode,uint8_t bitLength) {
    m_id=id;
    if(bitLength%2==0){
        m_bitLength=bitLength;
    } else{
        m_bitLength=bitLength+1;
    }
    if(m_bitLength==0){
        m_bitLength=2;
    }
    if(m_bitLength>32){
        m_bitLength=32;
    }
    m_txData=0;
    m_rxData=0;
    m_maxBitValue=((uint32_t)1)<<(m_bitLength-1);
    m_mslMode=mslMode;
    m_state=0;
    m_delay=0;
    m_enable = 1;
    m_debugPinVal=0;
    dirFun(m_id,MSL_DIR_INPUT);
}

bool MingMsl::ExchangeData(uint32_t sendData,uint32_t * receiveData) {
    if(m_comErr||!m_enable){
        return false;
    }
    m_txData=sendData;
    *receiveData=m_rxData;
    return true;
}

void  MingMsl::MslDelay(uint8_t ticks){
    m_rdy =0;
    m_delay =ticks-1;
}

void MingMsl::MasterOnTick() {
    switch (m_state)
    {
        case 0: {
            //��ʼ������
            dirFun(m_id,MSL_DIR_OUTPUT);
            writeFun(m_id,0);
            m_txDataTemp = m_txData;
            m_datCur = 0;
            m_comErr = 0;
            MslDelay(5);
            m_state = 1;
            break;
        }
        case 1: {//������ʱ5��tick׼����������
            dirFun(m_id,MSL_DIR_INPUT);
            MslDelay(5);
            m_state = 2;
            break;
        }
        case 2: {//���͵� ż��λ(������)
            dirFun(m_id,MSL_DIR_OUTPUT);
            writeFun(m_id,0);
            if (m_txDataTemp & m_maxBitValue)
            {
                MslDelay(10);
            }
            else
            {
                MslDelay(5);
            }
            m_txDataTemp <<= 1;
            m_datCur++;
            m_state = 3;
            break;
        }
        case 3: {//���͵� ����λ(������)
            dirFun(m_id,MSL_DIR_INPUT);
            if (m_txDataTemp & m_maxBitValue)
            {
                MslDelay(10);
            }
            else
            {
                MslDelay(5);
            }
            m_txDataTemp <<= 1;
            m_datCur++;
            if (m_datCur>m_bitLength + 1) //m_datCur==10, bit[0:9]������
            {
                m_levelCount = 0;
                m_datCur = 0;
                if(emitEvent!= nullptr) {
                    emitEvent(m_id, MSL_EVENT_SEND, m_txData);
                }
                //������ģʽ�򲻽���
                if(m_mslMode==MSL_MODE_MASTER_ONLY_SEND){
                    MslDelay(25); //׼����һ��ͨ��
                    m_state = 0;
                    break;
                }
                m_state = 4;
            }
            else
            {
                m_state = 2;
            }
            break;
        }
        case 4: { //////////////////////////////////////
            //��ʼ����
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id))//�ȴ��Է�����,����
            {
                m_state = 4;
                m_levelCount++; //
                if (m_levelCount>20) //4ms
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //���,�����쳣��֧
                }
            }
            else
            {
                m_bitLevelS = 0;
                m_rxDataTemp = 0;
                m_datCur = 0;
                m_state = 5;
                m_levelCount = 1; //��ǰ��֧Ҳ��ɼ�һ��
                #if CON_TIMR_TICK_DEBUG==1
                   writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
                #endif
            }
            break;
        }
        case 5: { //////////////////////////////////////
            dirFun(m_id,MSL_DIR_INPUT);
            if (m_bitLevelS == readFun(m_id))
            {
                m_levelCount++;
                if (m_levelCount>20) //4ms���
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //���,�����쳣��֧
                }
            }
            else
            {
                if (m_datCur>0) m_rxDataTemp <<= 1; //����Ѿ��յ�һЩ���ݣ����õ������������ƶ�
                m_state = 6;
            }
            #if CON_TIMR_TICK_DEBUG==1
              writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
            #endif
            break;
        }
        case 6: { //////////////////////////////////////
            if (m_levelCount >= 8) m_rxDataTemp++; //����7��tick��Ϊ��1
            m_bitLevelS ^= 1;
            m_datCur++;
            if (m_datCur >= m_bitLength) //m_datCur==8˵��[0:7]�����Ѿ��������
            {
                m_levelCount = 0;
                m_state = 7;
                m_rxData = m_rxDataTemp;
                if(emitEvent!= nullptr){
                    emitEvent(m_id, MSL_EVENT_RECEIVE, m_rxData);
                }

            }
            else
            {
                m_levelCount = 0;
                m_state = 5; //������һλ����
                //����ɲɼ�һ��
                if (m_bitLevelS == readFun(m_id)){
                    m_levelCount=1;
                    #if CON_TIMR_TICK_DEBUG==1
                      writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
                    #endif
                }
            }
            break;
        }
        case 7: { //////////////////////////////////////
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id) == 0) //�ȴ�bit[8]
            {
                m_levelCount++;
                if (m_levelCount>20) //4ms
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //���,�����쳣��֧
                }
            }
            else
            {
                dirFun(m_id,MSL_DIR_INPUT);
                MslDelay(25); //׼����һ��ͨ��
                m_state = 0;
                m_noBack = 0;
            }
            break;
        }
        case 255: { //////////////////////////////////////
            //�쳣��֧
            dirFun(m_id,MSL_DIR_INPUT);
            m_noBack = 1;//���岻�ظ�
            if (readFun(m_id) == 1)
            {
                m_waitComLCount = 0;
                m_waitComHCount++;
                m_state = 255;
                if (m_waitComHCount>20) //������4ms,���·���
                {

                    m_state = 0;
                    m_waitComHCount = 0;
                }
            }
            else
            { //���߱�����
                m_waitComHCount = 0;
                m_waitComLCount++;
                if (m_waitComLCount>80)//16ms
                {
                    m_comErr = 1; //���߱�����
                    m_waitComLCount = 0;
                    if(emitEvent!= nullptr){
                        emitEvent(m_id, MSL_EVENT_ERROR, 0);
                    }
                }
            }
            break;
        }
    }
}

void MingMsl::SlaveOnTick() {
    switch (m_state)
    {
        case 0: {
            ////��ʼ����֧
            dirFun(m_id,MSL_DIR_INPUT);
            m_levelCount = 0;
            m_waitComLCount = 0;
            m_waitComHCount = 0;
            m_comErr = 0;
            m_state = 1;
            break;
        }
        case 1: { //�ȴ���ʼλ
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id))
            {
                m_state = 1;
                m_waitComHCount++;
                if (m_waitComHCount>100)//��������100��tick
                {
                    m_state = 0; //���¿�ʼ����
                }
                if (m_waitComLCount >= 2)//��⵽����2��tick���ϵ͵�ƽ
                {
                    m_levelCount = 0;
                    m_state = 2;
                }
            }
            else
            {
                m_waitComLCount++;
                m_state = 1;
            }
            break;
        }
        case 2: {//��ʼ����
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id))//�ȴ��Է�����,����
            {
                m_state = 2;
                m_levelCount++; //
                if (m_levelCount>20) //4ms
                {

                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //���,�����쳣��֧
                }
            }
            else
            {
                m_bitLevelS = 0;
                m_rxDataTemp = 0;
                m_datCur = 0;
                m_state = 3;
                m_levelCount = 1; //��ǰ��֧Ҳ��ɼ�һ��
                #if CON_TIMR_TICK_DEBUG==1
                    writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
                #endif
            }
            break;
        }

        case 3: {
            if (m_bitLevelS == readFun(m_id))
            {
                m_levelCount++;
                if (m_levelCount>20) //4ms���
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //���,�����쳣��֧
                }
            }
            else
            { //m_datCur>0 ,˵���Ѿ��õ�����1bit����
                if (m_datCur>0) m_rxDataTemp <<= 1; //���õ������������ƶ�
                m_state = 4;
            }
            #if CON_TIMR_TICK_DEBUG==1
                writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
            #endif
            break;
        }
        case 4: {
            if (m_levelCount >= 8) m_rxDataTemp++; //����8��tick��Ϊ��1
            m_bitLevelS ^= 1;
            m_datCur++;
            if (m_datCur >= m_bitLength) //�����Ѿ��������
            {
                m_levelCount = 0;
                m_state = 5;
                m_rxData = m_rxDataTemp;
                if(emitEvent!= nullptr){
                    emitEvent(m_id, MSL_EVENT_RECEIVE, m_rxData);
                }
            }
            else
            {
                m_levelCount = 0;
                m_state = 3; //������һλ����
                //����ɲɼ�һ��
                if (m_bitLevelS == readFun(m_id)){
                    m_levelCount=1;
                    #if CON_TIMR_TICK_DEBUG==1
                        writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
                    #endif
                }
            }
            break;
        }
        case 5: {
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id) == 0) //�ȴ�bit[8]
            {
                m_levelCount++;
                if (m_levelCount>20) //4ms
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //���,�����쳣��֧
                }
            }
            else
            {
                //������ģʽ�򲻷���,׼���´ν���
                if(m_mslMode==MSL_MODE_SLAVE_ONLY_RECEIVE){
                    m_state = 0;
                    break;
                }
                MslDelay(10); //��ʱ2ms��׼������
                //��ʼ������
                m_txDataTemp = m_txData;
                m_datCur = 0;
                m_state = 6;
            }
            break;
        }

        case 6: {//���͵�ż��λ(������)
            dirFun(m_id,MSL_DIR_OUTPUT);
            writeFun(m_id,0);
            if (m_txDataTemp & m_maxBitValue)
            {
                MslDelay(10);
            }
            else
            {
                MslDelay(5);
            }
            m_txDataTemp <<= 1;
            m_datCur++;
            m_state = 7;
            break;
        }
        case 7: {//���͵�����λ(������)
            dirFun(m_id,MSL_DIR_INPUT);
            if (m_txDataTemp & m_maxBitValue)
            {
                MslDelay(10);
            }
            else
            {
                MslDelay(5);
            }
            m_txDataTemp <<= 1;
            m_datCur++;
            if (m_datCur>m_bitLength + 1) //m_datCur==10, bit[0:9]������
            {
                m_levelCount = 0;
                m_datCur = 0;
                dirFun(m_id,MSL_DIR_INPUT);
                MslDelay(5);//��ʱ1ms��׼����һ�ν���
                if(emitEvent!= nullptr){
                    emitEvent(m_id, MSL_EVENT_SEND, m_txData);
                }
                m_state = 0;
            }
            else
            {
                m_state = 6;
            }
            break;

        }
        case 255: { //�쳣��֧
            dirFun(m_id,MSL_DIR_INPUT);
            m_rxData = 0;
            if (readFun(m_id))
            {
                m_waitComLCount = 0;
                m_waitComHCount++;
                m_state = 255;
                if (m_waitComHCount>20) //������,���½���
                {
                    m_state = 0;
                    m_waitComHCount = 0;
                }
            }
            else
            {
                m_waitComHCount = 0;
                m_waitComLCount++;
                if (m_waitComLCount>80)//16ms
                {
                    m_comErr = 1; //���߱�����
                    m_waitComLCount = 0;
                }
            }
            break;
        }

    }
}

uint8_t MingMsl::GetComErr()
{
    return m_comErr;
}

void MingMsl::SetEnable(uint8_t enable){
    this->m_enable = enable;
}

uint32_t MingMsl::GetReceiveData(){
    return m_rxData;
}

void  MingMsl::SendData(uint32_t sendData){
    m_txData=sendData;
}

void MingMsl::OnTick() {
    if (m_delay == 0) m_rdy = 1;
    else m_delay--;
    if (!(m_rdy && m_enable))
    {
        return;
    }
    if(m_mslMode==MSL_MODE_MASTER||m_mslMode==MSL_MODE_MASTER_ONLY_SEND){
        MasterOnTick();
    } else{
        SlaveOnTick();
    }
}

