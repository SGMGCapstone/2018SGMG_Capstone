#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <conio.h>
#include "image_thread.hpp"
#include "header.hpp"
#include <CL/cl.h>
#include <opencv2/opencv.hpp>

using namespace std;

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

static const int HEIGHT = 1024;
// 0.25 = 1.0f / 4.0f
static const int WIDTH = (int)((float)HEIGHT * 3.84f * 0.25f);
static const int SCANLINE_NUM = 128;
static const int SAMPLE_NUM = 1024;
static const char* CL_GL_SHARING_EXT = "cl_khr_gl_sharing";
static float TGC_CURVE[1024];

#define OPENCL_C_PROG_BEP_FILE_NAME "programs/backend_processing.cl"
#define OPENCL_C_PROG_DSC_FILE_NAME "programs/digital_scanconversion.cl"
#define KERNEL_BEP_NAME "backend_processing"
#define KERNEL_DSC_NAME "digital_scanconversion"

typedef struct _OPENCL_C_PROG_SRC {
	size_t length;
	char *string;
} OPENCL_C_PROG_SRC;

static cl_platform_id platform;
static cl_device_id device;
static cl_context context;
static cl_command_queue cmd_queue;
cl_program program[2];
cl_kernel kernel[2];

cl_mem buffer_iq_input;
cl_mem buffer_tgc_curve;
cl_mem buffer_bep_result;
cl_mem buffer_dsc_result;

/******************************************************************************************************/
char *get_error_flag(cl_int errcode) {
	switch (errcode) {
	case CL_SUCCESS:
		return (char*) "CL_SUCCESS";
	case CL_DEVICE_NOT_FOUND:
		return (char*) "CL_DEVICE_NOT_FOUND";
	case CL_DEVICE_NOT_AVAILABLE:
		return (char*) "CL_DEVICE_NOT_AVAILABLE";
	case CL_COMPILER_NOT_AVAILABLE:
		return (char*) "CL_COMPILER_NOT_AVAILABLE";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:
		return (char*) "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case CL_OUT_OF_RESOURCES:
		return (char*) "CL_OUT_OF_RESOURCES";
	case CL_OUT_OF_HOST_MEMORY:
		return (char*) "CL_OUT_OF_HOST_MEMORY";
	case CL_PROFILING_INFO_NOT_AVAILABLE:
		return (char*) "CL_PROFILING_INFO_NOT_AVAILABLE";
	case CL_MEM_COPY_OVERLAP:
		return (char*) "CL_MEM_COPY_OVERLAP";
	case CL_IMAGE_FORMAT_MISMATCH:
		return (char*) "CL_IMAGE_FORMAT_MISMATCH";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:
		return (char*) "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case CL_BUILD_PROGRAM_FAILURE:
		return (char*) "CL_BUILD_PROGRAM_FAILURE";
	case CL_MAP_FAILURE:
		return (char*) "CL_MAP_FAILURE";
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:
		return (char*) "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
		return (char*) "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case CL_COMPILE_PROGRAM_FAILURE:
		return (char*) "CL_COMPILE_PROGRAM_FAILURE";
	case CL_LINKER_NOT_AVAILABLE:
		return (char*) "CL_LINKER_NOT_AVAILABLE";
	case CL_LINK_PROGRAM_FAILURE:
		return (char*) "CL_LINK_PROGRAM_FAILURE";
	case CL_DEVICE_PARTITION_FAILED:
		return (char*) "CL_DEVICE_PARTITION_FAILED";
	case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
		return (char*) "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
	case CL_INVALID_VALUE:
		return (char*) "CL_INVALID_VALUE";
	case CL_INVALID_DEVICE_TYPE:
		return (char*) "CL_INVALID_DEVICE_TYPE";
	case CL_INVALID_PLATFORM:
		return (char*) "CL_INVALID_PLATFORM";
	case CL_INVALID_DEVICE:
		return (char*) "CL_INVALID_DEVICE";
	case CL_INVALID_CONTEXT:
		return (char*) "CL_INVALID_CONTEXT";
	case CL_INVALID_QUEUE_PROPERTIES:
		return (char*) "CL_INVALID_QUEUE_PROPERTIES";
	case CL_INVALID_COMMAND_QUEUE:
		return (char*) "CL_INVALID_COMMAND_QUEUE";
	case CL_INVALID_HOST_PTR:
		return (char*) "CL_INVALID_HOST_PTR";
	case CL_INVALID_MEM_OBJECT:
		return (char*) "CL_INVALID_MEM_OBJECT";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
		return (char*) "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case CL_INVALID_IMAGE_SIZE:
		return (char*) "CL_INVALID_IMAGE_SIZE";
	case CL_INVALID_SAMPLER:
		return (char*) "CL_INVALID_SAMPLER";
	case CL_INVALID_BINARY:
		return (char*) "CL_INVALID_BINARY";
	case CL_INVALID_BUILD_OPTIONS:
		return (char*) "CL_INVALID_BUILD_OPTIONS";
	case CL_INVALID_PROGRAM:
		return (char*) "CL_INVALID_PROGRAM";
	case CL_INVALID_PROGRAM_EXECUTABLE:
		return (char*) "CL_INVALID_PROGRAM_EXECUTABLE";
	case CL_INVALID_KERNEL_NAME:
		return (char*) "CL_INVALID_KERNEL_NAME";
	case CL_INVALID_KERNEL_DEFINITION:
		return (char*) "CL_INVALID_KERNEL_DEFINITION";
	case CL_INVALID_KERNEL:
		return (char*) "CL_INVALID_KERNEL";
	case CL_INVALID_ARG_INDEX:
		return (char*) "CL_INVALID_ARG_INDEX";
	case CL_INVALID_ARG_VALUE:
		return (char*) "CL_INVALID_ARG_VALUE";
	case CL_INVALID_ARG_SIZE:
		return (char*) "CL_INVALID_ARG_SIZE";
	case CL_INVALID_KERNEL_ARGS:
		return (char*) "CL_INVALID_KERNEL_ARGS";
	case CL_INVALID_WORK_DIMENSION:
		return (char*) "CL_INVALID_WORK_DIMENSION";
	case CL_INVALID_WORK_GROUP_SIZE:
		return (char*) "CL_INVALID_WORK_GROUP_SIZE";
	case CL_INVALID_WORK_ITEM_SIZE:
		return (char*) "CL_INVALID_WORK_ITEM_SIZE";
	case CL_INVALID_GLOBAL_OFFSET:
		return (char*) "CL_INVALID_GLOBAL_OFFSET";
	case CL_INVALID_EVENT_WAIT_LIST:
		return (char*) "CL_INVALID_EVENT_WAIT_LIST";
	case CL_INVALID_EVENT:
		return (char*) "CL_INVALID_EVENT";
	case CL_INVALID_OPERATION:
		return (char*) "CL_INVALID_OPERATION";
	case CL_INVALID_GL_OBJECT:
		return (char*) "CL_INVALID_GL_OBJECT";
	case CL_INVALID_BUFFER_SIZE:
		return (char*) "CL_INVALID_BUFFER_SIZE";
	case CL_INVALID_MIP_LEVEL:
		return (char*) "CL_INVALID_MIP_LEVEL";
	case CL_INVALID_GLOBAL_WORK_SIZE:
		return (char*) "CL_INVALID_GLOBAL_WORK_SIZE";
	case CL_INVALID_PROPERTY:
		return (char*) "CL_INVALID_PROPERTY";
	case CL_INVALID_IMAGE_DESCRIPTOR:
		return (char*) "CL_INVALID_IMAGE_DESCRIPTOR";
	case CL_INVALID_COMPILER_OPTIONS:
		return (char*) "CL_INVALID_COMPILER_OPTIONS";
	case CL_INVALID_LINKER_OPTIONS:
		return (char*) "CL_INVALID_LINKER_OPTIONS";
	case CL_INVALID_DEVICE_PARTITION_COUNT:
		return (char*) "CL_INVALID_DEVICE_PARTITION_COUNT";
		/*        case CL_INVALID_PIPE_SIZE:
		return (char*) "CL_INVALID_PIPE_SIZE";
		case CL_INVALID_DEVICE_QUEUE:
		return (char*) "CL_INVALID_DEVICE_QUEUE";
		*/
	default:
		return (char*)"UNKNOWN ERROR CODE";
	}
}

