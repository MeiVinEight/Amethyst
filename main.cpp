// ************* previous declarations *************

#ifndef AMETHYST_DEFINITION_H // definition.h
#define AMETHYST_DEFINITION_H

#include <Windows.h>
#include <new.h>

typedef unsigned long long QWORD;

#endif



#ifndef AMETHYST_MEMORY_H // memory.h
#define AMETHYST_MEMORY_H

namespace Memory
{
	void *allocate(QWORD);
	void free(void *);
	void copy(void *, void *, QWORD);
}

#endif



#ifndef AMETHYST_THREAD_H // thread.h
#define AMETHYST_THREAD_H

namespace MT
{
	DWORD start(LPVOID);

	class thread
	{
		public:
		DWORD ID = 0;
		HANDLE object = 0;

		thread(DWORD (*)(LPVOID), LPVOID);
		explicit thread(HANDLE);
		static MT::thread current();
		BYTE alive() const;
		void close() const;
	};
}

#endif



#ifndef AMETHYST_EXCEPTION_H // exception.h
#define AMETHYST_EXCEPTION_H

namespace EA
{
	class exception
	{
		public:
		static const BYTE INTERNAL = 0;
		static const BYTE EXTERNAL = 1;
		BYTE type;
		WORD value;
		exception(BYTE, WORD);
	};
}

#endif



#ifndef AMETHYST_FILE_H // file.h
#define AMETHYST_FILE_H
// FileSystem
namespace FS
{
	// FileDescriptor
	typedef void* FD;

	static const QWORD EOF = -1;
	static const DWORD FILE_CLOSED = 3;

	BYTE create(LPCSTR);
	FS::FD open(LPCSTR, DWORD);
	FS::FD open(LPCSTR);
	void close(FS::FD);
	BYTE exist(LPCSTR);
	DWORD read(FD, BYTE *, DWORD);
	void write(FD, BYTE *, DWORD);
	void seek(FD, QWORD, DWORD);

	class BIO
	{
		public:
		virtual DWORD read(BYTE *, DWORD) = 0;
		virtual void write(BYTE *, DWORD) = 0;
		/**
		 * @TODO unread support
		 */
	};
	class FIO: public BIO
	{
		private:
		FS::FD file;
		BYTE opening = 1;

		public:
		static const BYTE RD = OF_READ;
		static const BYTE WT = OF_WRITE;
		static const BYTE RW = OF_READWRITE;

		explicit FIO(LPCSTR);
		explicit FIO(FS::FD);
		FIO(LPCSTR, BYTE);
		DWORD read(BYTE *, DWORD) override;
		void write(BYTE *, DWORD) override;
		void seek(QWORD) const;
		void close();
	};
}

#endif



#ifndef AMETHYST_SOCKET_H // ServerSocket.h
#define AMETHYST_SOCKET_H

//******** Win EXTERNAL API *********
#ifndef SOCK_STREAM
#include <WinSock2.h>
#endif

#pragma comment(lib, "ws2_32.lib")// LoadLibrary C:\Windows\system32\ws2_32.dll
//*************************************

namespace WSA
{
	const static WORD SOCKET_CLOSED = 1;
	const static WORD SOCKET_BOUND  = 2;

	// IP and PORT
	class SocketAddress;
	// Base ServerSocket operation
	class ServerSocket;
	class Socket;

	void startup();
	void cleanup();

	class SocketAddress
	{
		public:
		BYTE address[4] = {0};
		WORD ID = 0;

		DWORD make() const;
		void take(DWORD);
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
		BYTE binding = 0; // can bind
		BYTE opening = 1; // can close
		WORD backlog = 50;

		public:
		ServerSocket() = default;
		explicit ServerSocket(DWORD);
		explicit ServerSocket(const WSA::SocketAddress &);
		void bind(const SocketAddress &);
		WSA::Socket accept() const;
		void close();
	};
	class Socket: public FS::BIO
	{
		public:
		SOCKET connection = 0;
		WSA::SocketAddress address;
		BYTE opening = 1;

		DWORD read(BYTE *, DWORD) override;
		void write(BYTE *, DWORD) override;
		void close();
	};
}

#endif

// ************* previous declarations *************

