#ifndef AMETHYST_DEFINITION_H // definition.h
#define AMETHYST_DEFINITION_H

namespace WINT
{
	typedef unsigned char BYTE;
	typedef unsigned short WORD;
	typedef unsigned int DWORD;
	typedef unsigned long long QWORD;
}

#endif



#ifndef AMETHYST_EXCEPTION_H // exception.h
#define AMETHYST_EXCEPTION_H

namespace EA
{
	class exception
	{
		public:
		static const WINT::BYTE INTERNAL = 0;
		static const WINT::BYTE EXTERNAL = 1;
		WINT::BYTE type;
		WINT::WORD value;
		exception(WINT::BYTE, WINT::WORD);
	};
}

#endif



#ifndef AMETHYST_FILE_H // file.h
#define AMETHYST_FILE_H

#ifdef WIN32 // Windows File API

#include <fileapi.h>
#include <winnt.h>
#include <handleapi.h>
#include <WinBase.h>
#include <windef.h>

#endif

namespace FS // FileSystem
{
	// FileDescriptor
	typedef unsigned __int64 FD;

	static const WINT::DWORD FILE_CLOSED = 3;

	WINT::BYTE create(WINT::BYTE *);
	FS::FD open(WINT::BYTE *, WINT::DWORD);
	FS::FD open(WINT::BYTE *);
	void close(FS::FD);
	WINT::DWORD read(FD, WINT::BYTE *, WINT::DWORD);
	void write(FD, WINT::BYTE *, WINT::DWORD);
	void seek(FD, WINT::QWORD, WINT::DWORD);

	class FIO
	{
		private:
		FS::FD file;
		WINT::BYTE opening = 1;

		public:
		static const WINT::BYTE RD = OF_READ;
		static const WINT::BYTE WT = OF_WRITE;
		static const WINT::BYTE RW = OF_READWRITE;

		explicit FIO(WINT::BYTE *);
		FIO(WINT::BYTE *, WINT::BYTE);
		WINT::DWORD read(WINT::BYTE *, WINT::DWORD) const;
		void write(WINT::BYTE *, WINT::DWORD) const;
		void seek(WINT::QWORD) const;
		void close();
	};
}

#endif



#ifndef AMETHYST_SOCKET_H // ServerSocket.h
#define AMETHYST_SOCKET_H

//******** Win EXTERNAL API *********
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")// LoadLibrary C:\Windows\system32\ws2_32.dll
//*************************************

namespace WSA
{
	const static WINT::WORD SOCKET_CLOSED = 1;
	const static WINT::WORD SOCKET_BOUND  = 2;

	// IP and PORT
	class SocketAddress;
	// Base ServerSocket operation
	class ServerSocket;
	class Socket;

	int startup();
	int cleanup();

	class SocketAddress
	{
		public:
		WINT::BYTE address[4] = {0};
		WINT::WORD ID = 0;

		WINT::DWORD make() const;
		void take(WINT::DWORD);
	};

	class ServerSocket
	{
		private:
		/**
		 * System network handle.
		 * This field is initialized not in constructor but in bind.
		 * @TODO Issue: maybe change it init in constructor future.
		 */
		SOCKET connection = 0;
		WSA::SocketAddress address;
		WINT::BYTE binding = 0; // can bind
		WINT::BYTE opening = 1; // can close
		WINT::WORD backlog = 50;

		public:
		ServerSocket() = default;
		explicit ServerSocket(WINT::DWORD);
		explicit ServerSocket(const WSA::SocketAddress &);
		void bind(const SocketAddress &);
		WSA::Socket accept() const;
		void close();
	};

	class Socket
	{
		public:
		SOCKET connection = 0;
		WSA::SocketAddress address;
		WINT::BYTE opening = 1;

		WINT::DWORD read(WINT::BYTE *, WINT::DWORD) const;
		void write(WINT::BYTE *, WINT::DWORD) const;
		void close();
	};
}

#endif



#include <iostream>

