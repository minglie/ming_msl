/*
*********************************************************************************************************
*********************************************************************************************************
* File : ming_mcu_single_line.c
* By : Minglie
* Date :
*********************************************************************************************************
*/
#include<AT89X52.H>
#include "Main_Constant.h"
#include "ucos.h"
#include "ming_mcu_single_line.h"

//当前位
 unsigned char task0_dat_cur;
 //总线持续高计数
 unsigned char task0_wait_com_h_count;
//总线持续低计数
 unsigned char task0_wait_com_l_count;
 //发送寄存器
 unsigned char task0_tx_buf;
 //发送寄存器暂存
 unsigned char task0_tx_buf_temp;
 //接收寄存器
 unsigned char task0_rx_buf;
 //接收寄存器暂存
 unsigned char task0_rx_buf_temp;
//总线电平持续时间计数
 unsigned char task0_level_count;
 //从机不回复
 bit task0_bit_no_back;
 //总线电平高低状态
 bit task0_bit_level_s;
 //通讯错误标志
 bit task0_bit_com_err;



#if config_single_wire_task0_mode==0
void task0()
{
     WHILE(1){
				switch (ucos_task[OSTCBCur].step)
				{
						case 0: {
							//初始化发送，并发送起始位1ms
							COM_C = 0;
							COM = 0;
							task0_tx_buf_temp = task0_tx_buf;
							task0_dat_cur = 0;
							OSTimeDly(5);
							ucos_task[OSTCBCur].step = 1;
							break;
						}
						case 1: {//拉高延时1ms准备发送数据
							COM_C = 1;
							OSTimeDly(5);
							ucos_task[OSTCBCur].step = 2;
							break;
						}
						case 2: {//发送第0,2,4,6,8(从左数)
							COM_C = 0;
							COM = 0;
							if (task0_tx_buf_temp & 0x80)
							{
								OSTimeDly(10);
							}
							else
							{
								OSTimeDly(5);
							}
							task0_tx_buf_temp <<= 1;
							task0_dat_cur++;
							ucos_task[OSTCBCur].step = 3;
							break;
						}
						case 3: {//发送第1,3,5,7,9(从左数)
							COM_C = 1;
							if (task0_tx_buf_temp & 0x80)
							{
								OSTimeDly(10);
							}
							else
							{
								OSTimeDly(5);
							}
							task0_tx_buf_temp <<= 1;
							task0_dat_cur++;
							if (task0_dat_cur>config_com_task0_lengh + 1) //task0_dat_cur==10, bit[0:9]发送完
							{
								task0_level_count = 0;
								task0_dat_cur = 0;
								ucos_task[OSTCBCur].step = 4;
							}
							else
							{
								ucos_task[OSTCBCur].step = 2;
							}
							break;
						}
						case 4: { //////////////////////////////////////
									//开始接收
							COM_C = 1;
							if (COM)//等待对方发送,对齐
							{
								ucos_task[OSTCBCur].step = 4;
								task0_level_count++; //
								if (task0_level_count>20) //4ms
								{
									task0_level_count = 0;
									task0_wait_com_h_count = 0;
									task0_wait_com_l_count = 0;
									ucos_task[OSTCBCur].step = 255; //溢出,跳到异常分支
								}
							}
							else
							{
								task0_bit_level_s = 0;
								task0_rx_buf_temp = 0;
								task0_dat_cur = 0;
								task0_level_count = 0;
								ucos_task[OSTCBCur].step = 5;
							}
							break;
						}
						case 5: { //////////////////////////////////////
							COM_C = 1;
							if (task0_bit_level_s == COM)
							{
								task0_level_count++;
								if (task0_level_count>20) //4ms溢出
								{
									task0_level_count = 0;
									task0_wait_com_h_count = 0;
									task0_wait_com_l_count = 0;
									ucos_task[OSTCBCur].step = 255; //溢出,跳到异常分支
								}
							}
							else
							{
								if (task0_dat_cur>0) task0_rx_buf_temp <<= 1; //如果已经收到一些数据，将得到的数据向左移动
								ucos_task[OSTCBCur].step = 6;
							}
							break;
						}
						case 6: { //////////////////////////////////////
							if (task0_level_count >= 7) task0_rx_buf_temp++; //超过1.4ms认为是1
							task0_bit_level_s ^= 1;
							task0_dat_cur++;
							if (task0_dat_cur >= config_com_task0_lengh) //task0_dat_cur==8说明[0:7]数据已经接收完毕
							{
								task0_level_count = 0;
								ucos_task[OSTCBCur].step = 7;
								task0_rx_buf = task0_rx_buf_temp;
							}
							else
							{
								task0_level_count = 0;
								ucos_task[OSTCBCur].step = 5; //接收下一位数据
							}
							break;
						}
						case 7: { //////////////////////////////////////
							COM_C = 1;
							if (COM == 0) //等待bit[8]
							{
								task0_level_count++;
								if (task0_level_count>20) //4ms
								{
									task0_level_count = 0;
									task0_wait_com_h_count = 0;
									task0_wait_com_l_count = 0;
									ucos_task[OSTCBCur].step = 255; //溢出,跳到异常分支
								}
							}
							else
							{
								COM_C = 1;
								OSTimeDly(25); //延时5ms，准备下一次通信
								ucos_task[OSTCBCur].step = 0;
								task0_bit_no_back = 0;
							}
							break;
						}
						case 255: { //////////////////////////////////////
							//异常分支
					COM_C = 1;
					task0_bit_no_back = 1;//主板不回复
					if (COM == 1)
					{
						task0_wait_com_l_count = 0;
						task0_wait_com_h_count++;
						ucos_task[OSTCBCur].step = 255;
						if (task0_wait_com_h_count>20) //持续高4ms,重新发送
						{

							ucos_task[OSTCBCur].step = 0;
							task0_wait_com_h_count = 0;
						}
					}
					else
					{ //总线被拉低
						task0_wait_com_h_count = 0;
						task0_wait_com_l_count++;
						if (task0_wait_com_l_count>80)//16ms
						{
							task0_bit_com_err = 1; //总线被拉低
							task0_wait_com_l_count = 0;
						}
					}
					break;
				}
			}
		 }
}

