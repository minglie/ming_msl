#include <AT89X52.H>
#include "Function.h"
#include "Main_Constant.h"
#include "ming_mcu_single_line.h"


/*
*********************************************************************************************************
*********************************************************************************************************
* File : main.c
* By : Minglie
* Date :
*********************************************************************************************************
*/

void main()
{
	  P2=0xff;
    mcu_init();
	  //启动单线通讯任务
		start_ming_mcu_single_line_task0();
	
    while (1)
    {
        tm0_counter();
        //此if语句最长周期不能超过800us,否则可能通信错误
        if (bt.one.ms5_s)
        {
					   //发送数据
					   task0_tx_buf=P0;
					  //接收数据
					   P3= task0_rx_buf ;
             bt.one.ms5_s = 0;
        }
        
        
       
    }
}






















//串口接收中断函数
// void serial() interrupt 4 using 3
// {
//     if (RI)
//     {
//         unsigned char ch;
//         RI = 0;
//         ch = SBUF;
//         usart_rx_buf[usart_rx_count] = ch;
//         usart_rx_count++;
//         if (usart_rx_count == 1)
//         {
//             usart_rx_count = 0;
//             bt.one.rx_f = 1;
//             usart_send_string(usart_rx_buf, 1);
//             //	 usart_send_char(0x55);
//         }
//     }
// }
