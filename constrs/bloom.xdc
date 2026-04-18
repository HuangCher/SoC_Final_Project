# 100 MHz single-ended clock
set_property -dict {PACKAGE_PIN N15 IOSTANDARD LVCMOS33} [get_ports clk_in1_0]
create_clock -period 10.000 -name sys_clk [get_ports clk_in1_0]

# Reset (BTN[0], active-high)
set_property -dict {PACKAGE_PIN J2 IOSTANDARD LVCMOS25} [get_ports reset_rtl_0]

# Bank 0 voltage
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]