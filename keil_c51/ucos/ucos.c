/*
*********************************************************************************************************
*********************************************************************************************************
* File : ucos.c
* By : Minglie
* Date :
*********************************************************************************************************
*/


#include "ucos.h"


unsigned char OSTCBCur;

unsigned char  os_task_index = 0;
OSTCB_TypeDef ucos_task[config_max_tasks];



void os_init()
{


}


//创建一个任务
void OSTaskCreate(void (*task)(void* p_arg))
{
	int taskinx = os_task_index++;
	ucos_task[taskinx].task = task;
	ucos_task[taskinx].one.enable = 1;
}



void OSTimeTick()
{
	int i=0;
	for ( i = 0; i < os_task_index; i++) {
		if (ucos_task[i].delay == 0) ucos_task[i].one.rdy = 1;
		else ucos_task[i].delay--;
		if (ucos_task[i].one.rdy && ucos_task[i].one.enable)
		{
			OSTCBCur = i;
			ucos_task[i].task();
		}
	}
}

