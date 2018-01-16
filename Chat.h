#pragma once

class COnlineUserInfo
{
public:
	COnlineUserInfo(const CString & name, const CString & ip);
	bool operator==(const COnlineUserInfo & other) const;
	bool operator<(const COnlineUserInfo & other) const;
	CString ToString() const;
	CString m_name;
	CString m_ip;
};

class CChat
{
public:
	CChat(const CString & nickname, bool bMulticast, DWORD dwMulticastAddr);
	~CChat();

	void Send(const CString & message);

private:
	friend void SendThreadHelperProc(void * pThis);
	friend void RecvThreadHelperProc(void * pThis);
	friend void OnlineRefresherThreadHelperProc(void * pThis);

	void SendThread();
	void SendImpl(const CString & message);
	void RecvThread();
	void OnlineRefresherThread();

	void OnRecved(const std::vector<wchar_t> & data, const SOCKADDR_IN & sender);
	void AddOnlineUser(const COnlineUserInfo & onlineUsers);

	SOCKET CreateSocket(bool bMulticast, DWORD dwMulticastAddr) const;

	CString m_nickname;

	SOCKET m_sendSock;
	SOCKET m_recvSock;

	typedef std::vector<CString> MessagesVector;
	MessagesVector m_messagesQueue;
	HANDLE m_hMessagesMutex;
	HANDLE m_hNewMessageEvent;

	typedef std::map<COnlineUserInfo, DWORD> UsersTimeMap;
	UsersTimeMap m_onlineUsersMap;
	HANDLE m_hOnlineUsersMutex;

	HANDLE m_hOnlineRefresherThread;
	HANDLE m_hOnlineRefresherThreadTerminationEvent;

	HANDLE m_hSendThreadTerminationEvent;
	HANDLE m_hSendThread;
	
	HANDLE m_hRecvThreadTerminationEvent;
	HANDLE m_hRecvThread;

	SOCKADDR_IN m_destinationAddr;
};

