#ifndef __Ucos_H
#define __Ucos_H

#ifdef __cplusplus 
extern "C"
{
#endif


#define  config_max_tasks 2       //任务个数
#define  configTICK_RATE_us 800   //800us 配置心跳周期
#define  WHILE(a)
#define  OSTimeDly(k) {ucos_task[OSTCBCur].one.rdy =0; ucos_task[OSTCBCur].delay =k-1; }


 

	
  void   os_init();
	void   OSTimeTick();
	void   OSTaskCreate(void  (*task)());




	typedef struct
	{
		struct struct_tcb
		{
			unsigned rdy : 1;     //就绪
			unsigned enable : 1;  //启用禁用
		}one;
		unsigned char delay;  //延时计数
		unsigned char step; //状态机步骤
		void  (*task)();
	}OSTCB_TypeDef;


	extern unsigned char OSTCBCur;
	extern OSTCB_TypeDef ucos_task[config_max_tasks];


#ifdef __cplusplus 
}
#endif



#endif

