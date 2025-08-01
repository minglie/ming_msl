# ming_msl

[Ming_MSL](https://github.com/minglie/ming_msl) 协议（Ming Serial Logic）是一种基于定极性脉宽编码（Fixed-Polarity Pulse Width Encoding, FP-PW）的单线、非归零（Non-Return-to-Zero）串行通信协议。
该协议仅使用一根 SDA 线，采用位序决定极性，电平持续时间编码位值，具有结构简单、实现容易、抗干扰强等优点，适用于低速、周期性、对时序要求不高的应用场景，并支持拓展为双向通信模式。


| 属性          | 内容说明                                                     |
| ----------- | -------------------------------------------------------- |
| **协议类型**    | 单线异步串行通信协议，仅使用 1 根 SDA 信号线                               |
| **编码方式**    | 固定极性 + Tick 脉宽编码（5tick = 0，10tick = 1）       |
| **极性策略**    | 每个 bit 的 SDA 电平极性由其位序（奇偶性）决定：<br>奇数位为低，偶数位为高             |
| **位值判断**    | 通过电平保持时长编码 bit 值：`5 tick` 表示 `0`，`10 tick` 表示 `1`,允许±2tick宽容误差     |
| **帧结构**      | `起始位`（低5+高5） → `数据位 × N` → `停止位`（低5+高5） → `帧间隔`（≥25 tick） |
| **tick 时钟**   |  tick 推荐为 `1ms`，由系统定时器或分频器产生                            |
| **传输位宽**    | 默认 8 bit，可配置扩展为 4/6/8/16/32 bit 等等,位宽在主从之间需一致配置                            |
| **通信速率**    | 自定义,典型帧传输时间为100毫秒             |
| **应用场景**    | 慢速传感器数据采集、低频控制命令发出、MCU/FPGA简单互联                         |
| **起始位** | 表示一帧通信开始，用于初始化收发状态、锁定通信节奏，并为主机加载待发送数据 |
| **数据位** | 通过SDA电平**极性**+**时长** 双重编码,按位序映射 bit 值  |
| **停止位** | 表示一帧通信结束，用于触发接收完成事件，避免因SDA短路或断路导致的误判与残帧 |
| **帧间隔** | 帧与帧之间保持固定高电平间隔，用于与起始位形成清晰边界，避免连续帧误判 |
| **异常帧**   | 非帧间隔期间 SDA 电平持续 ≥20 tick 不跳变，即判定为异常帧  
| **抗干扰性**     | 接收方将 8~20 tick 识别为 `1`，具备 ±容差抗抖动能力，支持软时钟系统  
| **自恢复** | 通讯线被剪断时主从在 20 tick 内可识别异常；重新接通后可通过起始帧自动恢复同步 |

# wokwi 在线演示
[wokwi演示](https://wokwi.com/projects/436060085417312257)

# 移植
为实现跨平台兼容，Ming_MSL 提供 4 个底层函数指针接口。  
其中 `id` 参数用于区分多实例（如多个通信通道或设备）：
## 移植接口
``` c++
// 读取 SDA 引脚电平（0 或 1）
typedef int  (MslPinRead)(uint8_t id);

// 设置 SDA 引脚电平（0 或 1）
typedef void (MslPinWrite)(uint8_t id, uint8_t v);

// 设置 SDA 引脚方向（0：输出，1：输入）
typedef void (MslPinDir)(uint8_t id, uint8_t v);

// 接收事件回调（如接收到数据、发送完成或错误）
typedef void (MslOutEvent)(uint8_t id,MSL_Event_TypeDef eventType, uint32_t data);
```


## 四种工作模式
``` c++
typedef enum {
    MSL_MODE_MASTER = 0,        // 主机模式
    MSL_MODE_SLAVE,             // 从机模式
    MSL_MODE_MASTER_ONLY_SEND,  // 仅发送模式
    MSL_MODE_SLAVE_ONLY_RECEIVE // 仅接收模式
} MslMode_TypeDef;
```

## 三种对外事件
``` c++
typedef enum {
    MSL_EVENT_SEND = 0,         // 发送完成事件
    MSL_EVENT_RECEIVE ,         // 接收完成事件
    MSL_EVENT_ERROR             // 错误事件
} MSL_Event_TypeDef;
```


# 目录结构


```markdown
ming_msl
│  README.md
│─src
│    ├─cpp
│    │      MingMsl.cpp
│    │      MingMsl.h
│    │
│    └─verilog
│            msl_master_sender.v
│            msl_slave_receiver.v
│            tb.v
│
├─examples
│  ├─clion_esp32_wokwi
│  ├─fpga_sim
│  ├─keil_c51
│  └─proteus
```


本项目已实现：

 C++ 主从 [MingMsl.h](src/cpp/MingMsl.h)

 Verilog 主  [msl_master_sender.v](src/verilog/msl_master_sender.v)

 Verilog 从  [msl_master_receiver.v](src/verilog/msl_slave_receiver.v)

# 通讯距离
> ming_msl 默认tick取1ms,主要是为了好算和够用。
本表基于5V TTL 电平、普通排线/杜邦线、主从共地良好的前提条件， 
在实际项目用过 tick=800us,1m线长的情况
实际环境如有电磁干扰、接地浮动、电源不稳，应适当加大 tick 或引入缓冲电路。


| 通信距离   | 最小 tick | 实际电平持续时间范围          | 稳定性评级    | 说明                                   |
| ------ | --------------- | ------------------- | -------- | ------------------------------------ |
| ≤ 1 米  | ≥ **100 µs**    | 0 = 500 µs，1 = 1 ms | ⭐⭐⭐⭐⭐ 极稳 | 线短干净，可实现非常快的周期，适合嵌入式系统内部板间连接         |
| 1–3 米  | ≥ **200 µs**    | 0 = 1 ms，1 = 2 ms   | ⭐⭐⭐⭐ 稳定  | GND 共地良好可运行稳定，适合模块与主控之间短距连接          |
| 3–5 米  | ≥ **400 µs**    | 0 = 2 ms，1 = 4 ms   | ⭐⭐⭐ 一般   | 边沿略钝，tick 不能太小，防止±2 tick容错误判         |
| 5–10 米 | ≥ **1 ms**      | 0 = 5 ms，1 = 10 ms  | ⭐⭐ 有风险   | 建议缓冲器 + 降速 tick，波形完整性开始下降            |
| ≥ 10 米 | ≥ **2 ms**      | 0 = 10 ms，1 = 20 ms | ⭐ 容错压缩   | SDA 波形缓慢，钝化严重，建议硬件驱动/终端匹配/EMI保护等辅助机制 |


# 通讯速度

起始位：10 tick  
数据位：8 bit × 最大 10 tick = 80 tick  
停止位：10 tick  
帧间隔：25 tick  
**总计约：125 tick**
## 🕒 一帧传输耗时（125  tick）
| Tick 值     | 一帧耗时    |
| ---------- | ------- |
| **10 µs**  | 1.25 ms |
| **100 µs** | 12.5 ms |
| **800 µs** | 100 ms  |
| **1 ms**   | 125 ms  |
| **2 ms**   | 250 ms  |
| **10 ms**  | 1.25 秒  |
| **100 ms** | 12.5 秒  |

⚠️ 注：帧耗时取决于 bit 值（0 用 5 tick，1 用 10 tick），本表按全为 1（最慢）估算，实际通信通常更快。

# 时序图
``` mermaid 
%%{init: {'theme': 'default'}}%%
timeline
    title Ming_MSL 单帧传输时序图（8bit 数据，含起始/停止/间隔）

    section 起始位
    低电平（5 tick）: 0
    高电平（5 tick）: 5

    section 数据位（发送 0b10100110，bit7→bit0）
    bit7 = 1，低（10 tick）: 10
    bit6 = 0，高（5 tick）: 20
    bit5 = 1，低（10 tick）: 25
    bit4 = 0，高（5 tick）: 35
    bit3 = 0，低（5 tick）: 40
    bit2 = 1，高（10 tick）: 45
    bit1 = 1，低（10 tick）: 55
    bit0 = 1，高（5 tick）: 65

    section 停止位
    低电平（5 tick）: 70
    高电平（5 tick）: 75

    section 帧间隔
    空闲高电平（25 tick）: 80

```

## 8bit位宽通讯波形
> 紫色是主机控制总线
黄色是从机控制总线

![images/](img/1.jpg)
![images/](img/2.jpg)

## 单向应用场景
![images/](img/3.jpg)

## 8bit位宽 ila 波形
![images/](img/5.png)





# 设计理念

幼儿园时，凳子坏了，  
我就用一个木头疙瘩当凳子。  

因为
> **越简单的东西，越不容易坏，越不容易错。**


Ming_MSL 的价值，远不止“节省几根线材”。  
它解决的是**多信号线通信系统**在**稳定性、安全性、维护性**上的关键痛点：

- 焊点多 → 容易虚焊；
- 线越多 → 故障概率越高；
- 信号杂 → 干扰越强；
- 一条线异常 → 整个系统可能误判、误控。
- 异常不可感知 → 故障发生但主控毫无察觉，易引发危险行为。


**Ming_MSL 提供了一种结构简洁、易于移植、波形清晰、无时钟依赖的通用单线通信协议。**

它特别适用于以下场景：

- **资源受限设备**
- **低速、稳定周期性通信**
- **芯片之间的状态传递、命令触发、状态刷新等低频信息交互**
- **不适合引入复杂通信栈的简洁应用场合**


## 类似协议对比
Ming_MSL 的最大优势是对时序不敏感，具备天然的误差容忍能力，避免误差累积。
它采用“电平持续时间”来表达 bit 值，允许 ±数 ms 的偏差，即使中断打断、定时不准也能正确通信。

相比之下，常见软件 UART 若主从波特率略有偏差，或中断延迟，就容易产生误码。而 DS18B20 等协议则对时序极度敏感，需 µs 级精度，移植和稳定性都不易保证。

## 串口对比
| 特性        | UART 串口                  | Ming\_MSL 协议                |
| --------- | ------------------------ | --------------------------- |
| **引脚数**   | 至少 2 根（TXD + RXD）        | 仅 1 根 SDA                   |
| **通信速率**  | 常见为 9600\~115200 bps     | 典型每 bit ≥ 5ms，帧级通信，低速稳定     |
| **时钟依赖**  | 基于波特率计时，主从需严格时钟同步        | 完全不依赖外部时钟，同步由时间宽度隐式编码       |
| **波形结构**  | 边沿敏感，波形需精准对齐，易被干扰影响      | 电平持续时间判断位值，容忍 ±数 ms 抖动，抗干扰强 |
| **硬件支持**  | 需 UART IP、波特率器、FIFO、寄存器等 | 任意 GPIO + 软件定时即可，无需硬件支持     |
| **应用场景**  | 中速数据交互（如模块通信、终端打印）       | 慢速状态/命令传输（如控制信号、状态刷新）       |
| **实现复杂度** | 协议栈、校验位、帧格式相对复杂          | 极简起始位/停止位 + 时长判位，结构简单       |
| **芯片兼容性** | 大多数 MCU/FPGA 有硬件 UART    | 任意平台皆可实现，仅需读写 SDA 电平和方向控制接口 |

# 高级扩展

以下三类扩展机制在提升协议灵活性、拓展适用场景的同时，
也可能带来通信速率的下降、硬件资源要求或软件解析复杂度的增加。
适用于对协议功能性有更高需求的系统环境。

## 🌐 星型一主多从通信
  需要从传输位宽拿出几位来标识从机
``` 
         +-----------+
         |           |
         |  Master   |
         |           |
         +-----+-----+
               |
               |
     +---+  +--+  +--+  +--+  +--+
     | S1|  |S2|  |S3|  |S4|  |S5| ...
     +---+  +--+  +--+  +--+  +--+
```
## ⏱️ 动态波特率与节奏编码
Ming_MSL 协议基于“电平跳变 + tick 时间长度”的时间编码原理，
从设备可在起始位或结束位的跳变处测量 SDA 电平的持续时间，从而自动推导出当前帧的 tick 基准。

该特性可被扩展为“节奏编码通道”，即通过 tick 长度隐式携带帧级元信息

## 🎛 使用 ADC 采样 SDA 信号
得益于 Ming_MSL 协议**极低的传输速率、稳定的信号节奏、基于持续时间的编码机制**，接收端 不依赖于高速数字IO口 或 中断系统。
SDA 信号甚至可直接叠加在 DC 电源线上，通过串入电阻、电容耦合与钳位保护等手段提取 SDA 波形，
实现“电源线+信号线”复用，简化系统布线



本协议及其参考实现代码完全开源，欢迎移植、修改。


> 作者：minglie  
> 协议版本：v1.0  
> 更新时间：2025-07-12  
