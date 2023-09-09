set root_dir [pwd]

#############################################
# Check existence of subdirectories
#############################################
if { [ file exists sources ] == 1 } then {
  puts "sources directory found"
} else {
  puts "Error: sources subdirectory not found."
  exit
}

#############################################
# Create project
#############################################
create_project project $root_dir/project -part xc7a35tftg256-1 -force
set_property target_language VHDL [current_project]

add_files $root_dir/sources
update_compile_order -fileset sources_1
update_compile_order -fileset sim_1
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -module_name fifo18x16k -dir $root_dir/ip/
set_property -dict [list \
  CONFIG.Fifo_Implementation {Independent_Clocks_Block_RAM} \
  CONFIG.Input_Data_Width  {18} \
  CONFIG.Input_Depth {16384} \
  CONFIG.Enable_ECC {false} \
  CONFIG.Enable_Safety_Circuit {false} \
  CONFIG.Almost_Full_Flag {false} \
  CONFIG.Almost_Empty_Flag {false} \
  CONFIG.Read_Data_Count {true} \
  CONFIG.Write_Data_Count {true} \
  CONFIG.Use_Embedded_Registers {true} \
] [get_ips fifo18x16k]
generate_target {synthesis instantiation_template} [get_files $root_dir/ip/fifo18x16k.xcix] -force

add_files -norecurse $root_dir/ip/fifo8x1k.xcix
add_files -norecurse $root_dir/ip/fifo9x1k.xcix
add_files -norecurse $root_dir/ip/fifo16x1k.xcix
add_files -norecurse $root_dir/ip/ram16x1024.xcix
add_files -norecurse $root_dir/ip/clk_wiz_0.xcix
add_files -norecurse $root_dir/ip/fifo16x2k.xcix
add_files -norecurse $root_dir/ip/fifo16x8k.xcix
add_files -norecurse $root_dir/ip/FIFO8x2kXilinx.xcix
add_files -norecurse $root_dir/ip/EventFIFO.xcix
add_files -norecurse $root_dir/ip/crcRomXilinx.xcix

export_ip_user_files -of_objects [get_ips] -force -quiet

update_compile_order -fileset sources_1

file mkdir $root_dir/project/project.srcs/constrs_1
add_files -fileset constrs_1 -norecurse $root_dir/constraint/GROWTH-FY2016_Bitstream.xdc
add_files -fileset constrs_1 -norecurse $root_dir/constraint/GROWTH-FY2016_Timing.xdc
add_files -fileset constrs_1 -norecurse $root_dir/constraint/GROWTH_FY2016_Pins.xdc

#############################################
# Build bitstream file
#############################################
launch_runs impl_1 -to_step write_bitstream
wait_on_run impl_1
