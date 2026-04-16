module bloom_core #(
    parameter int KEY_WIDTH    = 32,
    parameter int FILTER_WIDTH = 1024
) (
    input   logic                       clk,
    input   logic                       rst_n,

    input   logic                       start,
    input   logic                       insert,
    input   logic                       query,
    input   logic                       clear,
    input   logic   [KEY_WIDTH-1:0]     key,

    output  logic                       done,
    output  logic                       busy,
    output  logic                       query_result
);

    localparam int IDX_WIDTH = $clog2(FILTER_WIDTH);

    typedef enum logic [1:0] {IDLE, INSERT, QUERY, CLEAR} state_t;

    state_t                         state_q,        state_d;
    logic   [KEY_WIDTH-1:0]         key_q,          key_d;
    logic   [FILTER_WIDTH-1:0]      filter_bits_q,  filter_bits_d;
    logic                           query_result_q, query_result_d;
    logic                           done_q,         done_d;
    logic                           busy_q,         busy_d;

    logic   [KEY_WIDTH-1:0]         h1, h2, h3;
    logic   [IDX_WIDTH-1:0]         idx1, idx2, idx3;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state_q        <=  IDLE;
            key_q          <=  '0;
            filter_bits_q  <=  '0;
            query_result_q <=  1'b0;
            done_q         <=  1'b0;
            busy_q         <=  1'b0;
        end else begin
            state_q        <=  state_d;
            key_q          <=  key_d;
            filter_bits_q  <=  filter_bits_d;
            query_result_q <=  query_result_d;
            done_q         <=  done_d;
            busy_q         <=  busy_d;
        end
    end

    always_comb begin
        state_d         =   state_q;
        key_d           =   key_q;
        filter_bits_d   =   filter_bits_q;
        query_result_d  =   query_result_q;
        done_d          =   done_q;

        h1              =   key_q ^ (key_q >> 16);
        h2              =   key_q * 32'h045D9F3B;
        h3              =   key_q + (key_q << 6) + (key_q >> 2);

        idx1            =   h1[IDX_WIDTH-1:0];
        idx2            =   h2[IDX_WIDTH-1:0];
        idx3            =   h3[IDX_WIDTH-1:0];

        case (state_q)
            IDLE: begin
                if (start && $onehot({insert, query, clear})) begin
                    key_d   =   key;

                    if      (insert) state_d =   INSERT;
                    else if (query)  state_d =   QUERY;
                    else             state_d =   CLEAR;
                end
            end

            INSERT: begin
                filter_bits_d[idx1] =   1'b1;
                filter_bits_d[idx2] =   1'b1;
                filter_bits_d[idx3] =   1'b1;

                query_result_d      =   1'b0;
                state_d             =   IDLE;
            end

            QUERY: begin
                query_result_d  =   filter_bits_q[idx1] &&
                                    filter_bits_q[idx2] &&
                                    filter_bits_q[idx3];

                state_d         =   IDLE;
            end

            CLEAR: begin
                filter_bits_d   =   '0;
                query_result_d  =   1'b0;

                state_d         =   IDLE;
            end

            default: state_d    =   IDLE;
        endcase

        busy_d  =   (state_d != IDLE);

        if (state_d != IDLE)
            done_d  =   1'b0;
        else if (state_q != IDLE)
            done_d  =   1'b1;
    end

    assign  done            =   done_q;
    assign  busy            =   busy_q;
    assign  query_result    =   query_result_q;
endmodule