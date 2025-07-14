# ming_msl

ming_msl 是周期,低速,双向,软件型的通信

- keil_c51 是c51版本的

- clion_esp32_wokwi 是c++重写的

- fpga 是verilog  ming_msl主机和从机的实现

# 移植
实现如下3个函数
``` c++
//读引脚电平
typedef int(MslPinRead)(uint8_t id);
//改引脚电平
typedef void (MslPinWrite)(uint8_t id,uint8_t v);
//改引脚方向
typedef void (MslPinDir)(uint8_t id,uint8_t v);
```


# 四种工作模式
``` c++
typedef enum {
    MSL_MODE_MASTER = 0,        // 主机模式
    MSL_MODE_SLAVE,             // 从机模式
    MSL_MODE_MASTER_ONLY_SEND,  // 仅发送模式
    MSL_MODE_SLAVE_ONLY_RECEIVE // 仅接收模式
} MslMode_TypeDef;
```

# 三种对外事件
``` c++
typedef enum {
    MSL_EVENT_SEND = 0,         // 发送完成事件
    MSL_EVENT_RECEIVE ,         // 接收完成事件
    MSL_EVENT_ERROR             // 错误事件
} MSL_Event_TypeDef;
```

# 通讯位宽
``` markdown
 2到32之间的偶数
```

# 时序图
![images/](img/1.jpg)
![images/](img/2.jpg)
![images/](img/3.jpg)
![images/](img/4.png)
![images/](img/5.png)
