#include "device_manager.h"
#include "payloads.h"
#include <type_traits>
#include<iostream>
#include <tuple>
#include "CL/opencl.h"
#include "aocl_utils.h"
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string.hpp>

using namespace aocl_utils;

device_manager::device_manager()
{
  
  auto _add_kernel = [&](std::string desc, auto func)
  {
    std::string intel_desc = std::string("hw_vid=8086:hw_pid=2f80:") +
                             std::string("hw_ss_vid=8086:hw_ss_pid=2f80:") +
                             std::string("sw_vid=*:sw_pid=*:") +
                             std::string("sw_fid=[INSERT_FUNC_ID]:sw_clid=*:sw_verid=*");
    //substite template
    intel_desc = boost::replace_first_copy(intel_desc, "[INSERT_FUNC_ID]", desc);
    //add to manifest
    this->add_kernel_desc( intel_desc );
    //add execution unit
    this->add_execution_method( lookup_type(intel_desc), this, func);
  };

  add_execution_method( lookup_type("sw_fid=default"), this, &device_manager::_default );

  //device manager should check if hw exists
  //and add the * fid if hardware exists
  _add_kernel( "*",       &device_manager::_default    ); 
  _add_kernel( "56789",   &device_manager::_matrix_mult);
  _add_kernel( "00000",   &device_manager::_hello_world);
}