int main()
{
	if (!WSA::startup())
	{
		WSA::ServerSocket ss;
		ss.bind(WSA::SocketAddress());
		WSA::SocketAddress addr;
		addr.take(0x04030201);

		WSA::cleanup();
	}
}



/*------------- implementation for file.h --------------*/

WINT::BYTE FS::create(WINT::BYTE *path)
{
	HANDLE h = CreateFile((LPCSTR)path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	WINT::BYTE success = h != INVALID_HANDLE_VALUE;
	if (success)
	{
		CloseHandle(h);
	}
	return success;
}

FS::FD FS::open(WINT::BYTE *path, WINT::DWORD mode)
{
	FS::create(path);
	OFSTRUCT data;
	HFILE hfVal = OpenFile((LPCSTR)path, &data, mode);
	if (hfVal == HFILE_ERROR)
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
	return (FS::FD)(unsigned int)hfVal;
}

FS::FD FS::open(WINT::BYTE *path)
{
	return FS::open(path, OF_READWRITE);
}

void FS::close(FS::FD fdVal)
{
	if (!CloseHandle((HANDLE)fdVal))
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
}

WINT::DWORD FS::read(FD fdVal, WINT::BYTE *b, WINT::DWORD len)
{
	WINT::DWORD readed;
	if (!::ReadFile((HANDLE)fdVal, b, (::DWORD)len, (LPDWORD)&readed, NULL))
	{
		WINT::DWORD err = GetLastError();
		// EOF
		readed = err == ERROR_BROKEN_PIPE ? -1 : readed;
		if (err != ERROR_BROKEN_PIPE)
		{
			throw EA::exception(EA::exception::INTERNAL, err);
		}
	}
	return readed;
}

void FS::write(FS::FD fdVal, WINT::BYTE *b, WINT::DWORD len)
{
	while (len)
	{
		WINT::DWORD written;
		if (!::WriteFile((HANDLE)fdVal, (LPCVOID)b, (DWORD)len, (LPDWORD)&written, NULL))
		{
			throw EA::exception(EA::exception::INTERNAL, GetLastError());
		}
		len -= written;
	}
}

void FS::seek(FD fdVal, WINT::QWORD offset, WINT::DWORD mode)
{
	LARGE_INTEGER distance;
	distance.QuadPart = (long long) offset;
	if (!SetFilePointerEx((HANDLE)fdVal, distance, NULL, (DWORD)mode))
	{
		throw EA::exception(EA::exception::EXTERNAL, GetLastError());
	}
}

FS::FIO::FIO(WINT::BYTE *path): FIO(path, FS::FIO::RW)
{
}

FS::FIO::FIO(WINT::BYTE *path, WINT::BYTE mode): file(FS::open(path, mode))
{
}

WINT::DWORD FS::FIO::read(WINT::BYTE *b, WINT::DWORD len) const
{
	if (this->opening)
	{
		return FS::read(this->file, b, len);
	}
	throw EA::exception(EA::exception::EXTERNAL, FS::FILE_CLOSED);
}

void FS::FIO::write(WINT::BYTE *b, WINT::DWORD len) const
{
	if (this->opening)
	{
		FS::write(this->file, b, len);
	}
	throw EA::exception(EA::exception::EXTERNAL, FS::FILE_CLOSED);
}

void FS::FIO::seek(WINT::QWORD offset) const
{
	FS::seek(this->file, offset, FILE_BEGIN);
}

void FS::FIO::close()
{
	if (this->opening)
	{
		FS::close(this->file);
		this->opening = 0;
	}
}



/*------------- implementation for ServerSocket.h -------------*/

int WSA::startup()
{
	WSADATA data;
	int err = WSAStartup(0x0202, &data);
	if (err)
	{
		std::cout << "Can't startup EXTERNAL: " << err;
	}
	return err;
}

int WSA::cleanup()
{
	int err = 0;
	if (WSACleanup() == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		std::cout << "Error occurred when cleanup EXTERNAL: ";
		switch (err)
		{
			case WSANOTINITIALISED:
			{
				std::cout << "A successful WSAStartup call must occur before using any Windows Sockets function.";
				break;
			}
			case WSAENETDOWN:
			{
				std::cout << "The network subsystem has failed.";
				break;
			}
			case WSAEINPROGRESS:
			{
				std::cout << "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
				break;
			}
		}
		std::cout << std::endl;
		WSASetLastError(err);
	}
	return err;
}

EA::exception::exception(WINT::BYTE type, WINT::WORD value): type(type), value(value)
{
}

WINT::DWORD WSA::SocketAddress::make() const
{
	WINT::DWORD val = (this->address[0] << 24) | (this->address[1] << 16) | (this->address[2] << 8) | (this->address[3]);
	WINT::DWORD v = 1;
	BYTE *p = (BYTE*) &v;
	if (p[0])
	{
		val = htonl(val);
	}
	return val;
}
void WSA::SocketAddress::take(WINT::DWORD addr)
{
	WINT::DWORD v = 1;
	BYTE *p = (BYTE*) &v;
	if (p[0])
	{
		addr = ntohl(addr);
	}
	this->address[0] = (addr >> 24) & 0xFF;
	this->address[1] = (addr >> 16) & 0xFF;
	this->address[2] = (addr >> 8) & 0xFF;
	this->address[3] = addr & 0xFF;
}

WSA::ServerSocket::ServerSocket(const WSA::SocketAddress &address)
{
	this->bind(address);
}
WSA::ServerSocket::ServerSocket(WINT::DWORD port)
{
	WSA::SocketAddress addr;
	addr.ID = port;
	this->bind(addr);
}
void WSA::ServerSocket::bind(const WSA::SocketAddress &endpoint)
{
	this->opening ? void() : throw EA::exception(EA::exception::EXTERNAL, WSA::SOCKET_CLOSED);
	this->binding ? throw EA::exception(EA::exception::EXTERNAL, WSA::SOCKET_BOUND) : void();

	this->address = endpoint;

	// Create an unbound socket
	this->connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	(this->connection == INVALID_SOCKET) ? throw EA::exception(EA::exception::INTERNAL, WSAGetLastError()) : void();

	// bind this socket to address
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(endpoint.ID);
	addr.sin_addr.S_un.S_addr = endpoint.make();
	int err = ::bind(this->connection, (SOCKADDR *)&addr, sizeof(SOCKADDR_IN));
	err ? throw EA::exception(EA::exception::INTERNAL, WSAGetLastError()) : void();

	// Set socket in listen
	err = listen(this->connection, this->backlog);
	err ? throw EA::exception(EA::exception::INTERNAL, WSAGetLastError()) : void();
	this->binding = true;
}

WSA::Socket WSA::ServerSocket::accept() const
{
	SOCKADDR_IN addr;
	int len = sizeof(SOCKADDR_IN);

	SOCKET conn = ::accept(this->connection, (SOCKADDR *)&addr, &len);
	WSA::Socket sock;
	sock.connection = conn;
	sock.address.take(addr.sin_addr.S_un.S_addr);

	return sock;
}

void WSA::ServerSocket::close()
{
	int err = closesocket(this->connection);
	if (err)
	{
		throw EA::exception(EA::exception::INTERNAL, WSAGetLastError());
	}
	this->opening = 0;
}

WINT::DWORD WSA::Socket::read(WINT::BYTE *b, WINT::DWORD len) const
{
	this->opening ? void() : throw EA::exception(EA::exception::EXTERNAL, WSA::SOCKET_CLOSED);
	return FS::read((FS::FD)this->connection, b, len);
}

void WSA::Socket::write(WINT::BYTE *b, WINT::DWORD len) const
{
	this->opening ? void() : throw EA::exception(EA::exception::EXTERNAL, WSA::SOCKET_CLOSED);
	FS::write((FS::FD)this->connection, b, len);
}

void WSA::Socket::close()
{
	if (this->opening)
	{
		if (closesocket(this->connection))
		{
			throw EA::exception(EA::exception::INTERNAL, WSAGetLastError());
		}
	}
	this->opening = 0;
}
