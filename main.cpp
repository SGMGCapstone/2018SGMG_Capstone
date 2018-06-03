#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <conio.h>
#include <winsock2.h>
#include <ws2def.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <thread>

#include "TCHAR.h"
#include "pdh.h"

#define PRINT_SOCKET_LAST_ERROR(X) (X) = WSAGetLastError();\
	printf("Socket Create Error : %x(%d)\n", (X), (X))

using namespace std;

static const int PORT_NUMBER = 17171;


static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

unsigned int nthreads = std::thread::hardware_concurrency();
static SOCKET socket_descriptor;
bool is_program_exit = false;
double dCPUSpeedMHz;
clock_t start, endtime;
void init() {
	PdhOpenQuery(NULL, NULL, &cpuQuery);
	PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
	PdhCollectQueryData(cpuQuery);
}
void GetProcessorSpeed()
{
	LARGE_INTEGER qwWait, qwStart, qwCurrent;
	QueryPerformanceCounter(&qwStart);
	QueryPerformanceFrequency(&qwWait);
	qwWait.QuadPart >>= 5;
	unsigned __int64 Start = __rdtsc();
	do
	{
		QueryPerformanceCounter(&qwCurrent);
	} while (qwCurrent.QuadPart - qwStart.QuadPart < qwWait.QuadPart);
	dCPUSpeedMHz = ((__rdtsc() - Start) << 5) / 1000000000.0;
}
DWORD WINAPI broadcast_thread_main(LPVOID lpParam)
{

	int temp;
	struct sockaddr_in sockaddr;
	PDH_FMT_COUNTERVALUE counterVal;

	double _msg;
	char  message[4]; //1 : cpu 클럭 2 : cpu 사용률 3: core 갯수 4: gpu 점유율 5: gpu core수

	GetProcessorSpeed();

	message[0] = (int)(10 * dCPUSpeedMHz);

	memset(&sockaddr, 0, sizeof(struct sockaddr_in));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(PORT_NUMBER);
	//sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;
	inet_pton(AF_INET, "255.255.255.255", &sockaddr.sin_addr.S_un.S_addr);

	while (!is_program_exit)
	{
		// monitering
		Sleep(500);
		PdhCollectQueryData(cpuQuery);
		PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
		message[1] = (int)counterVal.doubleValue;
		message[2] = nthreads;
		temp = sendto(socket_descriptor, message, sizeof(int), 0, (const struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));
	}

	printf("end broadcast thread\n");

	return 0;
}

int main()
{
	int ret_val = 0;
	int temp;
	char socket_option;
	char c;
	register int test;
	WSADATA wsaData;
	HANDLE broadcast_thread_handle, image_processing_thread_handle = NULL;
	DWORD broadcast_thread_id, image_processing_thread_id;



	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// socket 생성
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_descriptor == INVALID_SOCKET)
	{
		PRINT_SOCKET_LAST_ERROR(temp);
		ret_val = -1;
		goto SOCKET_CREATE_FAILED;
	}

	// socket 옵션 지정
	socket_option = 1; // true
	temp = setsockopt(socket_descriptor, SOL_SOCKET, SO_BROADCAST, (const char *)&socket_option, sizeof(char));
	if (temp == SOCKET_ERROR)
	{
		PRINT_SOCKET_LAST_ERROR(temp);
		ret_val = -1;
		goto SETSOCKOPT_FAILED;
	}
	init();
	// UDP broadcast를 반복할 thread 
	broadcast_thread_handle = CreateThread(NULL, 0, broadcast_thread_main, NULL, 0, &broadcast_thread_id);
	if (broadcast_thread_handle == NULL)
	{
		printf("CreateThreaad() failed, error : %d\n", GetLastError());
		ret_val = -1;
		goto CREATE_BROADCAST_THREAD_FAILED;
	}

	// probe로 부터 image를 받아 local에 저장할 thread
	//image_processing_thread_handle = CreateThread(NULL, 0, image_processing_thread_main, NULL, 0, &image_processing_thread_id);
	/*if (image_processing_thread_handle == NULL)
	{
	printf("CreateThreaad() failed, error : %d\n", GetLastError());
	ret_val = -1;
	goto CREATE_IMAGE_PROCESS_THREAD_FAILED;
	}*/

	// q를 누르면 program 종료
	printf("program exit - press 'q'\n");
	do
	{
		c = _getch();
	} while (c != 'q');

	is_program_exit = true;

	WaitForSingleObject(image_processing_thread_handle, INFINITE);
	CloseHandle(image_processing_thread_handle);

CREATE_IMAGE_PROCESS_THREAD_FAILED:
	is_program_exit = true;

	WaitForSingleObject(broadcast_thread_handle, INFINITE);
	CloseHandle(broadcast_thread_handle);

CREATE_BROADCAST_THREAD_FAILED:

SETSOCKOPT_FAILED:
	closesocket(socket_descriptor);

SOCKET_CREATE_FAILED:
	WSACleanup();

	return ret_val;
}