namespace Hexadecimal
{
	static const BYTE H[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	void transform(QWORD num, BYTE *data)
	{
		for (int i = 0; i < 16; i++)
		{
			data[15 - i] = H[num & 0xFF];
			num >>= 4;
		}
	}
}
class String: public FS::BIO
{
	public:
	BYTE *string;
	QWORD length;
	QWORD memory;
	QWORD position = 0;
	String(): string((BYTE *)Memory::allocate(16)), length(0), memory(16)
	{
	}
	String(const String &o): length(o.length), string((BYTE *)Memory::allocate(o.length)), memory(o.length)
	{
		Memory::copy(o.string, this->string, this->length);
	}
	String(String &&o): length(o.length), string(o.string), memory(o.memory)
	{
		o.string = NULL;
		o.length = 0;
	}
	~String()
	{
		if (this->string)
		{
			Memory::free(this->string);
			this->string = NULL;
		}
		this->length = 0;
		this->memory = 0;
	}
	String &operator=(String &&o)
	{
		if (this->string)
		{
			Memory::free(this->string);
		}
		this->string = o.string;
		this->length = o.length;
		this->memory = o.memory;
		this->position = o.position;
		o.string = NULL;
		o.length = 0;
		o.memory = 0;
		o.position = 0;
		return *this;
	}
	DWORD read(BYTE *b, DWORD len) override
	{
		QWORD available = this->length - this->position;
		len = (DWORD)(len < available ? len : available);
		Memory::copy(this->string + this->position, b, len);
		this->position += len;
		return len;
	}
	void write(BYTE *b, DWORD len) override
	{
		if (this->memory < this->length + len)
		{
			QWORD newLen = this->memory;
			while (newLen < this->length + len)
			{
				newLen >>= 1;
			}
			BYTE *newArr = (BYTE *) Memory::allocate(newLen);
			Memory::copy(this->string, newArr, this->length);
			Memory::free(this->string);
			this->string = newArr;
			this->memory = newLen;
		}
		Memory::copy(b, this->string + this->length, len);
		this->length += len;
	}
	BYTE equals(LPCSTR str) const
	{
		LPCSTR p = str;
		while (*p++);
		QWORD len = p - str - 1;
		if (this->length == len)
		{
			return String::equals((LPCSTR)this->string, str, len);
		}
		return false;
	}
	static BYTE equals(LPCSTR s1, LPCSTR s2, QWORD length)
	{
		while (length--)
		{
			if (s1[length] != s2[length])
			{
				return 0;
			}
		}
		return 1;
	}
};
namespace Amethyst
{
	BYTE space(BYTE c);
	DWORD console(LPVOID param);
	DWORD connection(LPVOID);

	class scanner
	{
		public:
		FS::BIO *stream = NULL;
		explicit scanner();
		String text() const;
		String string() const;
		QWORD number() const;
	};

