module bloom_core (
    input   logic           clk,
    input   logic           rst_n,

    input   logic           start,
    input   logic           insert,
    input   logic           query,
    input   logic           clear,
    input   logic   [31:0]  key,

    output  logic           done,
    output  logic           busy,
    output  logic           query_result
);

    typedef enum logic  [2:0]  {IDLE, DECODE, INSERT, QUERY, CLEAR, DONE, XXX = 'x} state_t;

    // internal regs
    state_t             state_q,        state_d;
    logic   [31:0]      key_q,          key_d;
    logic               op_insert_q,    op_insert_d;
    logic               op_query_q,     op_query_d;
    logic               op_clear_q,     op_clear_d;
    logic   [1023:0]    filter_bits_q,  filter_bits_d;
    logic               query_result_q, query_result_d;

    // comb signals
    logic   [31:0]      h1, h2, h3;
    logic   [9:0]       idx1, idx2, idx3;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state_q         <=  IDLE;
            key_q           <=  '0;
            op_insert_q     <=  1'b0;
            op_query_q      <=  1'b0;
            op_clear_q      <=  1'b0;
            filter_bits_q   <=  '0;
            query_result_q  <=  1'b0;
        end else begin
            state_q         <=  state_d;
            key_q           <=  key_d;
            op_insert_q     <=  op_insert_d;
            op_query_q      <=  op_query_d;
            op_clear_q      <=  op_clear_d;
            filter_bits_q   <=  filter_bits_d;  
            query_result_q  <=  query_result_d;          
        end
    end

    always_comb begin
        // default comb vals
        done            =   1'b0;
        busy            =   1'b0;
        key_d           =   key_q;
        op_insert_d     =   op_insert_q;
        op_query_d      =   op_query_q;
        op_clear_d      =   op_clear_q;
        state_d         =   state_q;
        filter_bits_d   =   filter_bits_q;
        query_result_d  =   query_result_q;

        h1              =   key_q ^ (key_q >> 16);
        h2              =   key_q * 32'h045D9F3B;
        h3              =   key_q + (key_q << 6) + (key_q >> 2);

        idx1            =   h1[9:0];
        idx2            =   h2[9:0];
        idx3            =   h3[9:0];

        case (state_q)
            IDLE: begin
                // if only 1 op selected and permission to start
                if (start && $onehot({insert, query, clear})) begin
                    done        =   1'b0;
                    busy        =   1'b1;
                    key_d       =   key;

                    op_insert_d =   insert && !query && !clear; // combination check for redundancy
                    op_query_d  =   !insert && query && !clear;
                    op_clear_d  =   !insert && !query && clear; 

                    state_d     =   DECODE;
                end
            end

            // FIXME_jacob_CLEANUP: decode stage & op regs are not needed, remove & refactor
            //                      query_result logic must also be changed with this
            DECODE: begin
                busy            =   1'b1;

                case ({op_insert_q, op_query_q, op_clear_q})
                    3'b100:
                        state_d =   INSERT;
                    3'b010:
                        state_d =   QUERY;
                    3'b001:
                        state_d =   CLEAR;
                    default: 
                        state_d =   IDLE;
                endcase
            end

            INSERT: begin
                busy                =   1'b1;

                filter_bits_d[idx1] =   1'b1;
                filter_bits_d[idx2] =   1'b1;
                filter_bits_d[idx3] =   1'b1;

                query_result_d      =   1'b0;
                state_d             =   DONE;
            end

            QUERY: begin
                busy            =   1'b1;

                query_result_d  =   filter_bits_q[idx1] && 
                                    filter_bits_q[idx2] && 
                                    filter_bits_q[idx3];

                state_d         =   DONE;
            end
            
            CLEAR: begin
                busy            =   1'b1;

                filter_bits_d   =   '0;
                query_result_d  =   1'b0;

                state_d         =   DONE;
            end

            DONE: begin
                // FIXME_jacob_CLEANUP: done/busy logic is weird, standardize
                busy                =   1'b0;
                done                =   1'b1;

                op_insert_d         =   1'b0;
                op_query_d          =   1'b0;
                op_clear_d          =   1'b0;

                if (!start) begin
                    state_d         =   IDLE;
                end
            end

            default: state_d = XXX; // FIXME_jacob_CLEANUP: get rid of XXX state maybe?
        endcase
    end

    assign  query_result    =   query_result_q;
endmodule