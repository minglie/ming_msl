`timescale 1ns/1ps

module tb;

    // 参数定义
    parameter P_DATA_WIDTH = 32;
    parameter P_SYSTEM_CLK = 1000;
    parameter CLK_PERIOD = 20;  // 50MHz

    // 时钟、复位
    reg i_clk;
    reg i_rst_n;

    initial i_clk = 0;
    always #(CLK_PERIOD / 2) i_clk = ~i_clk;

    // 主发器接口
    reg  [P_DATA_WIDTH-1:0] i_data;
    wire o_msl_sda;
    wire o_msl_1ms_tx;

    msl_master_sender #(
        .P_DATA_WIDTH(P_DATA_WIDTH),
        .P_SYSTEM_CLK(P_SYSTEM_CLK)
    ) sender_inst (
        .i_clk     (i_clk),
        .i_rst_n   (i_rst_n),
        .i_data    (i_data),
        .o_msl_sda (o_msl_sda),
        .o_msl_1ms (o_msl_1ms_tx)
    );

    // 从接器接口
    wire [P_DATA_WIDTH-1:0] o_data;
    wire o_msl_1ms_rx;

    msl_slave_receiver #(
        .P_DATA_WIDTH(P_DATA_WIDTH),
        .P_SYSTEM_CLK(P_SYSTEM_CLK)
    ) receiver_inst (
        .i_clk     (i_clk),
        .i_rst_n   (i_rst_n),
        .i_msl_sda (o_msl_sda),     // 接收发送器输出
        .o_data    (o_data),
        .o_msl_1ms (o_msl_1ms_rx)
    );

    // 毫秒等待任务
    task wait_ms(input integer ms);
        integer i;
        begin
            for (i = 0; i < ms  / CLK_PERIOD; i = i + 1)
                @(posedge i_clk);
        end
    endtask

    // 仿真流程
    initial begin
        $display("[TB] Starting loopback test...");
        // 初始状态
        i_rst_n = 0;
        i_data  = 8'h12345678; // 要发送的数据
         #5
         i_rst_n = 1;
        // 等待一次完整通信
          #5000

        $display("[TB] Sender i_data = %02X", i_data);
        $display("[TB] Receiver o_data = %02X", o_data);

        if (o_data == i_data)
            $display("[TB] ✅ Loopback test passed!");
        else
            $display("[TB] ❌ Loopback test failed!");

         #5
         $stop;
       // $finish;
    end

endmodule