	BYTE space(BYTE c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}
	DWORD console(LPVOID param)
	{
		LPVOID *ps = (LPVOID *)param;
		BYTE &running = *((BYTE*)ps[1]);

		FS::FD fdVal = GetStdHandle(STD_INPUT_HANDLE);
		FS::FIO file(fdVal);
		Amethyst::scanner scn;
		scn.stream = &file;
		while (running)
		{
			String str = (String &&)scn.text();
			if (String::equals((LPCSTR)str.string, "stop", 4))
			{
				running = 0;
			}
		}
		return 0;
	}
	DWORD connection(LPVOID param)
	{
		MT::thread *t = (MT::thread *)((LPVOID *)param)[0];
		WSA::Socket *sock = (WSA::Socket *)((LPVOID *)param)[1];

		while (sock->opening)
		{
			Amethyst::scanner scn;
			Amethyst::scanner strScn;
			scn.stream = sock;

			QWORD length = 0;

			String str = (String &&)scn.text();
			strScn.stream = &str;
			String method = (String &&)strScn.string();
			String url = (String &&)strScn.string();

			// read request header
			str = (String &&)scn.text();
			str.length--;
			while (str.length)
			{
				strScn.stream = &str;
				String field = (String &&)strScn.string();
				field.length--;
				if (field.equals("Content-Length"))
				{
					length = strScn.number();
				}
			}

			BYTE *buf = (BYTE *)Memory::allocate(length);
			for (QWORD readed = 0; readed < length; readed += sock->read(buf, length - readed));
			Memory::free(buf);

			if (method.equals("GET"))
			{
				if (FS::exist((LPCSTR)(url.string + 1)))
				{
				}
			}
		}

		/*
		 * @TODO free not here but out of thread
		 */
		Memory::free(sock);
		return 0;
	}
	Amethyst::scanner::scanner() = default;
	String Amethyst::scanner::text() const
	{
		String str;
		BYTE c;
		while (this->stream->read(&c, 1) && c != '\n')
		{
			str.write(&c, 1);
		}
		return str;
	}
	String Amethyst::scanner::string() const
	{
		String str;
		BYTE c;
		while (this->stream->read(&c, 1) && Amethyst::space(c));
		while (!Amethyst::space(c))
		{
			str.write(&c, 1);
			if (!this->stream->read(&c, 1))
			{
				break;
			}
		}
		return str;
	}
	QWORD Amethyst::scanner::number() const
	{
		BYTE c;
		while (this->stream->read(&c, 1) && Amethyst::space(c));
		QWORD num = 0;
		while (!Amethyst::space(c))
		{
			num *= 10;
			num += c - '0';
			if (!this->stream->read(&c, 1))
			{
				break;
			}
		}
		return num;
	}
}

int main()
{
	WSA::startup();

	WSA::ServerSocket ss = WSA::ServerSocket(80);


	BYTE running = 1;
	MT::thread consoleListener = MT::thread(&Amethyst::console, &running);

	while (running)
	{
		WSA::Socket *sock = (WSA::Socket *)Memory::allocate(sizeof(WSA::Socket));
		new(sock) WSA::Socket();
		*sock = ss.accept();
		MT::thread *conn = (MT::thread *)Memory::allocate(sizeof(MT::thread));
		new(conn) MT::thread(Amethyst::connection, sock);
	}

	consoleListener.close();

	WSA::cleanup();
}



/*------------- implementation for memory.h --------------*/

void *Memory::allocate(QWORD size)
{
	HANDLE heap = GetProcessHeap();
	heap ? void() : throw EA::exception(EA::exception::INTERNAL, GetLastError());
	return HeapAlloc(heap, 0, size);
}
void Memory::free(void *p)
{
	HANDLE heap = GetProcessHeap();
	heap ? void() : throw EA::exception(EA::exception::INTERNAL, GetLastError());
	HeapFree(heap, 0, p);
}
void Memory::copy(void *from, void *to, QWORD size)
{
	BYTE *f = (BYTE *)from;
	BYTE *t = (BYTE *)to;
	while (size--)
	{
		t[size] = f[size];
	}
}



/*------------- implementation for thread.h --------------*/

/**
 * @param param is an array of data
 *
 * param layout: [DWORD (*)(LPVOID), MT::thread *, LPVOID]
 * @return
 */
DWORD MT::start(LPVOID param)
{
	LPVOID *ps = (LPVOID *)param;
	DWORD (*entry)(LPVOID) = (DWORD (*)(LPVOID))ps[0];
	MT::thread *t = (MT::thread *)ps[1];

	DWORD ret = entry(ps + 1);
	Memory::free(param);
	t->close();
	return ret;
}
MT::thread::thread(DWORD (*proc)(LPVOID), LPVOID param)
{
	LPVOID *ps = (LPVOID *)Memory::allocate(3 * sizeof(LPVOID));
	ps[0] = (LPVOID)proc;
	ps[1] = this;
	ps[2] =param;
	this->object = CreateThread(NULL, 0, &start, ps, 0, &this->ID);
	if (!this->object)
	{
		Memory::free(ps);
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
}
MT::thread::thread(HANDLE obj): object(obj), ID(GetThreadId(obj))
{
	if (!this->ID)
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
}
MT::thread MT::thread::current()
{
	if (HANDLE obj = OpenThread(THREAD_ALL_ACCESS, TRUE, GetCurrentThreadId()))
	{
		return MT::thread(obj);
	}
	throw EA::exception(EA::exception::INTERNAL, GetLastError());
}
BYTE MT::thread::alive() const
{
	DWORD code;
	if (!GetExitCodeThread(this->object, &code))
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
	return code != STILL_ACTIVE;
}
void MT::thread::close() const
{
	if (!CloseHandle(this->object))
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
}


/*------------- implementation for file.h --------------*/

BYTE FS::create(LPCSTR path)
{
	HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	BYTE success = h != INVALID_HANDLE_VALUE;
	if (success)
	{
		CloseHandle(h);
	}
	return success;
}
FS::FD FS::open(LPCSTR path, DWORD mode)
{
	FS::create(path);
	OFSTRUCT data;
	HFILE hfVal = OpenFile(path, &data, mode);
	if (hfVal == HFILE_ERROR)
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
	return (FS::FD)(QWORD)(DWORD)hfVal;
}
FS::FD FS::open(LPCSTR path)
{
	return FS::open(path, OF_READWRITE);
}
void FS::close(FS::FD fdVal)
{
	if (!CloseHandle(fdVal))
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
}
BYTE FS::exist(LPCSTR path)
{
	OFSTRUCT data;
	if (OpenFile(path, &data, OF_EXIST) == HFILE_ERROR)
	{
		return 0;
	}
	return 1;
}
DWORD FS::read(FD fdVal, BYTE *b, DWORD len)
{
	DWORD readed;
	if (!::ReadFile(fdVal, b, len, &readed, NULL))
	{
		throw EA::exception(EA::exception::INTERNAL, GetLastError());
	}
	return readed;
}
void FS::write(FS::FD fdVal, BYTE *b, DWORD len)
{
	while (len)
	{
		DWORD written;
		if (!::WriteFile(fdVal, b, len, &written, NULL))
		{
			throw EA::exception(EA::exception::INTERNAL, GetLastError());
		}
		len -= written;
		b += written;
	}
}
void FS::seek(FD fdVal, QWORD offset, DWORD mode)
{
	LARGE_INTEGER distance;
	distance.QuadPart = (long long) offset;
	if (!SetFilePointerEx(fdVal, distance, NULL, mode))
	{
		throw EA::exception(EA::exception::EXTERNAL, GetLastError());
	}
}
FS::FIO::FIO(LPCSTR path): FIO(path, FS::FIO::RW)
{
}
FS::FIO::FIO(FS::FD fdVal): file(fdVal)
{
}
FS::FIO::FIO(LPCSTR path, BYTE mode): file(FS::open(path, mode))
{
}
DWORD FS::FIO::read(BYTE *b, DWORD len)
{
	if (this->opening)
	{
		return FS::read(this->file, b, len);
	}
	throw EA::exception(EA::exception::EXTERNAL, FS::FILE_CLOSED);
}
void FS::FIO::write(BYTE *b, DWORD len)
{
	if (this->opening)
	{
		FS::write(this->file, b, len);
	}
	throw EA::exception(EA::exception::EXTERNAL, FS::FILE_CLOSED);
}
void FS::FIO::seek(QWORD offset) const
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

void WSA::startup()
{
	WSADATA data;
	int err = WSAStartup(0x0202, &data);
	if (err)
	{
		throw EA::exception(EA::exception::INTERNAL, err);
	}
}
void WSA::cleanup()
{
	int err = 0;
	if (WSACleanup() == SOCKET_ERROR)
	{
		throw EA::exception(EA::exception::INTERNAL, WSAGetLastError());
	}
}
EA::exception::exception(BYTE type, WORD value): type(type), value(value)
{
}
DWORD WSA::SocketAddress::make() const
{
	DWORD val = (this->address[0] << 24) | (this->address[1] << 16) | (this->address[2] << 8) | (this->address[3]);
	DWORD v = 1;
	BYTE *p = (BYTE*) &v;
	if (p[0])
	{
		val = htonl(val);
	}
	return val;
}
void WSA::SocketAddress::take(DWORD addr)
{
	DWORD v = 1;
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
WSA::ServerSocket::ServerSocket(DWORD port)
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
	sock.address.ID = addr.sin_port;

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
DWORD WSA::Socket::read(BYTE *b, DWORD len)
{
	this->opening ? void() : throw EA::exception(EA::exception::EXTERNAL, WSA::SOCKET_CLOSED);
	DWORD readed = recv(this->connection, (char *)b, len, 0);
	if (readed == (DWORD)SOCKET_ERROR)
	{
		throw EA::exception(EA::exception::INTERNAL, WSAGetLastError());
	}
	readed ? void() : this->close();
	return readed;
}
void WSA::Socket::write(BYTE *b, DWORD len)
{
	this->opening ? void() : throw EA::exception(EA::exception::EXTERNAL, WSA::SOCKET_CLOSED);
	while (len)
	{
		DWORD sended = send(this->connection, (char *)b, len, 0);
		if (sended == (DWORD)SOCKET_ERROR)
		{
			DWORD err = WSAGetLastError();
			if (err != WSAECONNABORTED)
			{
				throw EA::exception(EA::exception::INTERNAL, err);
			}
			this->close();
			return;
		}
		len -= sended;
		b += sended;
	}
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
