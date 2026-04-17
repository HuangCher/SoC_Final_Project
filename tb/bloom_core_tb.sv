`timescale 1ns/1ps

module bloom_core_tb;

    logic           clk, rst_n;
    logic           start, insert, query, clear;
    logic   [31:0]  key;
    logic           done, busy, query_result;

    bloom_core dut (.*);

    initial clk = 0;
    always #5 clk = ~clk;

    int pass_count = 0;
    int fail_count = 0;

    task automatic do_reset();
        rst_n = 0;
        start = 0; insert = 0; query = 0; clear = 0; key = '0;
        repeat (3) @(posedge clk);
        rst_n = 1;
        @(posedge clk);
    endtask

    task automatic send_cmd(input logic do_insert, do_query, do_clear,
                            input logic [31:0] cmd_key);
        @(posedge clk);
        start  = 1;
        insert = do_insert;
        query  = do_query;
        clear  = do_clear;
        key    = cmd_key;
        @(posedge clk);
        start  = 0;
        insert = 0;
        query  = 0;
        clear  = 0;
        wait (done);
        @(posedge clk);
    endtask

    task automatic check(input string label, input logic actual, expected);
        if (actual === expected) begin
            $display("[PASS] %s", label);
            pass_count++;
        end else begin
            $display("[FAIL] %s  expected=%0b actual=%0b", label, expected, actual);
            fail_count++;
        end
    endtask

    initial begin
        do_reset();
        send_cmd(0, 1, 0, 32'hDEAD_BEEF);
        check("T1: query absent key after reset", query_result, 1'b0);

        send_cmd(1, 0, 0, 32'hCAFE_0001);
        send_cmd(0, 1, 0, 32'hCAFE_0001);
        check("T2: query key that was inserted", query_result, 1'b1);

        send_cmd(0, 0, 1, 32'h0);
        send_cmd(0, 1, 0, 32'hCAFE_0001);
        check("T3: query after clear returns absent", query_result, 1'b0);

        send_cmd(1, 0, 0, 32'hAAAA_0001);
        send_cmd(1, 0, 0, 32'hBBBB_0002);
        send_cmd(1, 0, 0, 32'hCCCC_0003);

        send_cmd(0, 1, 0, 32'hBBBB_0002);
        check("T4a: query inserted key", query_result, 1'b1);

        send_cmd(0, 1, 0, 32'hFFFF_FFFF);
        check("T4b: query non-inserted key", query_result, 1'b0);

        send_cmd(0, 0, 1, 32'h0);
        send_cmd(1, 0, 0, 32'h0000_0042);
        send_cmd(1, 0, 0, 32'h0000_0042);
        send_cmd(0, 1, 0, 32'h0000_0042);
        check("T5: query after double insert", query_result, 1'b1);

        $display("\n%0d passed, %0d failed", pass_count, fail_count);
        if (fail_count == 0) $display("ALL TESTS PASSED");
        else                 $display("SOME TESTS FAILED");
        $finish;
    end

endmodule