void check_error_code(cl_int errcode, int line, const char *file) {
	if (errcode != CL_SUCCESS) {
		fprintf(stderr, "^^^ OpenCL error in Line %d of FILE %s: %s(%d)\n\n",
			line, file, get_error_flag(errcode), errcode);
		exit(EXIT_FAILURE);
	}
}

#define CHECK_ERROR_CODE(a) check_error_code(a, __LINE__-1, __FILE__);
/******************************************************************************************************/

/******************************************************************************************************/
//#define _SHOW_OPENCL_C_PROGRAM
size_t read_kernel_from_file(const char *filename, char **source_str) {
	FILE *fp;
	size_t count;
	errno_t err;

	err = fopen_s(&fp, filename, "rb");
	if (err != 0)
	{
		fprintf(stderr, "Error: cannot open the file %s for reading...\n", filename);
		exit(EXIT_FAILURE);
	}

	fseek(fp, 0, SEEK_END);
	count = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	*source_str = (char *)malloc(count + 1);
	if (*source_str == NULL) {
		fprintf(stderr, "Error: cannot allocate memory for reading the file %s for reading...\n", filename);
	}

	fread(*source_str, sizeof(char), count, fp);
	*(*source_str + count) = '\0';

	fclose(fp);

#ifdef _SHOW_OPENCL_C_PROGRAM
	fprintf(stdout, "\n^^^^^^^^^^^^^^ The OpenCL C program ^^^^^^^^^^^^^^\n%s\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n", *source_str);
#endif

	return count;
}
/******************************************************************************************************/

/******************************************************************************************************/
void print_build_log(cl_program program, cl_device_id device, const char *title_suppl) {
	cl_int errcode_ret;
	char *string;
	size_t string_length;

	fprintf(stderr, "\n^^^^^^^^^^^^ Program build log (%s) ^^^^^^^^^^^^\n", title_suppl);
	errcode_ret = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &string_length);
	CHECK_ERROR_CODE(errcode_ret);

	string = (char *)malloc(string_length);
	if (string == NULL) {
		fprintf(stderr, "Error: cannot allocate memory for holding a program build log...\n");
	}

	errcode_ret = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, string_length, string, NULL);
	CHECK_ERROR_CODE(errcode_ret);

	fprintf(stderr, "%s", string);
	fprintf(stderr, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	free(string);
}
/******************************************************************************************************/

