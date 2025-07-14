/*
*********************************************************************************************************
*********************************************************************************************************
* File : ming_mcu_single_line.h
* By : Minglie
* Date :
*********************************************************************************************************
*/

#ifndef __ming_mcu_single_line_h
#define __ming_mcu_single_line_h


//启动单线通讯任务
void start_ming_mcu_single_line_task0(void);



#define config_single_wire_task0_mode 0 //单线工作模式，0为主机，1为从机
#define config_com_task0_lengh 8 //配置通信有效位数，只能取2,4,6,8
//引脚配置
#define COM    P1_7
#define COM_C  P1_7

//task0的共享变量
//发送寄存器
extern unsigned char task0_tx_buf;
//接收寄存器
extern unsigned char task0_rx_buf;




#endif

