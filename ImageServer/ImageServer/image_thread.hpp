#ifndef __IMAGE_THREAD_H__
#define __IMAGE_THREAD_H__

#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

DWORD WINAPI image_processing_thread_main(LPVOID lpParam);

#endif /*__IMAGE_THREAD_H__*/