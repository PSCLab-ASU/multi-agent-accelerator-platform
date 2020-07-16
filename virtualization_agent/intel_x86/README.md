----------------------------------------------------------------------------------------------------------------------------------------------------------------
PICO Controller for MKL Matrix Multiplication
----------------------------------------------------------------------------------------------------------------------------------------------------------------
- Building:
> export MKLROOT=/archive-t2/Design/fpga_computing/wip/depends/mkl/

> gcc -O3 -g utils/src/*.cc virtualization_agent/common/api/src/pico_utils.cc virtualization_agent/common/ctrl/src/pico_ctrl.cc virtualization_agent/common/ctrl/src/pico_ctrl_main.cc -o picoctrl -I../depends/boost/include -Ivirtualization_agent/common/api/include -Iaccelerator_agent/include -Iutils/include -Ivirtualization_agent/common/ctrl/include/ -Ivirtualization_agent/common/include/ -lstdc++ -std=c++2a -lzmq  -DMKL_ILP64 -m64 -I${MKLROOT}/include  -Wl,--start-group ${MKLROOT}/lib/intel64/libmkl_intel_ilp64.a ${MKLROOT}/lib/intel64/libmkl_gnu_thread.a ${MKLROOT}/lib/intel64/libmkl_core.a -Wl,--end-group -lgomp -lpthread -lm -ldl

- Running (sudo mode required):  
> ./picoctrl --action sample --claim "hw_vid=*:hw_pid=*:hw_ss_vid=*:hw_ss_pid=*:sw_vid=*:sw_pid=*:sw_clid=*:sw_fid=0:sw_verid=*" --nargs 5 --input_file stim_file .

--------------------------------------------------------------------------------------------------------------------------------------------------------------
intel_x86 Virtualization Agent
--------------------------------------------------------------------------------------------------------------------------------------------------------------  
- Building:  
> make virtualization_agent

- Running (sudo mode required):   
> ./intel_x86 --accel_port home_nexus --pub_port prouting --pids "1172:2494:198a:3852" --owner moore-1.eng.asu.edu --ext_port 8010