/******************************************************************************************************/
cl_ulong compute_elapsed_time(cl_event event, cl_profiling_info from, cl_profiling_info to) {
	cl_ulong from_time, to_time;
	cl_int errcode_ret;

	errcode_ret = clGetEventProfilingInfo(event, from, sizeof(cl_ulong), &from_time, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clGetEventProfilingInfo(event, to, sizeof(cl_ulong), &to_time, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	return (cl_ulong)(to_time - from_time);
}

void print_device_time(cl_event event) {
	// Consider CL_PROFILING_COMMAND_END to include for OpenCL 2.0
	cl_ulong time_elapsed;

	fprintf(stdout, "     * Time by device clock:\n");
	time_elapsed = compute_elapsed_time(event, CL_PROFILING_COMMAND_QUEUED, CL_PROFILING_COMMAND_END);
	fprintf(stdout, "       - Time from QUEUED to END = %.3fms\n", time_elapsed * 1.0e-6f);
	time_elapsed = compute_elapsed_time(event, CL_PROFILING_COMMAND_SUBMIT, CL_PROFILING_COMMAND_END);
	fprintf(stdout, "       - Time from SUBMIT to END = %.3fms\n", time_elapsed * 1.0e-6f);
	time_elapsed = compute_elapsed_time(event, CL_PROFILING_COMMAND_START, CL_PROFILING_COMMAND_END);
	fprintf(stdout, "       - Time from START to END = %.3fms\n\n", time_elapsed * 1.0e-6f);
}
/******************************************************************************************************/

/******************************************************************************************************/
void print_device_0(cl_device_id device) {
#define MAX_BUFFER_SIZE 1024
	char _buffer[MAX_BUFFER_SIZE]; // Use a char buffer of enough size for convenience.


	clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_NAME:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_TYPE:\t\t\t\t");
	{
		cl_device_type tmp = *((cl_device_type *)_buffer);
		if (tmp & CL_DEVICE_TYPE_CPU) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_CPU");
		if (tmp & CL_DEVICE_TYPE_GPU) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_GPU");
		if (tmp & CL_DEVICE_TYPE_ACCELERATOR) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_ACCELERATOR");
		if (tmp & CL_DEVICE_TYPE_DEFAULT) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_DEFAULT");
		if (tmp & CL_DEVICE_TYPE_CUSTOM) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_CUSTOM");
	}
	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_VENDOR:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_VERSION:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_PROFILE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PROFILE:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DRIVER_VERSION:\t\t\t\t%s\n", _buffer);

	fprintf(stdout, "\n");
}
/******************************************************************************************************/

/******************************************************************************************************/
void print_platform(cl_platform_id *platforms, int i) {
	// No error checking is made.
	char *param_value;
	size_t param_value_size;

	clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, NULL, NULL, &param_value_size);
	param_value = (char *)malloc(param_value_size);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, param_value_size, param_value, NULL);
	fprintf(stdout, "  * CL_PLATFORM_NAME:\t\t\t\t%s\n", param_value);
	free(param_value);

	clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, NULL, NULL, &param_value_size);
	param_value = (char *)malloc(param_value_size);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, param_value_size, param_value, NULL);
	fprintf(stdout, "  * CL_PLATFORM_VENDOR:\t\t\t\t%s\n", param_value);
	free(param_value);

	clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, NULL, NULL, &param_value_size);
	param_value = (char *)malloc(param_value_size);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, param_value_size, param_value, NULL);
	fprintf(stdout, "  * CL_PLATFORM_VERSION:\t\t\t%s\n", param_value);
	free(param_value);

	clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, NULL, NULL, &param_value_size);
	param_value = (char *)malloc(param_value_size);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, param_value_size, param_value, NULL);
	fprintf(stdout, "  * CL_PLATFORM_PROFILE:\t\t\t%s\n", param_value);
	free(param_value);

	clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, NULL, NULL, &param_value_size);
	param_value = (char *)malloc(param_value_size);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, param_value_size, param_value, NULL);
	fprintf(stdout, "  * CL_PLATFORM_EXTENSIONS:\t\t\t%s\n", param_value);
	free(param_value);
}