#else 
void task0()
{
		WHILE(1){
			switch (ucos_task[OSTCBCur].step)
			{
					case 0: {
						////初始化分支
						COM_C = 1;
						task0_level_count = 0;
						task0_wait_com_l_count = 0;
						task0_wait_com_h_count = 0;
						ucos_task[OSTCBCur].step = 1;
						break;
					}
					case 1: { //等待起始位
						COM_C = 1;
						if (COM == 1)
						{
							ucos_task[OSTCBCur].step = 1;
							task0_wait_com_h_count++;
							if (task0_wait_com_h_count>100)//持续拉高200us*100=20ms
							{
								ucos_task[OSTCBCur].step = 0; //重新开始接收
							}
							if (task0_wait_com_l_count >= 2)//检测到大于400us以上低电平
							{
								task0_level_count = 0;
								ucos_task[OSTCBCur].step = 2;
							}
						}
						else
						{
							task0_wait_com_l_count++;
							ucos_task[OSTCBCur].step = 1;
						}
						break;
					}
					case 2: {//开始接收
						COM_C = 1;
						if (COM)//等待对方发送,对齐
						{
							ucos_task[OSTCBCur].step = 2;
							task0_level_count++; //
							if (task0_level_count>20) //4ms
							{

								task0_level_count = 0;
								task0_wait_com_h_count = 0;
								task0_wait_com_l_count = 0;
								ucos_task[OSTCBCur].step = 255; //溢出,跳到异常分支
							}
						}
						else
						{
							task0_level_count = 0;
							task0_bit_level_s = 0;
							task0_rx_buf_temp = 0;
							task0_dat_cur = 0;
							ucos_task[OSTCBCur].step = 3;
						}
						break;
					}

					case 3: {
						if (task0_bit_level_s == COM)
						{
							task0_level_count++;
							if (task0_level_count>20) //4ms溢出
							{
								task0_level_count = 0;
								task0_wait_com_h_count = 0;
								task0_wait_com_l_count = 0;
								ucos_task[OSTCBCur].step = 255; //溢出,跳到异常分支
							}
						}
						else
						{ //task0_dat_cur>0 ,说明已经得到至少1bit数据
							if (task0_dat_cur>0) task0_rx_buf_temp <<= 1; //将得到的数据向左移动
							ucos_task[OSTCBCur].step = 4;
						}
						break;
					}
					case 4: {
						if (task0_level_count >= 7) task0_rx_buf_temp++; //超过1.4ms认为是1
						task0_bit_level_s ^= 1;
						task0_dat_cur++;
						if (task0_dat_cur >= config_com_task0_lengh) //task0_dat_cur==8说明[0:7]数据已经接收完毕
						{
							task0_level_count = 0;
							ucos_task[OSTCBCur].step = 5;
							task0_rx_buf = task0_rx_buf_temp;
						}
						else
						{
							task0_level_count = 0;
							ucos_task[OSTCBCur].step = 3; //接收下一位数据
						}
						break;
					}
					case 5: {
						COM_C = 1;
						if (COM == 0) //等待bit[8]
						{
							task0_level_count++;
							if (task0_level_count>20) //4ms
							{
								task0_level_count = 0;
								task0_wait_com_h_count = 0;
								task0_wait_com_l_count = 0;
								ucos_task[OSTCBCur].step = 255; //溢出,跳到异常分支
							}
						}
						else
						{
							COM_C = 1;
							OSTimeDly(10); //延时2ms，准备发送
											 //初始化发送
							task0_tx_buf_temp = task0_tx_buf;
							task0_dat_cur = 0;
							ucos_task[OSTCBCur].step = 6;
						}
						break;
					}

					case 6: {//发送第0,2,4,6,8(从左数)
						COM_C = 0;
						COM = 0;
						if (task0_tx_buf_temp & 0x80)
						{
							OSTimeDly(10);
						}
						else
						{
							OSTimeDly(5);
						}
						task0_tx_buf_temp <<= 1;
						task0_dat_cur++;
						ucos_task[OSTCBCur].step = 7;
						break;
					}
					case 7: {//发送第1,3,5,7,9(从左数)
						COM_C = 1;
						if (task0_tx_buf_temp & 0x80)
						{
							OSTimeDly(10);
						}
						else
						{
							OSTimeDly(5);
						}
						task0_tx_buf_temp <<= 1;
						task0_dat_cur++;
						if (task0_dat_cur>config_com_task0_lengh + 1) //task0_dat_cur==10, bit[0:9]发送完
						{
							task0_level_count = 0;
							task0_dat_cur = 0;
							COM_C = 1;
							OSTimeDly(5);//延时1ms，准备下一次接收
							ucos_task[OSTCBCur].step = 0;
						}
						else
						{
							ucos_task[OSTCBCur].step = 6;
						}
						break;

					}
					case 255: { //异常分支
				COM_C = 1;
				task0_rx_buf = 0;
				if (COM == 1)
				{
					task0_wait_com_l_count = 0;
					task0_wait_com_h_count++;
					ucos_task[OSTCBCur].step = 255;
					if (task0_wait_com_h_count>20) //持续高4ms,重新接收
					{
						ucos_task[OSTCBCur].step = 0;
						task0_wait_com_h_count = 0;
					}
				}
				else
				{
					task0_wait_com_h_count = 0;
					task0_wait_com_l_count++;
					if (task0_wait_com_l_count>80)//16ms
					{
						task0_bit_com_err = 1; //总线被拉低
						task0_wait_com_l_count = 0;
					}
				}
				break;
			}
			
		}
	}
}

#endif 





void start_ming_mcu_single_line_task0(){

	OSTaskCreate(task0);
}
