----------------------------------------------------------------------------------------------------------------------------------------------------------------
PICO Controller for Matrix Multiplication
----------------------------------------------------------------------------------------------------------------------------------------------------------------
Building:    
gcc -O3 -g utils/src/*.cc virtualization_agent/common/api/src/pico_utils.cc virtualization_agent/common/ctrl/src/pico_ctrl_matrix_mult.cc virtualization_agent/common/ctrl/src/pico_ctrl_main.cc -o picoctrl_matrix_mult -I../depends/boost/include -Ivirtualization_agent/common/api/include -Iaccelerator_agent/include -Iutils/include -Ivirtualization_agent/common/ctrl/include/ -Ivirtualization_agent/common/include/ -lstdc++ -std=c++2a -lzmq


Running (sudo mode required):  
./picoctrl_matrix_mult --action sample --claim "hw_vid=*:hw_pid=*:hw_ss_vid=*:hw_ss_pid=*:sw_vid=*:sw_pid=*:sw_clid=*:sw_fid=56789:sw_verid=*" --nargs 4 --input_file stim_file .

----------------------------------------------------------------------------------------------------------------------------------------------------------------
PICO Controller for Hello World
----------------------------------------------------------------------------------------------------------------------------------------------------------------

Building:  
gcc -O3 -g utils/src/*.cc virtualization_agent/common/api/src/pico_utils.cc virtualization_agent/common/ctrl/src/pico_ctrl_hello_world.cc virtualization_agent/common/ctrl/src/pico_ctrl_main.cc -o picoctrl_hello_world -I../depends/boost/include -Ivirtualization_agent/common/api/include -Iaccelerator_agent/include -Iutils/include -Ivirtualization_agent/common/ctrl/include/ -Ivirtualization_agent/common/include/ -lstdc++ -std=c++2a -lzmq


Running (sudo mode required):    
./picoctrl_hello_world --action sample --claim "hw_vid=*:hw_pid=*:hw_ss_vid=*:hw_ss_pid=*:sw_vid=*:sw_pid=*:sw_clid=*:sw_fid=00000:sw_verid=*" --nargs 1 --input_file stim_file .

--------------------------------------------------------------------------------------------------------------------------------------------------------------
intel_fpga_opencl Virtualization Agent
--------------------------------------------------------------------------------------------------------------------------------------------------------------  
Building:  
make virtualization_agent

Running:   
change to the following directory: multi-agent-accelerator-platform/virtualization_agent/intel_fpga_opencl/debug/bin  
copy compiled kernels to this directory from the following shared drive location: /archive-t2/user/mquraish/kernels/bin/  
switch to super user: sudo su  
source /home/tools/altera/setup_scripts/Intel_Setup_19.1-pro_nalla_385a.csh  


Emulation:  
env CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ./intel_fpga_opencl --accel_port home_nexus --pub_port prouting --pids "1172:2494:198a:3852" --owner moore-1.eng.asu.edu --ext_port 8010

FPGA:  
./intel_fpga_opencl --accel_port home_nexus --pub_port prouting --pids "1172:2494:198a:3852" --owner moore-1.eng.asu.edu --ext_port 8010  