void print_device(cl_device_id *devices, int j) {
	// No error checking is made.
#define MAX_BUFFER_SIZE 1024
	char _buffer[MAX_BUFFER_SIZE]; // Use a char buffer of enough size for convenience.
	cl_device_id device;

	device = devices[j];

	clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_NAME:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_TYPE:\t\t\t\t");
	{
		cl_device_type tmp = *((cl_device_type *)_buffer);
		if (tmp & CL_DEVICE_TYPE_CPU) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_CPU");
		if (tmp & CL_DEVICE_TYPE_GPU) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_GPU");
		if (tmp & CL_DEVICE_TYPE_ACCELERATOR) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_ACCELERATOR");
		if (tmp & CL_DEVICE_TYPE_DEFAULT) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_DEFAULT");
		if (tmp & CL_DEVICE_TYPE_CUSTOM) fprintf(stdout, "%s ", "CL_DEVICE_TYPE_CUSTOM");
	}
	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_AVAILABLE:\t\t\t%s\n", *((cl_bool *)_buffer) == CL_TRUE ? "YES" : "NO");

	clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_VENDOR:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_VENDOR_ID, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_VENDOR_ID:\t\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_VERSION:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_PROFILE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PROFILE:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DRIVER_VERSION:\t\t\t\t%s\n", _buffer);

	clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_EXTENSIONS:\t\t\t%s\n", _buffer);

	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_COMPUTE_UNITS:\t\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_WORK_ITEM_SIZES:\t\t\t%lu / %lu / %lu \n",
		*((size_t *)_buffer), *(((size_t *)_buffer) + 1), *(((size_t *)_buffer) + 2));

	clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_WORK_GROUP_SIZE:\t\t\t%lu\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_CLOCK_FREQUENCY:\t\t\t%u MHz\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_ADDRESS_BITS:\t\t\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_MEM_ALLOC_SIZE:\t\t\t%llu MBytes\n", *((cl_ulong *)_buffer) >> 20);

	clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_GLOBAL_MEM_SIZE:\t\t\t\t%llu MBytes\n", *((cl_ulong *)_buffer) >> 20);

	clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\t");
	switch (*((cl_device_mem_cache_type *)_buffer)) {
	case CL_NONE:
		fprintf(stdout, "CL_NONE\n");
		break;
	case CL_READ_ONLY_CACHE:
		fprintf(stdout, "CL_READ_ONLY_CACHE\n");
		break;
	case CL_READ_WRITE_CACHE:
		fprintf(stdout, "CL_READ_WRITE_CACHE\n");
		break;
	}

	clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:\t\t%lluKBytes\n", *((cl_ulong *)_buffer) >> 10);

	clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:\t%u Bytes\n", *((cl_int *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_LOCAL_MEM_TYPE:\t\t\t\t%s\n", *((cl_device_local_mem_type *)_buffer) == CL_LOCAL ? "LOCAL" : "GLOBAL");

	clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_LOCAL_MEM_SIZE:\t\t\t\t%llu KByte\n", *((cl_ulong *)_buffer) >> 10);

	clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_CONSTANT__buffer_SIZE:\t%llu MBytes\n", *((cl_ulong *)_buffer) >> 20);

	clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MEM_BASE_ADDR_ALIGN:\t\t\t%u Bits\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:\t%u Bytes\n", *((cl_uint *)_buffer));

	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_EXECUTION_CAPABILITIES:\t\t");
	{
		cl_device_exec_capabilities tmp = *((cl_device_exec_capabilities *)_buffer);
		if (tmp & CL_EXEC_KERNEL) fprintf(stdout, "%s ", "CL_EXEC_KERNEL");
		if (tmp & CL_EXEC_NATIVE_KERNEL) fprintf(stdout, "%s ", "CL_EXEC_NATIVE_KERNEL");
	}
	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_QUEUE_PROPERTIES:\t\t\t");
	{
		cl_command_queue_properties tmp = *((cl_command_queue_properties *)_buffer);
		if (tmp & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
			fprintf(stdout, "%s ", "CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE");
		if (tmp & CL_QUEUE_PROFILING_ENABLE)
			fprintf(stdout, "%s ", "CL_QUEUE_PROFILING_ENABLE");
	}
	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_ERROR_CORRECTION_SUPPORT:\t%s\n", *((cl_bool *)_buffer) == CL_TRUE ? "YES" : "NO");

	clGetDeviceInfo(device, CL_DEVICE_ENDIAN_LITTLE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_ENDIAN_LITTLE:\t\t\t\t%s\n", *((cl_bool *)_buffer) == CL_TRUE ? "YES" : "NO");

	clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_COMPILER_AVAILABLE:\t\t\t%s\n", *((cl_bool *)_buffer) == CL_TRUE ? "YES" : "NO");

	clGetDeviceInfo(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PROFILING_TIMER_RESOLUTION:\t%lu nanosecond(s)\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_PARAMETER_SIZE:\t\t\t%lu Bytes\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_CONSTANT_ARGS:\t\t\t%u\n", *((cl_uint *)_buffer));

	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_IMAGE_SUPPORT:\t\t\t\t%s\n", *((cl_bool *)_buffer) == CL_TRUE ? "YES" : "NO");

	clGetDeviceInfo(device, CL_DEVICE_MAX_SAMPLERS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_SAMPLERS:\t\t\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_READ_IMAGE_ARGS:\t\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_MAX_WRITE_IMAGE_ARGS:\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_IMAGE2D_MAX_WIDTH:\t\t\t%lu\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_IMAGE2D_MAX_HEIGHT:\t\t\t%lu\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_IMAGE3D_MAX_WIDTH:\t\t\t%lu\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_IMAGE3D_MAX_HEIGHT:\t\t\t%lu\n", *((size_t *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_IMAGE3D_MAX_DEPTH:\t\t\t%lu\n", *((size_t *)_buffer));

	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:\t\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:\t%u\n", *((cl_uint *)_buffer));

	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:\t%u\n", *((cl_uint *)_buffer));

	fprintf(stdout, "\n");

	clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_SINGLE_FP_CONFIG:\t\t");
	{
		cl_device_fp_config tmp = *((cl_device_fp_config *)_buffer);
		if (tmp & CL_FP_DENORM) fprintf(stdout, "%s ", "CL_FP_DENORM");
		if (tmp & CL_FP_INF_NAN) fprintf(stdout, "%s ", "CL_FP_INF_NAN");
		if (tmp & CL_FP_ROUND_TO_NEAREST) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_NEAREST");
		if (tmp & CL_FP_ROUND_TO_ZERO) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_ZERO");
		if (tmp & CL_FP_ROUND_TO_INF) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_INF");
		if (tmp & CL_FP_FMA) fprintf(stdout, "%s ", "CL_FP_FMA");
	}
	fprintf(stdout, "\n");
	/*
	clGetDeviceInfo(device, CL_DEVICE_HALF_FP_CONFIG, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_SINGLE_FP_CONFIG:\t\t");
	{
	cl_device_fp_config tmp = *((cl_device_fp_config *)_buffer);
	if (tmp & CL_FP_DENORM) fprintf(stdout, "%s ", "CL_FP_DENORM");
	if (tmp & CL_FP_INF_NAN) fprintf(stdout, "%s ", "CL_FP_INF_NAN");
	if (tmp & CL_FP_ROUND_TO_NEAREST) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_NEAREST");
	if (tmp & CL_FP_ROUND_TO_ZERO) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_ZERO");
	if (tmp & CL_FP_ROUND_TO_INF) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_INF");
	if (tmp & CL_FP_FMA) fprintf(stdout, "%s ", "CL_FP_FMA");
	}
	fprintf(stdout, "\n");
	*/

	clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(_buffer), _buffer, NULL);
	fprintf(stdout, "   - CL_DEVICE_DOUBLE_FP_CONFIG:\t\t");
	{
		cl_device_fp_config tmp = *((cl_device_fp_config *)_buffer);
		if (tmp & CL_FP_DENORM) fprintf(stdout, "%s ", "CL_FP_DENORM");
		if (tmp & CL_FP_INF_NAN) fprintf(stdout, "%s ", "CL_FP_INF_NAN");
		if (tmp & CL_FP_ROUND_TO_NEAREST) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_NEAREST");
		if (tmp & CL_FP_ROUND_TO_ZERO) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_ZERO");
		if (tmp & CL_FP_ROUND_TO_INF) fprintf(stdout, "%s ", "CL_FP_ROUND_TO_INF");
		if (tmp & CL_FP_FMA) fprintf(stdout, "%s ", "CL_FP_FMA");
	}
	fprintf(stdout, "\n");
}

void print_devices(cl_platform_id *platforms, int i) {
	cl_uint n_devices;
	cl_device_id *devices;
	cl_int errcode_ret;

	errcode_ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, NULL, NULL, &n_devices);
	CHECK_ERROR_CODE(errcode_ret);

	devices = (cl_device_id *)malloc(sizeof(cl_device_id) * n_devices);
	errcode_ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, n_devices, devices, NULL);
	CHECK_ERROR_CODE(errcode_ret);

	for (int j = 0; j < n_devices; j++) {
		fprintf(stdout, "----- [Begin of Device %d(%d) of Platform %d] ----------------------------------------------------------------------\n\n",
			j, n_devices, i);
		print_device(devices, j);
		fprintf(stdout, "\n----- [End of Device %d(%d) of Platform %d] ------------------------------------------------------------------------\n\n",
			j, n_devices, i);
	}
	free(devices);
}

