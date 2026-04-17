`timescale 1ns/1ps

module bloom_axi_tb;

    logic           aclk, aresetn;
    logic   [3:0]   s_axi_awaddr;
    logic   [2:0]   s_axi_awprot;
    logic           s_axi_awvalid;
    logic           s_axi_awready;
    logic   [31:0]  s_axi_wdata;
    logic   [3:0]   s_axi_wstrb;
    logic           s_axi_wvalid;
    logic           s_axi_wready;
    logic   [1:0]   s_axi_bresp;
    logic           s_axi_bvalid;
    logic           s_axi_bready;
    logic   [3:0]   s_axi_araddr;
    logic   [2:0]   s_axi_arprot;
    logic           s_axi_arvalid;
    logic           s_axi_arready;
    logic   [31:0]  s_axi_rdata;
    logic   [1:0]   s_axi_rresp;
    logic           s_axi_rvalid;
    logic           s_axi_rready;

    bloom_axi dut (.*);

    initial aclk = 0;
    always #5 aclk = ~aclk;

    int pass_count = 0;
    int fail_count = 0;

    localparam logic [3:0]  ADDR_KEY_IN  = 4'h0,
                            ADDR_CONTROL = 4'h4,
                            ADDR_RESULT  = 4'h8,
                            ADDR_STATUS  = 4'hC;

    localparam logic [31:0] CMD_INSERT = 32'h3,
                            CMD_QUERY  = 32'h5,
                            CMD_CLEAR  = 32'h9;


    task automatic do_reset();
        aresetn       = 0;
        s_axi_awaddr  = '0; s_axi_awprot = '0; s_axi_awvalid = 0;
        s_axi_wdata   = '0; s_axi_wstrb  = '0; s_axi_wvalid  = 0;
        s_axi_bready  = 0;
        s_axi_araddr  = '0; s_axi_arprot = '0; s_axi_arvalid = 0;
        s_axi_rready  = 0;
        repeat (3) @(posedge aclk);
        aresetn = 1;
        @(posedge aclk);
    endtask

    task automatic axi_write(input logic [3:0] addr, input logic [31:0] data);
        @(posedge aclk);
        s_axi_awaddr  = addr;
        s_axi_wdata   = data;
        s_axi_wstrb   = 4'hF;
        s_axi_awvalid = 1'b1;
        s_axi_wvalid  = 1'b1;
        @(posedge aclk);
        s_axi_awvalid = 1'b0;
        s_axi_wvalid  = 1'b0;
        while (!s_axi_bvalid) @(posedge aclk);
        s_axi_bready  = 1'b1;
        @(posedge aclk);
        s_axi_bready  = 1'b0;
    endtask

    task automatic axi_read(input logic [3:0] addr, output logic [31:0] data);
        @(posedge aclk);
        s_axi_araddr  = addr;
        s_axi_arvalid = 1'b1;
        @(posedge aclk);
        s_axi_arvalid = 1'b0;
        while (!s_axi_rvalid) @(posedge aclk);
        data = s_axi_rdata;
        s_axi_rready  = 1'b1;
        @(posedge aclk);
        s_axi_rready  = 1'b0;
    endtask

    task automatic poll_done();
        logic [31:0] status;
        int timeout = 100;
        do begin
            axi_read(ADDR_STATUS, status);
            timeout--;
            if (timeout == 0) begin
                $display("[TIMEOUT] poll_done exceeded 100 reads");
                $finish;
            end
        end while (!status[0]);
    endtask

    task automatic check(input string label, input logic [31:0] actual, expected);
        if (actual === expected) begin
            $display("[PASS] %s", label);
            pass_count++;
        end else begin
            $display("[FAIL] %s  expected=0x%08h actual=0x%08h", label, expected, actual);
            fail_count++;
        end
    endtask

    logic [31:0] rd_data;

    initial begin
        do_reset();

        axi_write(ADDR_KEY_IN, 32'h1234_5678);
        axi_read(ADDR_KEY_IN, rd_data);
        check("T1: KEY_IN readback", rd_data, 32'h1234_5678);

        axi_write(ADDR_CONTROL, 32'h0000_0005);
        axi_read(ADDR_CONTROL, rd_data);
        check("T2: CONTROL readback", rd_data, 32'h0000_0005);

        axi_write(ADDR_KEY_IN, 32'hCAFE_0001);
        axi_write(ADDR_CONTROL, CMD_INSERT);
        poll_done();
        axi_read(ADDR_STATUS, rd_data);
        check("T3: INSERT done", rd_data, 32'h0000_0001);

        axi_write(ADDR_KEY_IN, 32'hCAFE_0001);
        axi_write(ADDR_CONTROL, CMD_QUERY);
        poll_done();
        axi_read(ADDR_RESULT, rd_data);
        check("T4: QUERY inserted key, result=1", rd_data, 32'h0000_0001);

        axi_write(ADDR_CONTROL, CMD_CLEAR);
        poll_done();
        axi_write(ADDR_KEY_IN, 32'hCAFE_0001);
        axi_write(ADDR_CONTROL, CMD_QUERY);
        poll_done();
        axi_read(ADDR_RESULT, rd_data);
        check("T5: QUERY after CLEAR, result=0", rd_data, 32'h0000_0000);

        $display("\n%0d passed, %0d failed", pass_count, fail_count);
        if (fail_count == 0) $display("ALL TESTS PASSED");
        else                 $display("SOME TESTS FAILED");
        $finish;
    end

endmodule
