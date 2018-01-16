#include "StdAfx.h"
#include "Chat.h"
#include "Lab2App.h"

const WORD CHAT_PORT = ::htons(5555);
const DWORD MAX_DATAGRAM_SIZE = 0x10000;
const DWORD USER_IN_LIST_TIMEOUT = 10000;	// 10 sec

CChat::CChat(const CString & nickname, bool bMulticast, DWORD dwMulticastAddr)
	: m_hMessagesMutex(::CreateMutex(nullptr, FALSE, nullptr))
	, m_hOnlineUsersMutex(::CreateMutex(nullptr, FALSE, nullptr))
	, m_hRecvThreadTerminationEvent(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
	, m_hSendThreadTerminationEvent(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
	, m_hOnlineRefresherThreadTerminationEvent(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
	, m_hNewMessageEvent(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
	, m_hSendThread(nullptr)
	, m_hRecvThread(nullptr)
	, m_sendSock(INVALID_SOCKET)
	, m_recvSock(INVALID_SOCKET)
	, m_nickname(nickname)
{
	ZeroMemory(&m_destinationAddr, sizeof m_destinationAddr);
	m_destinationAddr.sin_family = AF_INET;
	m_destinationAddr.sin_port = CHAT_PORT;
	m_destinationAddr.sin_addr.s_addr = bMulticast? dwMulticastAddr : INADDR_BROADCAST;

	m_sendSock = CreateSocket(bMulticast, dwMulticastAddr);
	if (INVALID_SOCKET == m_sendSock)
		return;

	m_recvSock = CreateSocket(bMulticast, dwMulticastAddr);
	if (INVALID_SOCKET == m_recvSock)
		return;

	m_hSendThread = (HANDLE)_beginthread(SendThreadHelperProc, 0, this);
	m_hRecvThread = (HANDLE)_beginthread(RecvThreadHelperProc, 0, this);
	m_hOnlineRefresherThread = (HANDLE)_beginthread(OnlineRefresherThreadHelperProc, 0, this);
}

CChat::~CChat()
{
	if (m_hRecvThread)
	{
		::SetEvent(m_hSendThreadTerminationEvent);
		::SetEvent(m_hRecvThreadTerminationEvent);
		::SetEvent(m_hOnlineRefresherThreadTerminationEvent);

		::WaitForSingleObject(m_hRecvThread, INFINITE);
		::WaitForSingleObject(m_hSendThread, INFINITE);
		::WaitForSingleObject(m_hOnlineRefresherThread, INFINITE);
	}

	::CloseHandle(m_hSendThreadTerminationEvent);
	::CloseHandle(m_hRecvThreadTerminationEvent);
	::CloseHandle(m_hOnlineRefresherThreadTerminationEvent);

	::CloseHandle(m_hMessagesMutex);
	::CloseHandle(m_hOnlineUsersMutex);
	::CloseHandle(m_hNewMessageEvent);

	if (INVALID_SOCKET != m_sendSock)
		::closesocket(m_sendSock);

	if (INVALID_SOCKET != m_recvSock)
		::closesocket(m_recvSock);
}

void CChat::Send(const CString & message)
{
	::WaitForSingleObject(m_hMessagesMutex, INFINITE);
	m_messagesQueue.push_back(message);
	::ReleaseMutex(m_hMessagesMutex);
	::SetEvent(m_hNewMessageEvent);
}

void CChat::SendThread()
{
	HANDLE waitObjects[] = { m_hSendThreadTerminationEvent, m_hNewMessageEvent };
	DWORD dwWaitResult = WAIT_TIMEOUT;
	while (true)
	{
		if (dwWaitResult == WAIT_OBJECT_0 + 1)
		{
			MessagesVector messages;
			::WaitForSingleObject(m_hMessagesMutex, INFINITE);
			messages = m_messagesQueue;
			m_messagesQueue.clear();
			::ReleaseMutex(m_hMessagesMutex);
			for (MessagesVector::iterator it = messages.begin(); it != messages.end(); ++it)
				SendImpl(*it);
		}
		else if (dwWaitResult == WAIT_TIMEOUT)
			SendImpl(CString());
		else
			return;
		dwWaitResult = ::WaitForMultipleObjects(2, waitObjects, FALSE, USER_IN_LIST_TIMEOUT / 2);
	}
}

void CChat::SendImpl(const CString & message)
{
	const size_t nicknameSize = wcslen(m_nickname.GetString()) + 1;
	const size_t messageSize = wcslen(message.GetString()) + 1;
	std::vector<wchar_t> sendMessage(nicknameSize + messageSize);
	const char * pBuffer = (const char *)&sendMessage[0];
	wcsncpy_s(&sendMessage[0], nicknameSize, m_nickname.GetString(), nicknameSize);
	wcsncpy_s(&sendMessage[0] + nicknameSize, messageSize, message.GetString(), messageSize);

	int nSent = ::sendto(m_sendSock, pBuffer, sendMessage.size() * sizeof(wchar_t),
						 0, (const sockaddr *)&m_destinationAddr, sizeof m_destinationAddr);
	if (nSent < 0)
	{
		::AfxMessageBox(L"Failed to ::send");
		::PostQuitMessage(-1);
	}
}

void CChat::RecvThread()
{
	HANDLE hRecvEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (SOCKET_ERROR == ::WSAEventSelect(m_recvSock, hRecvEvent, FD_READ))
	{
		::AfxMessageBox(L"Error ::WSAEventSelect");
		::PostQuitMessage(-1);
		return;
	}

	HANDLE waitEvents[] = { hRecvEvent, m_hRecvThreadTerminationEvent };

	while (true)
	{
		std::vector<wchar_t> data(MAX_DATAGRAM_SIZE / sizeof(wchar_t));
		char * recvBuffer = (char *)&data[0];
		SOCKADDR_IN fromAddr = { 0 };
		fromAddr.sin_family = AF_INET;
		int fromLen = sizeof fromAddr;
		int nRead = ::recvfrom(m_recvSock, recvBuffer, MAX_DATAGRAM_SIZE, 0, (sockaddr *)&fromAddr, &fromLen);
		if (nRead <= 0)
		{
			DWORD dwWait = ::WaitForMultipleObjects(2, waitEvents, FALSE, INFINITE);
			if (dwWait == WAIT_OBJECT_0)
				continue;
			else
				break;
		}

		size_t nChars = nRead / 2;
		data[nChars] = L'\0';
		OnRecved(data, fromAddr);
	}
}

void CChat::OnlineRefresherThread()
{
	while (WAIT_TIMEOUT == ::WaitForSingleObject(m_hOnlineRefresherThreadTerminationEvent, USER_IN_LIST_TIMEOUT / 5))
	{
		UsersTimeMap updatedMap;
		std::vector<CString> users;
		::WaitForSingleObject(m_hOnlineUsersMutex, INFINITE);
		DWORD dwCurrentTime = ::GetTickCount();
		for (UsersTimeMap::iterator it = m_onlineUsersMap.begin(); it != m_onlineUsersMap.end(); ++it)
		{
			if (dwCurrentTime - it->second > USER_IN_LIST_TIMEOUT)
				continue;

			updatedMap.insert(*it);
			users.push_back(it->first.ToString());
		}
		m_onlineUsersMap = updatedMap;
		::ReleaseMutex(m_hOnlineUsersMutex);
		theApp.UpdateNicknameList(users);
	}
}

void CChat::OnRecved(const std::vector<wchar_t> & data, const SOCKADDR_IN & sender)
{
	CString nickname = &data[0];
	CString message = (&data[0] + wcslen(nickname.GetString()) + 1);
	wchar_t ip[20];
	::InetNtopW(AF_INET, (PVOID)&sender.sin_addr, ip, 20);
	AddOnlineUser(COnlineUserInfo(nickname, ip));
	if (!message.IsEmpty())
		theApp.AddMessage(nickname + "[" + ip + "]: " + message);
}

void CChat::AddOnlineUser(const COnlineUserInfo & info)
{
	::WaitForSingleObject(m_hOnlineUsersMutex, INFINITE);
	m_onlineUsersMap[info] = GetTickCount();
	::ReleaseMutex(m_hOnlineUsersMutex);
}

SOCKET CChat::CreateSocket(bool bMulticast, DWORD dwMulticastAddr) const
{
	SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == sock)
	{
		::AfxMessageBox(L"Failed ::socket");
		return sock;
	}

	BOOL bReuseAddr = TRUE;
	BOOL bBroadcast = TRUE;
	if (SOCKET_ERROR == ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&bReuseAddr, sizeof bReuseAddr)
		|| SOCKET_ERROR == ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&bBroadcast, sizeof bBroadcast))
	{
		::closesocket(sock);
		::AfxMessageBox(L"Failed ::setsockopt SO_REUSEADDR | SO_BROADCAST");
		return INVALID_SOCKET;
	}

	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = CHAT_PORT;

	if (SOCKET_ERROR == ::bind(sock, (const sockaddr *)&addr, sizeof addr))
	{
		::closesocket(sock);
		::AfxMessageBox(L"Failed ::bind");
		return INVALID_SOCKET;
	}
	
	if (!bMulticast)
		return sock;

	IP_MREQ mreq = { 0 };
	mreq.imr_interface.s_addr = INADDR_ANY;
	mreq.imr_multiaddr.s_addr = dwMulticastAddr;
	DWORD dwMulticastIf = INADDR_ANY;
	if (SOCKET_ERROR == ::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof mreq)
		|| SOCKET_ERROR == ::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&dwMulticastIf, sizeof dwMulticastIf))
	{
		::closesocket(sock);
		::AfxMessageBox(L"Failed ::setsockopt IP_ADD_MEMBERSHIP");
		return INVALID_SOCKET;
	}

	return sock;
}

void SendThreadHelperProc(void * pThis)
{
	((CChat *)pThis)->SendThread();
}

void RecvThreadHelperProc(void * pThis)
{
	((CChat *)pThis)->RecvThread();
}

void OnlineRefresherThreadHelperProc(void * pThis)
{
	((CChat *)pThis)->OnlineRefresherThread();
}

COnlineUserInfo::COnlineUserInfo(const CString & name, const CString & ip)
	: m_name(name)
	, m_ip(ip)
{
}

bool COnlineUserInfo::operator==(const COnlineUserInfo & other) const
{
	return m_name == other.m_name;
}

bool COnlineUserInfo::operator<(const COnlineUserInfo & other) const
{
	return m_name.Collate(other.m_name) < 0;
}

CString COnlineUserInfo::ToString() const
{
	return m_name + L"[" + m_ip + L"]";
}