void show_OpenCL_platform(void) {
	cl_uint n_platforms;
	cl_platform_id *platforms;
	cl_int errcode_ret;

	errcode_ret = clGetPlatformIDs(NULL, NULL, &n_platforms);
	CHECK_ERROR_CODE(errcode_ret);

	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * n_platforms);

	errcode_ret = clGetPlatformIDs(n_platforms, platforms, NULL);
	CHECK_ERROR_CODE(errcode_ret);

	fprintf(stdout, "\n");
	for (int i = 0; i < n_platforms; i++) {
		fprintf(stdout, "===== [Begin of Platform %d(%d)] ========================================================================================\n\n",
			i, n_platforms);
		print_platform(platforms, i);
		fprintf(stdout, "\n");
		print_devices(platforms, i);
		fprintf(stdout, "===== [End of Platform %d(%d)] ==========================================================================================\n\n",
			i, n_platforms);
	}
	free(platforms);
}
/******************************************************************************************************/

/******************************************************************************************************/
void printf_KernelWorkGroupInfo(cl_kernel kernel, cl_device_id device) {
	cl_int errcode_ret;
	size_t tmp_size[3];
	cl_ulong tmp_ulong = 0;

	errcode_ret = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
		sizeof(size_t), (void *)tmp_size, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	fprintf(stdout, "   # The preferred multiple of workgroup size for launch (hint) is %lu.\n", tmp_size[0]);

	errcode_ret = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE,
		sizeof(size_t), (void *)tmp_size, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	fprintf(stdout, "   # The maximum work-group size that can be used to execute this kernel on this device is %lu.\n", tmp_size[0]);

	errcode_ret = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_LOCAL_MEM_SIZE,
		sizeof(cl_ulong), (void *)tmp_ulong, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	fprintf(stdout, "   # The amount of local memory in bytes being used by this kernel is %llu.\n", tmp_ulong);

	errcode_ret = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE,
		sizeof(cl_ulong), (void *)tmp_ulong, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	fprintf(stdout, "   # The minimum amount of private memory in bytes used by each workitem in the kernel is %llu.\n", tmp_ulong);

	errcode_ret = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
		sizeof(size_t) * 3, (void *)tmp_size, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	fprintf(stdout, "   # The work-group size specified by the __attribute__((reqd_work_group_size(X, Y, Z))) qualifier is (%lu, %lu, %lu).\n",
		tmp_size[0], tmp_size[1], tmp_size[2]);

	fprintf(stdout, "\n");
}

bool InitializeOpenCL() {
	cl_int errcode_ret;
	float compute_time;

	OPENCL_C_PROG_SRC prog_src_bep, prog_src_dsc;

	if (0) {
		// Just to reveal my OpenCl platform...
		show_OpenCL_platform();
		return false;
	}

	/* Get the first platform. */
	errcode_ret = clGetPlatformIDs(1, &platform, NULL);
	CHECK_ERROR_CODE(errcode_ret);

	/* Get the first GPU device. */
	errcode_ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	CHECK_ERROR_CODE(errcode_ret);

	/* Get string containing supported device extensions. */
	size_t ext_size = 1024;
	char* ext_string = (char*)malloc(ext_size);
	errcode_ret = clGetDeviceInfo(device, CL_DEVICE_NAME, ext_size, ext_string, &ext_size);
	fprintf(stdout, "GPU DEVICE NAME : %s\n", ext_string);
	free(ext_string);

	/* Create a context with the devices. */
#ifdef __APPLE__
	// Get current CGL Context and CGL Share group
	CGLContextObj kCGLContext = CGLGetCurrentContext();
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);

	// Create CL context properties, add handle & share-group enum
	cl_context_properties properties[] = {
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
		(cl_context_properties)kCGLShareGroup, 0
	};
