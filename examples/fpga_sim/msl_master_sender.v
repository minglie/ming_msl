//circuit
// msl_master_sender.v
//| 起始位 | 数据位 | 结束位 | 帧间隔(P_GAP) | 起始位 | 数据位 | 结束位 | 帧间隔(P_GAP) |
module msl_master_sender #(
    parameter P_DATA_WIDTH = 8,
    parameter P_SYSTEM_CLK = 50_000_000
) (
    input  wire        i_clk,
    input  wire        i_rst_n,
    input  wire [P_DATA_WIDTH-1:0] i_data,
    output reg         o_msl_sda,
    output reg         o_msl_1ms
);

    //1ms分频器
    localparam P_1MS_DIV = P_SYSTEM_CLK / 1000 - 1;
    reg [31:0] r_clk_1ms_cnt;
    reg        r_1ms_tick;
    // 1ms tick分频器
    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_clk_1ms_cnt <= 0;
            r_1ms_tick <= 0;
            o_msl_1ms<=0;
        end else if (r_clk_1ms_cnt == P_1MS_DIV) begin
            r_clk_1ms_cnt <= 0;
            r_1ms_tick <= 1;
            o_msl_1ms<=!o_msl_1ms;
        end else begin
            r_clk_1ms_cnt <= r_clk_1ms_cnt + 1;
            r_1ms_tick <= 0;
        end
    end

    //状态集
    localparam P_IDLE  = 3'd0;
    localparam P_START = 3'd1;
    localparam P_SEND  = 3'd2;
    localparam P_STOP  = 3'd3;
    localparam P_GAP   = 3'd4;
    reg [2:0] r_state, r_next_state;
    reg [7:0] r_cnt;
    reg [7:0] r_bit_cnt;
    reg [P_DATA_WIDTH-1:0] r_rx_data_temp;

    //转移集
    always @(*) begin
       r_next_state = r_state;
        case (r_state)
            P_IDLE:  r_next_state = P_START;
            P_START: if (r_cnt == 4'd9) r_next_state = P_SEND;
            P_SEND:  if (r_bit_cnt == P_DATA_WIDTH) r_next_state = P_STOP;
            P_STOP:  if (r_cnt == 4'd9) r_next_state = P_GAP;
            P_GAP:   if (r_cnt == 5'd24) r_next_state = P_IDLE;
        endcase
    end

    //动作集
    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_state      <= P_IDLE;
            r_cnt  <= 0;
            r_bit_cnt    <= 0;
            r_rx_data_temp <= 0;
            o_msl_sda    <= 1'b1;
        end else if(r_1ms_tick) begin
            r_state <= r_next_state;
            case (r_state)
                P_IDLE: begin
                    o_msl_sda    <= 1'b1;
                    r_bit_cnt    <= 0;
                    r_rx_data_temp <= i_data;
                    r_cnt  <= 0;
                end
                P_START: begin
                    o_msl_sda   <= (r_cnt < 5) ? 1'b0 : 1'b1;
                    if(r_cnt == 4'd9)  r_cnt <= 0;
                    else               r_cnt <= r_cnt + 1;
                end
                P_SEND: begin
                    //偶数位是低电平,奇数位是高电平
                    o_msl_sda   <= r_bit_cnt[0]==1;
                    //0:延时5个tick 1:延时10个tick
                    if (r_cnt == (r_rx_data_temp[P_DATA_WIDTH-1-r_bit_cnt]==0?4:9)) begin
                        r_cnt <= 0;
                        r_bit_cnt <= r_bit_cnt + 1;
                    end
                    else  r_cnt <= r_cnt + 1;
                end
                P_STOP: begin
                    o_msl_sda   <= (r_cnt < 5) ? 1'b0 : 1'b1;
                    if(r_cnt == 4'd9)  r_cnt <= 0;
                    else               r_cnt <= r_cnt + 1;
                end
                P_GAP: begin
                    o_msl_sda   <= 1'b1;
                    r_cnt <= r_cnt + 1;
                end
            endcase
        end
    end
endmodule
