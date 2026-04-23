module bloom_axi #(
    parameter int ADDR_WIDTH    = 4,
    parameter int DATA_WIDTH    = 32,
    parameter int KEY_WIDTH     = 32,
    parameter int FILTER_WIDTH  = 1024
) (
    input   logic                       aclk,
    input   logic                       aresetn,

    // wr addr
    input   logic   [ADDR_WIDTH-1:0]    s_axi_awaddr,
    input   logic   [2:0]               s_axi_awprot,
    input   logic                       s_axi_awvalid,
    output  logic                       s_axi_awready,

    // write data
    input   logic   [DATA_WIDTH-1:0]    s_axi_wdata,
    input   logic   [DATA_WIDTH/8-1:0]  s_axi_wstrb,
    input   logic                       s_axi_wvalid,
    output  logic                       s_axi_wready,

    // write resp
    output  logic   [1:0]               s_axi_bresp,
    output  logic                       s_axi_bvalid,
    input   logic                       s_axi_bready,

    // rd addr
    input   logic   [ADDR_WIDTH-1:0]    s_axi_araddr,
    input   logic   [2:0]               s_axi_arprot,
    input   logic                       s_axi_arvalid,
    output  logic                       s_axi_arready,

    // rd data
    output  logic   [DATA_WIDTH-1:0]    s_axi_rdata,
    output  logic   [1:0]               s_axi_rresp,
    output  logic                       s_axi_rvalid,
    input   logic                       s_axi_rready
);

    // addr map
    localparam logic [ADDR_WIDTH-1:0] ADDR_KEY_IN  = 4'h0;
    localparam logic [ADDR_WIDTH-1:0] ADDR_CONTROL = 4'h4;
    localparam logic [ADDR_WIDTH-1:0] ADDR_RESULT  = 4'h8;
    localparam logic [ADDR_WIDTH-1:0] ADDR_STATUS  = 4'hC;

    // AXI-Lite write: AW and W channels are independent — latch each, then commit.
    logic bvalid_q;
    logic have_aw, have_w;
    logic wr_en;
    logic [ADDR_WIDTH-1:0] wr_addr_q;
    logic [DATA_WIDTH-1:0] wr_data_q;

    // No new address/data while a write response is outstanding (single outstanding).
    assign s_axi_awready = !have_aw && !bvalid_q;
    assign s_axi_wready  = !have_w && !bvalid_q;

    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            wr_en     <= 1'b0;
            wr_addr_q <= '0;
            wr_data_q <= '0;
            bvalid_q  <= 1'b0;
            have_aw   <= 1'b0;
            have_w    <= 1'b0;
        end else begin
            wr_en <= 1'b0;

            if (bvalid_q && s_axi_bready)
                bvalid_q <= 1'b0;

            if (have_aw && have_w && !bvalid_q) begin
                wr_en     <= 1'b1;
                bvalid_q  <= 1'b1;
                have_aw   <= 1'b0;
                have_w    <= 1'b0;
            end else begin
                if (s_axi_awready && s_axi_awvalid) begin
                    wr_addr_q <= s_axi_awaddr;
                    have_aw   <= 1'b1;
                end
                if (s_axi_wready && s_axi_wvalid) begin
                    wr_data_q <= s_axi_wdata;
                    have_w    <= 1'b1;
                end
            end
        end
    end

    assign s_axi_bvalid = bvalid_q;
    assign s_axi_bresp  = 2'b00;

    // AXI read handshake
    logic rvalid_q;

    logic rd_accept;
    assign rd_accept = s_axi_arvalid && !rvalid_q;

    assign s_axi_arready = !rvalid_q;

    logic [ADDR_WIDTH-1:0] rd_addr_q;
    logic rd_en;

    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            rd_en     <= 1'b0;
            rd_addr_q <= '0;
            rvalid_q  <= 1'b0;
        end else begin
            rd_en <= rd_accept;

            if (rd_accept)
                rd_addr_q <= s_axi_araddr;

            if (rd_accept)
                rvalid_q <= 1'b1;
            else if (rvalid_q && s_axi_rready)
                rvalid_q <= 1'b0;
        end
    end

    assign s_axi_rvalid = rvalid_q;
    assign s_axi_rdata  = rd_mux;
    assign s_axi_rresp  = 2'b00;

    // mmaped regs
    logic [DATA_WIDTH-1:0] key_in_reg;
    logic [DATA_WIDTH-1:0] control_reg;
    logic [DATA_WIDTH-1:0] result_rd;
    logic [DATA_WIDTH-1:0] status_rd;

    // wr decode
    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            key_in_reg  <= '0;
            control_reg <= '0;
        end else if (wr_en) begin
            case (wr_addr_q)
                ADDR_KEY_IN:  key_in_reg  <= wr_data_q;
                ADDR_CONTROL: control_reg <= wr_data_q;
                default: ;
            endcase
        end
    end

    // rd mux
    logic [DATA_WIDTH-1:0] rd_mux;

    always_comb begin
        case (rd_addr_q)
            ADDR_KEY_IN:  rd_mux = key_in_reg;
            ADDR_CONTROL: rd_mux = control_reg;
            ADDR_RESULT:  rd_mux = result_rd;
            ADDR_STATUS:  rd_mux = status_rd;
            default:      rd_mux = '0;
        endcase
    end

    // connection logic
    // 1 cycle start pulse when sw writes CONTROL
    logic ctrl_wr;
    assign ctrl_wr = wr_en && (wr_addr_q == ADDR_CONTROL);

    logic core_start;
    logic core_insert;
    logic core_query;
    logic core_clear;

    assign core_start  = ctrl_wr && wr_data_q[0];
    assign core_insert = ctrl_wr && wr_data_q[1];
    assign core_query  = ctrl_wr && wr_data_q[2];
    assign core_clear  = ctrl_wr && wr_data_q[3];

    // core outputs
    logic core_done;
    logic core_busy;
    logic core_query_result;

    // status/result readback
    assign result_rd = {31'b0, core_query_result};
    assign status_rd = {30'b0, core_busy, core_done};

    // core instance
    bloom_core #(
        .KEY_WIDTH    (KEY_WIDTH),
        .FILTER_WIDTH (FILTER_WIDTH)
    ) u_core (
        .clk          (aclk),
        .rst_n        (aresetn),
        .start        (core_start),
        .insert       (core_insert),
        .query        (core_query),
        .clear        (core_clear),
        .key          (key_in_reg[KEY_WIDTH-1:0]),
        .done         (core_done),
        .busy         (core_busy),
        .query_result (core_query_result)
    );
endmodule
