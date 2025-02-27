// ClientBySelect_Test1.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <cstring>
#include <thread>
#include <stdio.h>
#include <tchar.h>
#include <WinSock2.h>
#include <iostream>
#include <assert.h>
#include <string.h>
#include <WS2tcpip.h>
#include <queue>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
#pragma warning(disable: 4996)

const int nMessageWordMaxSize = 1024;	//发送消息的文字信息最大的字节数

//TCP消息封装结构
struct TCPMessage
{
	bool bIsHeartBeat;	//是否为心跳报文
	char chMessage[nMessageWordMaxSize];	//发送的文字信息(这里为定长的大小)  //如果不要设置定长，则需要使用流模式去传递！！！	

	TCPMessage()
	{
		bIsHeartBeat = true;
		memset(chMessage, 0, nMessageWordMaxSize);
	}
};

const int nSize = sizeof(char) * sizeof(TCPMessage);

/*
	描述： 客户端发送消息头部解析结构体
	nSelfID:	自己的ID号 
	nSendToID:	接收者的ID号
	buf:		发送的信息
*/
const int nMessageBufMaxSize = nMessageWordMaxSize - 2*sizeof(UINT32);	//发送消息的文字信息最大的字节数
typedef struct ST_SendToIpInfo
{
	UINT32 nSelfID;		//4字节
	UINT32 nSendToID;	//4字节
	char buf[nMessageBufMaxSize];
}SendToIpInfo;

#define SERVER_PORT 8888
#define SERVER_IP "192.168.15.112"
bool bIsHaveConnect = false;
UINT32	selfID = 0;


//按:分割字符串
void SplitString(string str, SendToIpInfo& ipInfo)
{
	ipInfo.nSelfID = 0;
	ipInfo.nSendToID = 0;
	ZeroMemory(ipInfo.buf, nMessageBufMaxSize);
	char buf[16];
	char strinfo[nMessageWordMaxSize];
	ZeroMemory(strinfo, nMessageWordMaxSize);
	memcpy(strinfo, str.c_str(), str.length());
	char* p = strtok_s(strinfo, ":", (char**)(&buf));

	int count = 0;
	while (p)
	{
		switch (count)
		{
		case 0:
			ipInfo.nSelfID = atoi(p);
			break;
		case 1:
			ipInfo.nSendToID = atoi(p);
			break;
		case 2:
			memcpy(ipInfo.buf, p, nMessageBufMaxSize);
			break;
		}

		count++;
		p = strtok_s(NULL, ":", (char**)(&buf));

	}

}


//发送消息给服务器的消息队列
std::queue<string> m_qeSendMessage;
void ReceiveInput(VOID)
{
	char input[nMessageWordMaxSize] = { 0 };
	while (true)
	{
		cin.getline(input, nMessageWordMaxSize);	//字符串的“空格”也会获取！！
		if (input[0] != '\0')
		{
			SendToIpInfo ipInfo;
			SplitString(input, ipInfo);
			if (ipInfo.nSelfID == 0 || ipInfo.nSendToID == 0)
			{
				cout << "您输入的ID格式不正确，请重新输入！ " << endl;
				cout << "发送信息格式： 自己ID:接收者ID:发送的内容 " << endl;
				continue;
			}
			m_qeSendMessage.push(input);
			memset(input, 0, nMessageWordMaxSize);
		}

		//Sleep(1000);
	}
}


bool bIsPrintSendMsgSuccess = false;

//发送消息给服务器
void SendMsgToServer(fd_set& fdTemp, fd_set& fdRead, SOCKET& sclient)
{
	//取需要发送给server的信息
	string str = m_qeSendMessage.front();

	//对发送的报文信息进行封装
	char* cSendMessage = (char*)malloc(nMessageWordMaxSize);
	memset(cSendMessage, 0, nMessageWordMaxSize);

	////因为发送的文字信息为定长，所以，str.length()不能超过1024字节！！！
	//if (str.length() >= nMessageWordMaxSize)
	//{
	//	memcpy(cSendMessage, str.c_str(), nMessageWordMaxSize);
	//}
	//else
	//	memcpy(cSendMessage, str.c_str(), str.length());

	/*
		客户端输入格式：IP:端口号:发送信息
	*/

	SendToIpInfo ipInfo;
	SplitString(str, ipInfo);
	memcpy(cSendMessage, &ipInfo, nMessageWordMaxSize);
	ULONG ulFlags = MSG_PARTIAL;
	WSABUF	m_wsaBuf;
	m_wsaBuf.buf = cSendMessage;
	m_wsaBuf.len = nMessageWordMaxSize;

	INT nRet = WSASend(
		sclient,
		&m_wsaBuf,
		1,
		&(m_wsaBuf.len),
		ulFlags,
		NULL,
		NULL);

	if (SOCKET_ERROR == nRet && WSA_IO_PENDING != WSAGetLastError())
	{
		cout << SERVER_IP << "closed" << endl;

		//如果关闭相关的socket，则必须clear掉相关socket
		FD_CLR(sclient, &fdTemp);
		FD_CLR(sclient, &fdRead);

		if (sclient != INVALID_SOCKET)
		{
			closesocket(sclient);
			sclient = INVALID_SOCKET;

			sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sclient == INVALID_SOCKET)
			{
				cout << "创建socket失败！！！！" << endl;
				return;
			}

			FD_SET(sclient, &fdRead);
		}

		bIsHaveConnect = false;
	}
	else
	{
		m_qeSendMessage.pop();

		//如果是连接到服务器成功，则不打印
		if (bIsPrintSendMsgSuccess)
		{
			cout << "发送数据成功！！！！！！！！" << endl;
		}
		else
			bIsPrintSendMsgSuccess = true;
		
	}
	
	free(cSendMessage);
	cSendMessage = NULL;
}