#else
	// Create CL context properties, add WGL context & handle to DC
	cl_context_properties properties[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform, // OpenCL platform
		0
	};
#endif

	/* Create a context with the devices. */
	context = clCreateContext(properties, 1, &device, NULL, NULL, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	/* Create a command-queue for the GPU device. */
	cmd_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	buffer_iq_input = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(short) * 2 * SCANLINE_NUM * SAMPLE_NUM, NULL, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);
	buffer_tgc_curve = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * SAMPLE_NUM, NULL, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);
	buffer_bep_result = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * SCANLINE_NUM * SAMPLE_NUM, NULL, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);
	buffer_dsc_result = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned char) * WIDTH * HEIGHT, NULL, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	/* Create a program from OpenCL C source code. */
	prog_src_bep.length = read_kernel_from_file(OPENCL_C_PROG_BEP_FILE_NAME, &prog_src_bep.string);
	program[0] = clCreateProgramWithSource(context, 1, (const char **)&prog_src_bep.string, &prog_src_bep.length, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	prog_src_dsc.length = read_kernel_from_file(OPENCL_C_PROG_DSC_FILE_NAME, &prog_src_dsc.string);
	program[1] = clCreateProgramWithSource(context, 1, (const char **)&prog_src_dsc.string, &prog_src_dsc.length, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	/* Build a program executable from the program object. */
	errcode_ret = clBuildProgram(program[0], 1, &device, NULL, NULL, NULL);
	if (errcode_ret != CL_SUCCESS) {
		print_build_log(program[0], device, "GPU");
		return false;
	}

	/* Build a program executable from the program object. */
	errcode_ret = clBuildProgram(program[1], 1, &device, NULL, NULL, NULL);
	if (errcode_ret != CL_SUCCESS) {
		print_build_log(program[1], device, "GPU");
		return false;
	}

	/* Create the kernel from the program. */
	kernel[0] = clCreateKernel(program[0], KERNEL_BEP_NAME, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	/* Create the kernel from the program. */
	kernel[1] = clCreateKernel(program[1], KERNEL_DSC_NAME, &errcode_ret);
	CHECK_ERROR_CODE(errcode_ret);

	free(prog_src_bep.string);
	free(prog_src_dsc.string);

	return true;
}

void FinalizeOpenCL(void) {
	cl_int errcode_ret;

	clFlush(cmd_queue);
	clFinish(cmd_queue);
	errcode_ret = clReleaseKernel(kernel[0]);
	errcode_ret = clReleaseKernel(kernel[1]);
	errcode_ret = clReleaseProgram(program[0]);
	errcode_ret = clReleaseProgram(program[1]);
	errcode_ret = clReleaseContext(context);
}

static float bep_buffer[SCANLINE_NUM * SAMPLE_NUM];
void save_iq_data_to_jpeg_image(const char *iq_data, const char *filename, cv::VideoWriter &video_test)
{
	cl_int errcode_ret;
	const size_t global_work_size_bep[2] = { 1024, 128 };
	const size_t global_work_size_dsc[2] = { 1024, 1024 };
	const size_t local_work_size[2] = { 16, 8 };
	cv::Mat mat(HEIGHT, WIDTH, CV_8UC1);
	float gain = 100.0f / 8000.0f;
	float DR = 20.0f * log10f(60.0f * 150.0f);
	float min = 15000.0f / (pow(10.0, DR / 20.0));
	float logFactor = 255.0f * 20.0 / DR / log10f(10.0f);
	const float dscFactor = 128.0f / WIDTH;

	errcode_ret = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &buffer_iq_input);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &buffer_bep_result);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[0], 2, sizeof(cl_mem), &buffer_tgc_curve);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[0], 3, sizeof(float), &gain);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[0], 4, sizeof(float), &DR);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[0], 5, sizeof(float), &min);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[0], 6, sizeof(float), &logFactor);
	CHECK_ERROR_CODE(errcode_ret);

	clEnqueueWriteBuffer(cmd_queue, buffer_iq_input, CL_FALSE, 0, sizeof(short) * SCANLINE_NUM * SAMPLE_NUM * 2, iq_data, 0, NULL, NULL);
	clFinish(cmd_queue);
	clEnqueueWriteBuffer(cmd_queue, buffer_tgc_curve, CL_FALSE, 0, sizeof(float) * SAMPLE_NUM, TGC_CURVE, 0, NULL, NULL);
	clFinish(cmd_queue);

	clEnqueueNDRangeKernel(cmd_queue, kernel[0], 2, NULL, global_work_size_bep, local_work_size, 0, NULL, NULL);
	clFinish(cmd_queue);

	errcode_ret = clSetKernelArg(kernel[1], 0, sizeof(cl_mem), &buffer_bep_result);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[1], 1, sizeof(cl_mem), &buffer_dsc_result);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[1], 2, sizeof(float), &dscFactor);
	CHECK_ERROR_CODE(errcode_ret);
	errcode_ret = clSetKernelArg(kernel[1], 3, sizeof(int), &WIDTH);
	CHECK_ERROR_CODE(errcode_ret);

	errcode_ret = clEnqueueNDRangeKernel(cmd_queue, kernel[1], 2, NULL, global_work_size_dsc, local_work_size, 0, NULL, NULL);
	CHECK_ERROR_CODE(errcode_ret);
	clFinish(cmd_queue);

	clEnqueueReadBuffer(cmd_queue, buffer_dsc_result, CL_FALSE, 0, sizeof(unsigned char) * WIDTH * HEIGHT, mat.data, 0, NULL, NULL);
	clFinish(cmd_queue);

	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_OPTIMIZE);
	compression_params.push_back(CV_IMWRITE_PAM_FORMAT_GRAYSCALE);
	imwrite(filename, mat, compression_params);

	video_test.write(mat);

	//cv::Mat out;
	//cv::cvtColor(mat, out, CV_GRAY2BGR);
	//video_test.write(out);
}

