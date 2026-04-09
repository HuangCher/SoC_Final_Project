module bloom_core (
    input   logic           clk,
    input   logic           rst_n,

    input   logic           start,
    input   logic           insert,
    input   logic           query,
    input   logic           clear,
    input   logic   [31:0]  key_in,

    output  logic           done,
    output  logic           busy,
    output  logic           query_result
);
  
    typedef enum logic  [2:0]  {IDLE, HASH, INSERT, QUERY, CLEAR, DONE, XXX = 'x} state_t;
    state_t state_q, state_d;

    logic   [1023:0]    filter_bits;        // bloom bit storage
    logic   [31:0]      h1, h2, h3;         // hash vals
    logic   [9:0]       idx1, idx2, idx3    // index vals

    logic   op_insert_q, op_insert_d;
    logic   op_query_q, op_query_d;
    logic   op_clear_q, op_clear_d;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            done            <=  '0';
            busy            <=  '0';
            query_result    <=  '0';
            filter_bits     <=  '0;

            op_insert_q     <=  '0';
            op_query_q      <=  '0';
            op_clear_q      <=  '0';
        end else begin
            state_q     <=  state_d;
            op_insert_q <=  op_insert_d;
            op_query_q  <=  op_query_d;
            op_clear_q  <=  op_clear_d;
        end
    end

    always_comb begin
        state_d = state_q;
        
        case (state_q)
            IDLE: 
            HASH:
            INSERT:
            QUERY:
            CLEAR:
            DONE:

            default: state_d = XXX;
        endcase
    end


endmodule