int main()
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0)
	{
		cout << "WSAStartup(sockVersion, &data) == 0!!!!!" << endl;
		return 0;
	}

	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET)
	{
		cout << "创建socket失败！！！！" << endl;
		return 0;
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVER_PORT);
	//serAddr.nSelfID.S_un.S_addr = inet_addr(m_cIp);
	struct in_addr addr;
	inet_pton(AF_INET, SERVER_IP, (void *)&addr);
	serAddr.sin_addr.s_addr = addr.s_addr;

	fd_set fdRead;
	fd_set fdTemp;
	FD_ZERO(&fdRead);
	FD_SET(sclient, &fdRead);

	TIMEVAL tv;
	int nRet = 0;

	int nSize = sizeof(char) * (sizeof(TCPMessage));
	char* recvData = (char*)malloc(nSize);

	cout << "请设置您的ID（ID为大于0的4位整数）： ";

	char input[100] = { 0 };
	while (true)
	{
		cin.getline(input, nMessageWordMaxSize);	//字符串的“空格”也会获取！！
		if (input[0] != '\0')
		{
			if (atoi(input) > 0 && (atoi(input)>1000))
			{
				selfID = atoi(input);
				break;
			}
			else
			{
				cout<< "您输入的ID格式不正确，请重新输入！"<<endl;
				cout << "请设置您的ID（ID为大于0的整数）： ";
			}
		}

		//Sleep(1000);
	}

	//创建一个线程，接收玩家输入信息
	std::thread thrdInput(&ReceiveInput);
	thrdInput.detach();
	
	while (true)
	{
		if (!bIsHaveConnect)
		{
			if (connect(sclient, (sockaddr *)(&serAddr), sizeof(sockaddr_in)) == SOCKET_ERROR)
			{
				cout << "连接服务器中。。。。。。" << endl;//<< GetLastError() << endl;
				bIsHaveConnect = false;
				Sleep(1000);
				continue;
			}
			else
			{
				cout << "连接" << SERVER_IP << ":" << SERVER_PORT << " " << "服务器connect成功！！！！" << endl;
				char cResourceID[4] = { 0 };
				itoa(selfID, cResourceID, 10);
				m_qeSendMessage.push(cResourceID);
				cout << "发送信息格式： 自己ID:接收者ID:发送的内容" << endl;
				bIsPrintSendMsgSuccess = false;
				bIsHaveConnect = true;
				continue;
			}
		}

		//每次调用完select后，必须tv会被重置，所以，必须重新赋值！！
		tv.tv_sec = 0;
		tv.tv_usec = 100;
		fdTemp = fdRead;

		//只处理read事件，不过后面还是会有读写消息发送的
		//select成功时返回0， 失败时返回-1 
		nRet = select(1, &fdTemp, NULL, NULL, &tv);	//windows下的select函数的第一个参数是没意义的，linux下为socket描述符的总数（linux下socket描述符也是文件描述符）

		if (nRet == -1)
		{
			cout << "select() error: " << GetLastError() << endl;
			break;
		}
		//else if (nRet == 0)
		//{
		//	//没有连接或者没有读事件
		//	Sleep(100);
		//	continue;
		//}

		if (FD_ISSET(sclient, &fdTemp))
		{
			if (bIsHaveConnect)
			{
				memset(recvData, 0, nSize);
				//接收来自客户端的数据

				ULONG ulFlags = MSG_PARTIAL;
				DWORD dwNumBytesOfRecvd;
				WSABUF	m_wsaBuf;
				m_wsaBuf.buf = recvData;
				m_wsaBuf.len = nMessageWordMaxSize;

				UINT nRet = WSARecv(
					sclient,
					&(m_wsaBuf),
					1,
					&dwNumBytesOfRecvd,// 接收的字节数，异步操作的返回结果一般为0，具体接收到的字节数在完成端口获得
					&(ulFlags),
					NULL,
					NULL);

				if (SOCKET_ERROR == nRet && WSA_IO_PENDING != WSAGetLastError())
				{
					cout << "server " << SERVER_IP << "closed" << endl;
					//如果关闭相关的socket，则必须clear掉相关socket
					FD_CLR(sclient, &fdTemp);
					FD_CLR(sclient, &fdRead);

					//windows套接字不能被关闭多次!!!!!!
					if (sclient != INVALID_SOCKET)
					{
						closesocket(sclient);
						sclient = INVALID_SOCKET;

						sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
						if (sclient == INVALID_SOCKET)
						{
							cout << "创建socket失败！！！！" << endl;
							return 0;
						}

						FD_SET(sclient, &fdRead);
					}

					bIsHaveConnect = false;
				}
				else if (dwNumBytesOfRecvd > 0)
				{
					//前4字节为发送者ID
					char cResourceID[4] = { 0 };
					memcpy(cResourceID, recvData, 4);
					UINT32 nResourceID = atoi(cResourceID);
					char* p = recvData + 4;
					if (nResourceID == selfID)
					{
						cout << "server " << ": " << p << endl;
					}
					else
					{
						cout << nResourceID << ": " << p << endl;
					}
					
				}
				else
				{
					continue;
				}

			}

		}

		if (bIsHaveConnect)
		{
			if (!m_qeSendMessage.empty())
			{
				SendMsgToServer(fdTemp, fdRead, sclient);
			}
		}

		//Sleep(100);

	}

	if (sclient != INVALID_SOCKET)
	{
		closesocket(sclient);
		sclient = INVALID_SOCKET;
	}

	WSACleanup();

    return 0;
}

