------------------------------------------------------------------------------------------------------------------------------------------------------------------------
New Nexus
------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Building:  
gcc -O3 -g -fconcepts-ts -Wno-return-type utils/json.cc utils/utils.cc utils/zmsg_builder.cc utils/zmsg_viewer.cc utils/config_components.cc pico_services/common/pico_utils.cc xcelerate/accel_utils.cc nexus/nexus_utils.cc nexus/claim_subsys_base.cc nexus/nexus_perfmon.cc nexus/rank_perfmon.cc nexus/rank_arbiter.cc nexus/nex_predicates.cc nexus/accel_def_lut.cc nexus/hw_nex_cache.cc nexus/job_interface.cc nexus/job_details.cc nexus/pico_helper.cc nexus/alloc_tracker.cc nexus/hw_tracker.cc nexus/res_mgr_frontend.cc nexus/nexus_hub.cc -o hw_nexus -Ixcelerate -Ipico_services/common -Inexus -I../depends/boost/include -Iutils -lstdc++ -std=c++2a -lzmq 2>&1 | tee compiler.out

Running:  
./hw_nexus --internal_port home_nexus --external_port 8000 --publisher_port prouting --repo "/archive-t2/Design/fpga_computing/wip/repo/"

----------------------------------------------------------------------------------------------------------------------------------------------------------------
New OpenCL service
----------------------------------------------------------------------------------------------------------------------------------------------------------------
pico_intel_fpga:  

gcc -O0 -gdwarf-2 -fconcepts-ts -Wno-return-type -Wno-pointer-arith pico_services/common/*.cc pico_services/intel_fpga_bl/*.cc nexus/nexus_utils.cc utils/json.cc utils/json_viewer.cc utils/zmsg_builder.cc utils/zmsg_viewer.cc utils/config_components.cc utils/utils.cc -o opencl_drte -Iutils -I./nexus -Ipico_services/common -Ipico_services/intel_fpga_bl -I../depends/boost/include -I./common/inc/ -I$ALTERAOCLSDKROOT/host/include -lstdc++ -std=c++17 -l:/usr/local/lib/libzmq.so -lOpenCL -L/usr/local/lib -L../depends/boost/lib/ -lboost_regex -lboost_iostreams 2>&1 | tee output
pico_x86

Building:    
gcc -O0 -gdwarf-2 -fconcepts-ts -Wno-return-type -Wno-pointer-arith pico_services/common/*.cc pico_services/intel_x86/*.cc nexus/nexus_utils.cc utils/json.cc utils/json_viewer.cc utils/zmsg_builder.cc utils/zmsg_viewer.cc utils/resource_logic.cc utils/config_components.cc utils/utils.cc -o pico_x86 -Iutils -I./nexus -Ipico_services/common -Ipico_services/intel_x86 -I../depends/boost/include -I./common/inc/ -I$ALTERAOCLSDKROOT/host/include -lstdc++ -std=c++17 -l:/usr/local/lib/libzmq.so -lOpenCL -L/usr/local/lib -L../depends/boost/lib/ -lboost_regex -lboost_iostreams 2>&1 | tee output

Running:  
../pico_x86 --accel_port home_nexus --pub_port prouting --pids "1172:2494:198a:3852" --owner moore-1.eng.asu.edu --ext_port 8010

----------------------------------------------------------------------------------------------------------------------------------------------------------------
New Nexusctl
----------------------------------------------------------------------------------------------------------------------------------------------------------------

Building:  
gcc -O3 -g utils/json.cc utils/zmsg_builder.cc utils/zmsg_viewer.cc utils/json_viewer.cc nexus/nexusctl/nexus.cc utils/utils.cc nexus/nexusctl/nexusctl.cc -o nexusctl -I../depends/boost/include -Inexus -Iutils -Inexus/nexusctl -lstdc++ -std=c++17 -lzmq
(old) gcc -O3 -g nexus.cc ../utils.cc nexusctl.cc -o nexusctl -I../../ -I../ -I. -lstdc++ -std=c++17 -lzmq  

Running:    
sudo -E env LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./nexusctl --external_port 8000 --action rollcall --hosts moore-1.eng.asu.edu,moore-2.eng.asu.edu  
sudo -E env LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./nexusctl --external_port 8000 --action rollcall --hosts moore-2.eng.asu.edu.    
sudo -E env LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./nexusctl --external_port 8000 --action rollcall --hosts moore-1.eng.asu.edu   

--------------------------------------------------------------------------------------------------------------------------------------------------------------
New Xcelerate API
--------------------------------------------------------------------------------------------------------------------------------------------------------------  
Building:  
from home dir:  
g++ -O3 nexus/nexus_utils.cc client_interface/client_utils.cc client_interface/mpi_computeobj.cc utils/* xcelerate/* -o Xcelerate -I../depends/boost/include -Iutils -Ixcelerate -Iclient_interface -Inexus -L../depends/boost/lib/ -lmpich -lstdc++ -std=c++2a -lpthread -lzmq -lboost_system -lboost_thread 2>&1 | tee output  
  
-------------------------------------------------------------------------------------------------------------------------------------------------------------
MPI extend driver
-------------------------------------------------------------------------------------------------------------------------------------------------------------
(Note: removed the -shared flag to get error details)
Building:    
from home dir:   
g++ -O3 -shared ./utils/utils.cc ./utils/zmsg_builder.cc ./utils/zmsg_viewer.cc ./utils/config_components.cc ./utils/json.cc ./client_interface/* ./xcelerate/accel_utils.cc -Iutils/ -Iclient_interface/ -Ixcelerate/ -I../depends/boost/include -lstdc++ -std=c++2a -lpthread -lzmq -lmpich -o libmpi_ext.so -fPIC -fvisibility=hidden 2>&1 | tee output
export LD_LIBRARY_PATH=/archive-t2/Design/fpga_computing/wip/nexus-platform/unit_test/:$LD_LIBRARY_PATH  

Checking symbols:  
nm --demangle --dynamic --defined-only --extern-only libmpi_ext.so | grep MPI_*  

--------------------------------------------------------------------------------------------------------------------------------------------------------------
Application (Hello World)
---------------------------------------------------------------------------------------------------------------------------------------------------------------
Building:    
gcc -O3 -g ../client_interface/mpi_computeobj.cc ../utils/runtime_ctx.cc hello_world.cc -o hello_world -I. -I../utils/ -I../client_interface/ -lstdc++ -std=c++11 -lzmq -lpthread -lmpich -l:libmpi_ext.so  

Running:  
../Xcelerate mpiexec -np 1 ./hello_world -- --accel_async=true --accel_host_file=/archive-t2/Design/fpga_computing/wip/nexus-platform/unit_test/hostfile  

----------------------------------------------------------------------------------------------------------------------------------------------------------------
New picoctrl
----------------------------------------------------------------------------------------------------------------------------------------------------------------
Building:  
gcc -O3 -g  utils/*.cc pico_services/common/pico_utils.cc pico_services/common/picoctl/pico_ctrl.cc pico_services/common/picoctl/pico_ctrl_main.cc -o picoctrl -I../depends/boost/include -Inexus -Iutils -Ipico_services/common/picoctl/ -Ipico_services/common/ -lstdc++ -std=c++2a -lzmq  

Running: ./picoctrl --action sample --claim "hw_vid=*:hw_pid=*:hw_ss_vid=*:hw_ss_pid=*:sw_vid=*:sw_pid=*:sw_clid=*:sw_fid=12345:sw_verid=*" --nargs 1 --input_file stim_file .