device_manager::return_type device_manager::_matrix_mult ( header_type hdr, input_type input)
{
  std::cout << "---dm->matrix_mult..." << std::endl;
    
  scoped_array<cl_device_id> device;

  auto& in_payload = input.second.get(); 
  unsigned num_in_args = in_payload.get_size();
  //std::cout<< "Num of input argument: "<< num_in_args << std::endl;
  std::vector<unsigned> in_size;
  std::vector<unsigned> in_type_size;
  std::vector<unsigned> out_size;
  std::vector<unsigned> out_type_size;
  
  //hardcoded output information. Can be read from AOCX file. 
  unsigned num_out_args = 1;
  out_size.push_back((unsigned)(64*64*sizeof(float)));
  out_type_size.push_back((unsigned)sizeof(float));

  std::vector<arg_element_t> payload_data;

  for(int i=0; i<num_in_args; i++){
    payload_data.push_back(in_payload.pop_arg());
  }

  for(int i=num_in_args-1; i>=0; i--){
     auto[header, data] = payload_data[i];
     auto[sign_t, data_t, t_size, size] = header;

     //prepare input data size
     in_size.push_back((unsigned)(size*t_size));
     in_type_size.push_back((unsigned)t_size);
  }

  // Hard-coded input and output kernel arguments indentifier: Will be read from AOCX file later.
  // 0 indicates a memory object to be enqueued to a global buffer. 1 indicates other types.  
  std::vector<bool> kernel_in_arg_type = {0,0,1,1};
  std::vector<bool> kernel_out_arg_type = {0};
  
  //Determine kernel argument sizes based on the argument types
  std::vector<size_t> kernel_in_arg_size;
  std::vector<size_t> kernel_out_arg_size;

  for(int i=0; i<num_in_args; i++){
    size_t size = kernel_in_arg_type[i]? in_type_size[i] : sizeof(cl_mem);
    kernel_in_arg_size.push_back(size);
  }

   for(int i=0; i<num_out_args; i++){
    size_t size = kernel_out_arg_type[i]? out_type_size[i] : sizeof(cl_mem);
    kernel_out_arg_size.push_back(size);
  }

  //hardcoded global and local work size. Will be decided later how to parse them.
  size_t global_work_size[2] = {64,64};
  size_t local_work_size[2] = {64,64};
  
   
  //Initializes the OpenCL objects.
  unsigned num_devices = 0;
  cl_int status;

  printf("Initializing OpenCL\n");

  if(!setCwdToExeDir()) {printf("Incorrect working directory\n");}

  //Get the OpenCL platform.
  cl_platform_id platform = findPlatform("Intel(R) FPGA SDK for OpenCL(TM)");
  if(platform == NULL) 
  {
    printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.\n");
  }

  //Query the available OpenCL device.
  device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  printf("Using %d device(s)\n", num_devices);
  for(int i = 0; i < num_devices; ++i) 
  {
    printf("  %s\n", getDeviceName(device[i]).c_str());
  }

  if(num_devices == 0) 
  {
    checkError(-1, "No devices");
  }

  //Create the context.
  cl_context context = clCreateContext(NULL, num_devices, device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");

  const char *kernel_name = "matrix_mult";

  std::string binary_file = getBoardBinaryFile(kernel_name, device[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  cl_program program = createProgramFromBinary(context, binary_file.c_str(), device, num_devices);
  printf("Binary created successfully\n");
  
  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  cl_command_queue queue = clCreateCommandQueue(context, device[0], CL_QUEUE_PROFILING_ENABLE, &status);
  checkError(status, "Failed to create command queue");
  printf("Command queue created successfully\n");

  // Create Kernel.
  cl_kernel kernel = clCreateKernel(program, kernel_name, &status);
  checkError(status, "Failed to create kernel");

  printf("Kernel created successfully\n");
  cl_mem* in_buf = new cl_mem [num_in_args];
  cl_mem* out_buf = new cl_mem [num_out_args];

  for(int i=0; i<num_in_args; i++) 
  {
    in_buf[i] = clCreateBuffer(context, CL_MEM_READ_ONLY, in_size[i], NULL, &status);
    checkError(status, "Failed to create buffer for input");
  }
  
  for(int i=0; i<num_out_args; i++) 
  {
    out_buf[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY, out_size[i], NULL, &status);
    checkError(status, "Failed to create buffer for output");
  }

  printf("Running the kernel\n");
  
  //Index for Payload data
  unsigned index = num_in_args-1;

  for(int i = 0; i <num_in_args; i++) 
  {
    auto[header, data] = payload_data[index--];
    auto[sign_t, data_t, t_size, size] = header;
    status = clEnqueueWriteBuffer(queue, in_buf[i], CL_FALSE, 0, in_size[i], data, 0, NULL, NULL);
    checkError(status, "Failed to transfer input");
  }

  //Launch kernels.
  cl_event kernel_event;

  //Counter for kernel argument index
  unsigned argi = 0;

  for(int i = 0; i < num_out_args; i++) 
  {
    status = clSetKernelArg(kernel, argi++, kernel_out_arg_size[i], &out_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);
  }

  for(int i = 0; i < num_in_args; i++) 
  {
    status = clSetKernelArg(kernel, argi++, kernel_in_arg_size[i], &in_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);
  }

  //Enqueue kernel.
  status = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &kernel_event);
  checkError(status, "Failed to launch kernel");

  clFinish(queue);


  //Prepare the output to send results back
  return_type output;
  recv_payload rp;

  void * results = nullptr;
  bool valid_type = true;

  rp.set_tid( input.first );
  rp.set_nargs(num_out_args); 

  //Getting output header information from input. Output header should be generated separately
  auto[header, data] = payload_data[2];
  results = rp.allocate_back( header ).get();

  for(int i = 0; i < num_out_args; i++) 
  {
    void* out = alignedMalloc(out_size[i]);
    printf("Reading Output Data from Kernel\n");
    status = clEnqueueReadBuffer(queue, out_buf[i], CL_TRUE, 0, out_size[i], out, 0, NULL, NULL);
    auto fout = (unsigned char*) out;
    results = out;
    printf("Partial Output from FPGA: \n");
    for(int j = 0; j < 100; ++j){
      printf("%02x", fout[j]);
    }
    printf("\n");
    checkError(status, "Failed to read output matrix");
    alignedFree(out);
  }

  //add to the output
  output.push_back( std::move(rp) );

  //Release the resources allocated during OpenCL initialization
	if(kernel)  {clReleaseKernel(kernel);}
	if(queue)   {clReleaseCommandQueue(queue);}
  if(program) {clReleaseProgram(program);}
  if(context) {clReleaseContext(context);}
  delete[] in_buf;
  delete[] out_buf;


  return output;
}

device_manager::return_type device_manager::_hello_world ( header_type hdr, input_type input)
{
  std::cout << "---dm->hello world..." << std::endl;
  return_type output;
  recv_payload rp;
  cl_platform_id platform;
  unsigned num_devices;
  scoped_array<cl_device_id> device;
  cl_context context;
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;

  // Initializes the OpenCL objects.
  platform = NULL;
  num_devices = 0;
  context = NULL;
  program = NULL;
  cl_int status;

  printf("Initializing OpenCL\n");

  if(!setCwdToExeDir()) {printf("Incorrect working directory\n");}

  //Get the OpenCL platform.
  platform = findPlatform("Intel(R) FPGA SDK for OpenCL(TM)");
  if(platform == NULL) {
    printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.\n");
  }

  //Query the available OpenCL device.
  device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  printf("Using %d device(s)\n", num_devices);
  for(unsigned i = 0; i < num_devices; ++i) {
    printf("  %s\n", getDeviceName(device[i]).c_str());
  }

  if(num_devices == 0) {
    checkError(-1, "No devices");
  }

  //Create the context.
  context = clCreateContext(NULL, num_devices, device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");

  const char *kernel_name = "hello_world_emu";

  std::string binary_file = getBoardBinaryFile(kernel_name, device[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), device, num_devices);

  //Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");


	queue = clCreateCommandQueue(context, device[0], CL_QUEUE_PROFILING_ENABLE, &status);
  checkError(status, "Failed to create command queue");

  //Create Kernel.
  kernel = clCreateKernel(program, kernel_name, &status);
  checkError(status, "Failed to create kernel");

  rp.set_tid( input.first );
  rp.set_nargs( 1 ); 

  //add to the output
  output.push_back( std::move( rp ) );
  return output;
}

device_manager::return_type device_manager::_default ( header_type hdr, input_type input)
{
  std::cout << "---dm->default..." << std::endl; 
  return_type output;

  return output;
}