bool CheckDirectory(const char *path_name)
{
	bool result = false;
	WIN32_FIND_DATA FN;
	HANDLE hFind;
	string cs;

	//printf("check directory - %s\n", path_name);
	hFind = FindFirstFile((LPCSTR)path_name, &FN);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			cs = FN.cFileName;
			if (cs == "iq_data"
				&& FN.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				result = true;
			}
		} while (FindNextFile(hFind, &FN) != 0);

		if (GetLastError() == ERROR_NO_MORE_FILES)
		{
			FindClose(hFind);
		}
	}

	return result;
}

void GetIQDataFiles(const char *path_name, vector<string> *fileList)
{
	WIN32_FIND_DATA FN;
	HANDLE hFind;
	char search_arg[MAX_PATH];
	string cs;
	string parent_directory = string(path_name);
	parent_directory.append("\\iq_data\\");
	sprintf_s(search_arg, MAX_PATH, "%s\\iq_data\\*_0.iqdata", path_name);

	hFind = FindFirstFile((LPCTSTR)search_arg, &FN);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			cs.clear();
			cs = parent_directory + FN.cFileName;
			fileList->push_back(cs);
		} while (FindNextFile(hFind, &FN) != 0);

		if (GetLastError() == ERROR_NO_MORE_FILES)
		{
			FindClose(hFind);
		}
	}
}

void GetDirectories(const char *path_name, vector<string> *dirList)
{
	WIN32_FIND_DATA FN;
	HANDLE hFind;
	char search_arg[MAX_PATH];
	string cs;
	sprintf_s(search_arg, MAX_PATH, "%s\\*", path_name);

	hFind = FindFirstFile((LPCTSTR)search_arg, &FN);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FN.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				cs = FN.cFileName;
				if (cs != "." && cs != "..")
				{
					dirList->push_back(cs);
				}
			}
		} while (FindNextFile(hFind, &FN) != 0);

		if (GetLastError() == ERROR_NO_MORE_FILES)
		{
			FindClose(hFind);
		}
	}
}

static char iq_test[SAMPLE_NUM * SCANLINE_NUM * 2 * 2] = { 0, };
__int64 _start, _freq, _end;
#define CHECK_TIME_START QueryPerformanceFrequency((LARGE_INTEGER*)&_freq); QueryPerformanceCounter((LARGE_INTEGER*)&_start)
#define CHECK_TIME_END(a) QueryPerformanceCounter((LARGE_INTEGER*)&_end); a = (float)((float)(_end - _start) / (_freq * 1.0e-3f))

void ConvertIQDataImageAndVideo(string output_image_path, string output_video_path, vector<string> &fileList)
{
	char file_name[MAX_PATH];
	FILE *fp;
	errno_t err;
	int i;
	float total_elapsed_time = 0.0f;
	float compute_time;
	cv::VideoWriter video_test(output_video_path.c_str(), cv::VideoWriter::fourcc('8', 'B', 'P', 'S'), 10.0, cv::Size(WIDTH, HEIGHT), false);
	video_test.set(
	)

	fprintf(stdout, "Processing");

	i = 0;
	for (vector<string>::iterator iter = fileList.begin();
		iter != fileList.end();
		iter++, i++)
	{
		if (i % 10 == 0) putchar('.');
		CHECK_TIME_START;
		//printf("open file : %s\n", (*iter).c_str());
		err = fopen_s(&fp, (*iter).c_str(), "rb");
		if (err == 0)
		{
			fread(iq_test, sizeof(short), SAMPLE_NUM * SCANLINE_NUM * 2, fp);
			fclose(fp);
		}

		snprintf(file_name, MAX_PATH, "%s\\%05d.jpg", output_image_path.c_str(), i);
		save_iq_data_to_jpeg_image(iq_test, file_name, video_test);
		CHECK_TIME_END(compute_time);
		//printf("1 cycle clock = %.3fms\n", compute_time);
		total_elapsed_time += compute_time;
	}

	video_test.release();
	fprintf(stdout, "\ntotal clock = %.3fms\nConversion is over\n", total_elapsed_time);
}

