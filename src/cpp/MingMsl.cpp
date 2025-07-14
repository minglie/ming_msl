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
            //初始化发送
            dirFun(m_id,MSL_DIR_OUTPUT);
            writeFun(m_id,0);
            m_txDataTemp = m_txData;
            m_datCur = 0;
            m_comErr = 0;
            MslDelay(5);
            m_state = 1;
            break;
        }
        case 1: {//拉高延时5个tick准备发送数据
            dirFun(m_id,MSL_DIR_INPUT);
            MslDelay(5);
            m_state = 2;
            break;
        }
        case 2: {//发送第 偶数位(从左数)
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
        case 3: {//发送第 奇数位(从左数)
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
            if (m_datCur>m_bitLength + 1) //m_datCur==10, bit[0:9]发送完
            {
                m_levelCount = 0;
                m_datCur = 0;
                if(emitEvent!= nullptr) {
                    emitEvent(m_id, MSL_EVENT_SEND, m_txData);
                }
                //仅发送模式则不接收
                if(m_mslMode==MSL_MODE_MASTER_ONLY_SEND){
                    MslDelay(25); //准备下一次通信
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
            //开始接收
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id))//等待对方发送,对齐
            {
                m_state = 4;
                m_levelCount++; //
                if (m_levelCount>20) //4ms
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //溢出,跳到异常分支
                }
            }
            else
            {
                m_bitLevelS = 0;
                m_rxDataTemp = 0;
                m_datCur = 0;
                m_state = 5;
                m_levelCount = 1; //当前分支也算采集一次
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
                if (m_levelCount>20) //4ms溢出
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //溢出,跳到异常分支
                }
            }
            else
            {
                if (m_datCur>0) m_rxDataTemp <<= 1; //如果已经收到一些数据，将得到的数据向左移动
                m_state = 6;
            }
            #if CON_TIMR_TICK_DEBUG==1
              writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
            #endif
            break;
        }
        case 6: { //////////////////////////////////////
            if (m_levelCount >= 8) m_rxDataTemp++; //超过7个tick认为是1
            m_bitLevelS ^= 1;
            m_datCur++;
            if (m_datCur >= m_bitLength) //m_datCur==8说明[0:7]数据已经接收完毕
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
                m_state = 5; //接收下一位数据
                //这里可采集一次
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
            if (readFun(m_id) == 0) //等待bit[8]
            {
                m_levelCount++;
                if (m_levelCount>20) //4ms
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //溢出,跳到异常分支
                }
            }
            else
            {
                dirFun(m_id,MSL_DIR_INPUT);
                MslDelay(25); //准备下一次通信
                m_state = 0;
                m_noBack = 0;
            }
            break;
        }
        case 255: { //////////////////////////////////////
            //异常分支
            dirFun(m_id,MSL_DIR_INPUT);
            m_noBack = 1;//主板不回复
            if (readFun(m_id) == 1)
            {
                m_waitComLCount = 0;
                m_waitComHCount++;
                m_state = 255;
                if (m_waitComHCount>20) //持续高4ms,重新发送
                {

                    m_state = 0;
                    m_waitComHCount = 0;
                }
            }
            else
            { //总线被拉低
                m_waitComHCount = 0;
                m_waitComLCount++;
                if (m_waitComLCount>80)//16ms
                {
                    m_comErr = 1; //总线被拉低
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
            ////初始化分支
            dirFun(m_id,MSL_DIR_INPUT);
            m_levelCount = 0;
            m_waitComLCount = 0;
            m_waitComHCount = 0;
            m_comErr = 0;
            m_state = 1;
            break;
        }
        case 1: { //等待起始位
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id))
            {
                m_state = 1;
                m_waitComHCount++;
                if (m_waitComHCount>100)//持续拉高100个tick
                {
                    m_state = 0; //重新开始接收
                }
                if (m_waitComLCount >= 2)//检测到大于2个tick以上低电平
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
        case 2: {//开始接收
            dirFun(m_id,MSL_DIR_INPUT);
            if (readFun(m_id))//等待对方发送,对齐
            {
                m_state = 2;
                m_levelCount++; //
                if (m_levelCount>20) //4ms
                {

                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //溢出,跳到异常分支
                }
            }
            else
            {
                m_bitLevelS = 0;
                m_rxDataTemp = 0;
                m_datCur = 0;
                m_state = 3;
                m_levelCount = 1; //当前分支也算采集一次
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
                if (m_levelCount>20) //4ms溢出
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //溢出,跳到异常分支
                }
            }
            else
            { //m_datCur>0 ,说明已经得到至少1bit数据
                if (m_datCur>0) m_rxDataTemp <<= 1; //将得到的数据向左移动
                m_state = 4;
            }
            #if CON_TIMR_TICK_DEBUG==1
                writeDebugFun(m_id,m_debugPinVal=m_debugPinVal^1);
            #endif
            break;
        }
        case 4: {
            if (m_levelCount >= 8) m_rxDataTemp++; //超过8个tick认为是1
            m_bitLevelS ^= 1;
            m_datCur++;
            if (m_datCur >= m_bitLength) //数据已经接收完毕
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
                m_state = 3; //接收下一位数据
                //这里可采集一次
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
            if (readFun(m_id) == 0) //等待bit[8]
            {
                m_levelCount++;
                if (m_levelCount>20) //4ms
                {
                    m_levelCount = 0;
                    m_waitComHCount = 0;
                    m_waitComLCount = 0;
                    m_state = 255; //溢出,跳到异常分支
                }
            }
            else
            {
                //仅接收模式则不发送,准备下次接收
                if(m_mslMode==MSL_MODE_SLAVE_ONLY_RECEIVE){
                    m_state = 0;
                    break;
                }
                MslDelay(10); //延时2ms，准备发送
                //初始化发送
                m_txDataTemp = m_txData;
                m_datCur = 0;
                m_state = 6;
            }
            break;
        }

        case 6: {//发送第偶数位(从左数)
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
        case 7: {//发送第奇数位(从左数)
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
            if (m_datCur>m_bitLength + 1) //m_datCur==10, bit[0:9]发送完
            {
                m_levelCount = 0;
                m_datCur = 0;
                dirFun(m_id,MSL_DIR_INPUT);
                MslDelay(5);//延时1ms，准备下一次接收
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
        case 255: { //异常分支
            dirFun(m_id,MSL_DIR_INPUT);
            m_rxData = 0;
            if (readFun(m_id))
            {
                m_waitComLCount = 0;
                m_waitComHCount++;
                m_state = 255;
                if (m_waitComHCount>20) //持续高,重新接收
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
                    m_comErr = 1; //总线被拉低
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

