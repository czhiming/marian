#include <cassert>
#include <sstream>
#include "matrix.h"
#include "kernel.h"

using namespace std;

namespace amunmt {
namespace FPGA {
namespace mblas {

Matrix::Matrix(const cl_context &context, const cl_device_id &device)
:context_(context)
,device_(device)
,rows_(0)
,cols_(0)
{

}

Matrix::Matrix(const cl_context &context, const cl_device_id &device, size_t rows, size_t cols, float val)
:context_(context)
,device_(device)
,rows_(rows)
,cols_(cols)
{

}

Matrix::Matrix(const cl_context &context, const cl_device_id &device, size_t rows, size_t cols, float *val)
:context_(context)
,device_(device)
,rows_(rows)
,cols_(cols)
{
  mem_ = clCreateBuffer(context_,  CL_MEM_COPY_HOST_PTR,  sizeof(float) * rows * cols, val, NULL);
}

void Matrix::Resize(size_t rows, size_t cols, size_t beam, size_t batches)
{
  rows_ = rows;
  cols_ = cols;

  //clReleaseMemObject(mem_);
  mem_ = clCreateBuffer(context_,  CL_MEM_READ_WRITE,  sizeof(float) * rows * cols, NULL, NULL);

}

float Matrix::Sum() const
{
  int err;
  size_t global;                      // global domain size for our calculation
  size_t local;                       // local domain size for our calculation

  cl_mem output = clCreateBuffer(context_, CL_MEM_WRITE_ONLY, sizeof(float), NULL, &err);
  CheckError(err);
  assert(output);

  // create kernel
  cl_command_queue commands = CreateCommandQueue(context_, device_);
  cl_kernel kernel = CreateKernel("kernels/sum.cl", "sum", context_, device_);

  // Set the arguments to our compute kernel
  unsigned int count = size();

  CheckError( clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_) );
  CheckError( clSetKernelArg(kernel, 1, sizeof(cl_mem), &output) );
  CheckError( clSetKernelArg(kernel, 2, sizeof(unsigned int), &count) );

  // Get the maximum work group size for executing the kernel on the device
  //
  CheckError( clGetKernelWorkGroupInfo(kernel, device_, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL) );

  global = 1024;

  cerr << "local=" << local << endl;
  cerr << "global=" << global << endl;

  CheckError( clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL) );

  // Wait for the command commands to get serviced before reading back results
  //
  CheckError( clFinish(commands) );

}

std::string Matrix::Debug(bool detailed) const
{
  std::stringstream strm;
  strm << BaseMatrix::Debug(detailed) << " " << mem_;

  if (detailed) {
    float sum = Sum();
    strm << " size=" << size() << " sum=" << sum << std::flush;
  }

  return strm.str();
}

}
}
}