static char network_iq_data_buf[1024 * 4 * 128 * 2];
void ConvertIQDataToVideo(string output_image_path, string output_video_path, SOCKET *connection)
{
	bool isSocketConnect = true;
	int offset;
	int len;
	int temp;
	int frame_number = 0;
	char file_name_buffer[MAX_PATH];
	cv::VideoWriter video_test(output_video_path.c_str(), CV_FOURCC('M', 'J', 'P', 'G'), 10.0, cv::Size(WIDTH, HEIGHT), false);

	while (isSocketConnect)
	{
		offset = 0;
		len = 1024 * 4 * 128;

		while (offset < len)
		{
			temp = recv(*connection, network_iq_data_buf + offset, len - offset, 0);
			if (temp == 0)
			{
				isSocketConnect = false;
				printf("socket close\n");
				break;
			}
			else if (temp < 0)
			{
				isSocketConnect = false;
				printf("socket error : %d\n", WSAGetLastError());
				break;
			}
			offset += temp;
		}

		if (offset == len)
		{
			snprintf(file_name_buffer, MAX_PATH, "%s\\%05d.jpg", output_image_path.c_str(), frame_number);

			save_iq_data_to_jpeg_image(network_iq_data_buf, file_name_buffer, video_test);
			
			frame_number++;
		}
	}

	video_test.release();

	printf("Video Writer End\n");
}

DWORD WINAPI image_processing_thread_main(LPVOID lpParam)
{
	vector<string> dirList;
	vector<string> iqFileList;
	string testDirName;
	string outputVideoPath;
	string outputImageDir;
	bool result;

	InitializeOpenCL();

	for (int i = 0; i < SAMPLE_NUM; i++)
	{
		TGC_CURVE[i] = 0.3 + i / SAMPLE_NUM * 0.5;
	}

	GetDirectories(".", &dirList);
	for (vector<string>::iterator iter = dirList.begin();
		iter != dirList.end();
		iter++)
	{
		break;
		testDirName = (*iter);
		testDirName.append("\\*");
		result = CheckDirectory(testDirName.c_str());

		if (result)
		{
			fprintf(stdout, "\n\nconvert dir : %s\\iq_data\n", (*iter).c_str());
			outputVideoPath = (*iter);
			outputImageDir = (*iter);

			outputVideoPath.append("\\video");
			outputImageDir.append("\\image");

			CreateDirectory(outputVideoPath.c_str(), NULL);
			CreateDirectory(outputImageDir.c_str(), NULL);

			fprintf(stdout, "output video directory : %s\n", outputVideoPath.c_str());
			fprintf(stdout, "output image directory : %s\n", outputImageDir.c_str());

			outputVideoPath.append("\\output.avi");

			iqFileList.clear();
			GetIQDataFiles((*iter).c_str(), &iqFileList);

			ConvertIQDataImageAndVideo(outputImageDir.c_str(), outputVideoPath.c_str(), iqFileList);
		}
	}
	
	struct sockaddr_in my_sockaddr_in;
	SOCKET tcp_listen_socket;
	SOCKET tcp_client_socket;
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];
	int opt = 1;
	struct timeval tv;
	int res;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	tcp_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	res = setsockopt(tcp_listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));
	printf("setsockopt result : %d\n", res);

	my_sockaddr_in.sin_family = AF_INET;
	my_sockaddr_in.sin_addr.s_addr = INADDR_ANY;
	my_sockaddr_in.sin_port = htons(27272);
      
	res = ::bind(
		tcp_listen_socket,
		(const struct sockaddr *)&my_sockaddr_in,
		(int)sizeof(my_sockaddr_in)
	);
	if (res == SOCKET_ERROR)
	{
		printf("bind failed");
		return 0;
	}

	if (::listen(tcp_listen_socket, 1) == SOCKET_ERROR)
	{
		printf("listend failed");
		return 0;
	}

	{
		u_long opt = 1;
		::ioctlsocket(tcp_listen_socket, FIONBIO, &opt);
	}

	int len = sizeof(struct sockaddr_in);
	fd_set select_fd;
	while (!is_program_exit)
	{
		FD_ZERO(&select_fd);
		FD_SET(tcp_listen_socket, &select_fd);

		res = select(tcp_listen_socket + 1, &select_fd, (fd_set *)NULL, (fd_set *)NULL, &tv);
		if (res <= 0)
		{
			time(&rawtime);
			localtime_s(&timeinfo, &rawtime);
			strftime(buffer, sizeof(buffer), "%H%M%S", &timeinfo);
			fprintf(stdout, "accept timeout - %s\n", buffer);
			continue;
		}

		tcp_client_socket = ::accept(
			tcp_listen_socket,
			(struct sockaddr *)&my_sockaddr_in,
			&len
		);

		{
			u_long opt = 0;
			::ioctlsocket(tcp_client_socket, FIONBIO, &opt);
		}

		if (tcp_client_socket == INVALID_SOCKET)
		{
			printf("accept failed (INVALID_SOCKET) : %d\n", WSAGetLastError());
			continue;
		}

		time(&rawtime);
		localtime_s(&timeinfo, &rawtime);
		strftime(buffer, sizeof(buffer), "%y%m%d_%H%M%S", &timeinfo);

		printf("connect client!! - %s\n", buffer);

		CreateDirectory(buffer, NULL);

		outputImageDir = string(buffer);
		outputVideoPath = string(buffer);

		outputImageDir.append("\\image");
		outputVideoPath.append("\\video");

		CreateDirectory(outputVideoPath.c_str(), NULL);
		CreateDirectory(outputImageDir.c_str(), NULL);

		outputVideoPath.append("\\video.avi");

		ConvertIQDataToVideo(outputImageDir, outputVideoPath, &tcp_client_socket);

		closesocket(tcp_client_socket);
		Sleep(500);
	}

	FinalizeOpenCL();

	closesocket(tcp_listen_socket);
	fprintf(stdout, "end image processing thread\n");

	return 0;
}