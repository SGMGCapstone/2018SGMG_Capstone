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
#include "image_thread.hpp"

#define PRINT_SOCKET_LAST_ERROR(X) (X) = WSAGetLastError();\
   printf("Socket Create Error : %x(%d)\n", (X), (X))

using namespace std;

static const int PORT_NUMBER = 17171;

struct packet
{
	int32_t magic;
	int32_t cpu_core_count;
	double cpu_clock;
	double cpu_usage;
};

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;
string  data1;
unsigned int nthreads = std::thread::hardware_concurrency();
static SOCKET socket_descriptor;
bool is_program_exit = false;
double dCPUSpeedMHz;
clock_t start, endtime;
void init() {
	PdhOpenQuery(NULL, NULL, &cpuQuery);
	PdhAddCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
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
void httpsend(void)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed.\n");
		return;
	}

	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct hostent *host;
	host = gethostbyname("184.106.153.149");
	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons(80);
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
	printf("Connecting...\n");
	if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
		printf("Could not connect");
		return;
	}
	printf("Connected.\r\n");
	send(Socket, data1.c_str(), strlen(data1.c_str()), 0);

	closesocket(Socket);
	WSACleanup();
	return;
}

DWORD WINAPI broadcast_thread_main(LPVOID lpParam)
{
	static struct packet data;
	int temp;
	char tmp[256];
	struct sockaddr_in sockaddr;
	PDH_FMT_COUNTERVALUE counterVal;

	double _msg;

	GetProcessorSpeed();

	data.magic = 0x20121632;
	data.cpu_clock = dCPUSpeedMHz;
	data.cpu_core_count = nthreads;

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
		data.cpu_usage = counterVal.doubleValue;
		sprintf_s(tmp, "%d HTTP/1.1\r\nUser-Agent: ununbum\r\nHost: 184.106.153.149\r\nConnection: close\r\n\r\n", (int)data.cpu_usage);
		data1 = "GET /update?api_key=WD112D9UCJ4ZWDPR&field2=" + string(tmp);
		httpsend();
		printf("%lf %d %lf  %d\n\n", data.cpu_clock, data.cpu_core_count, data.cpu_usage, sizeof(struct packet));
		temp = sendto(socket_descriptor, (const char *)&data, sizeof(struct packet), 0, (const struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));
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

	WSADATA wsaData;
	HANDLE broadcast_thread_handle, image_processing_thread_handle;
	DWORD broadcast_thread_id, image_processing_thread_id;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	init();

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

	// UDP broadcast를 반복할 thread 
	broadcast_thread_handle = CreateThread(NULL, 0, broadcast_thread_main, NULL, 0, &broadcast_thread_id);
	if (broadcast_thread_handle == NULL)
	{
		printf("CreateThreaad() failed, error : %d\n", GetLastError());
		ret_val = -1;
		goto CREATE_BROADCAST_THREAD_FAILED;
	}

	// probe로 부터 image를 받아 local에 저장할 thread
	image_processing_thread_handle = CreateThread(NULL, 0, image_processing_thread_main, NULL, 0, &image_processing_thread_id);
	if (image_processing_thread_handle == NULL)
	{
		printf("CreateThreaad() failed, error : %d\n", GetLastError());
		ret_val = -1;
		goto CREATE_IMAGE_PROCESS_THREAD_FAILED;
	}

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