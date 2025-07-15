// msl_slave_receiver.v
//| 起始位 | 数据位 | 结束位 | 帧间隔(S_GAP) | 起始位 | 数据位 | 结束位 | 帧间隔(S_GAP) |
/// @brief MSL Slave Receiver
/// @param P_DATA_WIDTH 接收位宽
/// @param P_CLK_FREQ 时钟频率，单位 Hz
module msl_slave_receiver #(
    parameter P_DATA_WIDTH = 8,
    parameter P_CLK_FREQ = 50_000_000
)(
    input  wire        i_clk,
    input  wire        i_rst_n,
    input  wire        i_msl_sda,
    output reg [P_DATA_WIDTH-1:0] o_data,
    output reg        o_msl_1ms
);

    // 1ms分频
    localparam L_TICK_1MS = P_CLK_FREQ / 1000 - 1;
    reg [31:0] r_clk_1ms_cnt;
    reg        r_tick_1ms;
    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_clk_1ms_cnt <= 0;
            r_tick_1ms <= 0;
            o_msl_1ms <= 0;
        end else if (r_clk_1ms_cnt == L_TICK_1MS) begin
            r_clk_1ms_cnt <= 0;
            r_tick_1ms <= 1;
            o_msl_1ms <= ~o_msl_1ms;
        end else begin
            r_clk_1ms_cnt <= r_clk_1ms_cnt + 1;
            r_tick_1ms <= 0;
        end
    end

    // 状态机
    localparam S_IDLE  = 3'd0;
    localparam S_START = 3'd1;
    localparam S_RECV  = 3'd2;
    localparam S_STOP  = 3'd3;
    localparam S_GAP   = 3'd4;
    reg [2:0] r_state, r_next_state;
    reg [7:0] r_cnt;
    reg [7:0] r_bit_cnt;
    reg [P_DATA_WIDTH-1:0] r_rx_data_temp;
    reg r_last_sda;



    // 状态转移
    always @(*) begin
        r_next_state = r_state;
        case (r_state)
            S_IDLE:  if (i_msl_sda == 1'b0) r_next_state = S_START;
            S_START: if (r_cnt == 4'd0 && i_msl_sda == 1'b0 && r_last_sda == 1'b1) r_next_state = S_RECV;
            S_RECV:  if (r_bit_cnt == P_DATA_WIDTH) r_next_state = S_STOP;
            S_STOP:  if (r_cnt == 4'd9) r_next_state = S_GAP;
            S_GAP:   if (r_cnt >22) r_next_state = S_IDLE;
        endcase
        if(r_next_state !=S_GAP && r_next_state != S_IDLE && r_cnt>22) begin
             r_next_state=S_GAP;
        end
    end

    // 数据接收
    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_state <= S_IDLE;
            r_cnt <= 0;
            r_bit_cnt <= 0;
            r_rx_data_temp <= 0;
            o_data<=0;
            r_last_sda <= 1'b1;
        end else if (r_tick_1ms) begin
            r_state <= r_next_state;
            r_last_sda <= i_msl_sda;
            case (r_state)
                S_IDLE: begin
                    r_cnt <= 0;
                    r_bit_cnt <= 0;
                    r_rx_data_temp <= 0;
                end
                S_START: begin
                    if (i_msl_sda == 1'b0) begin
                        r_cnt <= r_cnt + 1;
                    end else begin
                        r_cnt <= 0;
                        r_bit_cnt <= 0;
                    end
                end
                S_RECV: begin
                    if (i_msl_sda == r_last_sda) begin
                        r_cnt <= r_cnt + 1;
                    end else begin
                        // 电平变化，判决上一位
                        if (r_bit_cnt < P_DATA_WIDTH) begin
                            if (r_cnt >= 7)
                                r_rx_data_temp[P_DATA_WIDTH-1-r_bit_cnt] <= 1'b1;
                            else
                                r_rx_data_temp[P_DATA_WIDTH-1-r_bit_cnt] <= 1'b0;
                             r_bit_cnt <= r_bit_cnt + 1;
                        end
                        r_cnt <= 0;
                    end
                end
                S_STOP: begin
                    o_data <= r_rx_data_temp;
                    if (r_cnt == 4'd9) r_cnt <= 0;
                    else r_cnt <= r_cnt + 1;
                end
                S_GAP: begin
                    r_cnt <= r_cnt + 1;
                end
            endcase
        end
    end
endmodule
