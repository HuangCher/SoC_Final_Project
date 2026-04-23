# build.tcl — Recreate the Bloom Accelerator Vivado project from scratch.
#
# Usage:  Open Vivado 2025.1, then in the Tcl console:
#           cd <path-to-this-repo>
#           source build.tcl
#
# Prerequisites:
#   - Vivado 2025.1
#   - This repo checked out with the following layout:
#       src/bloom_core.sv, src/bloom_axi.sv
#       bd/design_1.tcl
#       constrs/bloom.xdc
#       build.tcl  (this file)

set script_dir [file dirname [file normalize [info script]]]
set proj_name  bloom_accel_v
set proj_dir   $script_dir/build/$proj_name
set ip_dir     $script_dir/build/ip_repo/bloom_axi
set part       xc7s50csga324-1

# ============================================================
#  Step 1 — Package bloom_axi IP from RTL sources
# ============================================================
puts "--- Packaging bloom_axi IP ---"

file mkdir $ip_dir

create_project -in_memory -part $part
add_files -norecurse [list \
    $script_dir/src/bloom_core.sv \
    $script_dir/src/bloom_axi.sv  \
]
set_property file_type SystemVerilog [get_files *.sv]
set_property top bloom_axi [current_fileset]
update_compile_order -fileset sources_1

ipx::package_project -root_dir $ip_dir -vendor jfrankel -library user -taxonomy /UserIP -import_files

set core [ipx::current_core]
set_property NAME         bloom_axi                       $core
set_property VERSION      1.0                             $core
set_property DISPLAY_NAME "Bloom Filter AXI Accelerator"  $core
set_property DESCRIPTION  "AXI4-Lite Bloom filter accelerator" $core

# Clock interface with FREQ_HZ
ipx::infer_bus_interface aclk xilinx.com:signal:clock_rtl:1.0 $core
ipx::add_bus_parameter FREQ_HZ [ipx::get_bus_interfaces aclk -of_objects $core]
set_property value 100000000 [ipx::get_bus_parameters FREQ_HZ \
    -of_objects [ipx::get_bus_interfaces aclk -of_objects $core]]

# Reset interface with polarity
ipx::infer_bus_interface aresetn xilinx.com:signal:reset_rtl:1.0 $core
set_property value ACTIVE_LOW [ipx::get_bus_parameters POLARITY \
    -of_objects [ipx::get_bus_interfaces aresetn -of_objects $core]]

ipx::check_integrity $core
ipx::save_core $core
close_project

puts "--- bloom_axi IP packaged to $ip_dir ---"

# ============================================================
#  Step 2 — Create the Vivado project
# ============================================================
puts "--- Creating project ---"

create_project $proj_name $proj_dir -part $part -force
set_property IP_REPO_PATHS $ip_dir [current_project]
update_ip_catalog -rebuild

# ============================================================
#  Step 3 — Build the block design
# ============================================================
puts "--- Building block design ---"

source $script_dir/bd/design_1.tcl

# Exclude bloom_axi from MicroBlaze instruction address space (if mapped there)
catch {
    exclude_bd_addr_seg \
        [get_bd_addr_segs perifs/axi_perifs/bloom_axi_0/s_axi/reg0] \
        -target_address_space [get_bd_addr_spaces microblaze_0/Instruction]
}

validate_bd_design
save_bd_design

# ============================================================
#  Step 4 — Add constraints
# ============================================================
puts "--- Adding constraints ---"

add_files -fileset constrs_1 -norecurse $script_dir/constrs/bloom.xdc

# ============================================================
#  Step 5 — Create HDL wrapper and set as top
# ============================================================
puts "--- Creating HDL wrapper ---"

set bd_file [get_files design_1.bd]
make_wrapper -files $bd_file -top

set wrapper_dir [file join $proj_dir ${proj_name}.gen sources_1 bd design_1 hdl]
add_files -norecurse [file join $wrapper_dir design_1_wrapper.v]

set_property TOP design_1_wrapper [current_fileset]
update_compile_order -fileset sources_1

# ============================================================
#  Done
# ============================================================
puts ""
puts "============================================"
puts "  Project created at: $proj_dir"
puts "  bloom_axi base:     0x44A00000"
puts ""
puts "  Next steps:"
puts "    1. Run Synthesis:     launch_runs synth_1 -jobs 4"
puts "    2. Run Implementation: launch_runs impl_1 -to_step write_bitstream -jobs 4"
puts "    3. Export hardware:   write_hw_platform -fixed -include_bit -force $proj_dir/${proj_name}.xsa"
puts "============================================"
