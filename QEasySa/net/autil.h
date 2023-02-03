#pragma once
#ifndef AUTIL_H
#define AUTIL_H
#include "../stdafx.h"
#include <QReadWriteLock>

constexpr auto SLICE_MAX = 20;
constexpr auto SLICE_SIZE = 65500;
constexpr auto NETBUFSIZ = 1024 * 64;
constexpr auto MAX_BUFFER = 32768;
constexpr auto MAX_SMALLBUFF = 16384;

constexpr auto DEFAULTTABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}";
constexpr auto DEFAULTFUNCBEGIN = "&";
constexpr auto DEFAULTFUNCEND = "#";
constexpr auto SEPARATOR = ";";

class KeyManager
{
	DISABLE_COPY_MOVE(KeyManager);

public:
	virtual ~KeyManager() { m_key.clear(); }
	void __vectorcall SetKey(const std::string& key)
	{
		EncryptKey(key);
	}
	std::string __vectorcall GetKey() const
	{
		return DecryptKey();
	}
	bool __vectorcall isValid() const
	{
		return !m_key.empty();
	}

	bool __vectorcall isValid(const std::string& key) const
	{
		return !key.empty();
	}


private:
	KeyManager() = default;

	//key加密
	void __vectorcall EncryptKey(const std::string& key)
	{
		std::string encryptedKey = key;
		for (size_t i = 0; i < key.length(); i++)
		{
			encryptedKey[i] = encryptedKey[i] << 1 ^ encryptedKey[i] >> 7;
			encryptedKey[i] = encryptedKey[i] ^ (encryptedKey[i] & 0xff) >> 4;
			encryptedKey[i] = (encryptedKey[i] & 0xff) << 4 | (encryptedKey[i] & 0xff) >> 4;
		}
		//比較
		if (encryptedKey != m_key)
		{
			m_key = encryptedKey;
		}
	}
	//key解密
	std::string __vectorcall DecryptKey() const
	{
		std::string decryptedKey = m_key;
		for (size_t i = 0; i < decryptedKey.length(); i++)
		{
			decryptedKey[i] = (decryptedKey[i] & 0xff) << 4 | (decryptedKey[i] & 0xff) >> 4;
			decryptedKey[i] = decryptedKey[i] ^ (decryptedKey[i] & 0xff) >> 4;
			decryptedKey[i] = (decryptedKey[i] >> 1) ^ (decryptedKey[i] << 7);
		}
		return decryptedKey;
	}
	std::string m_key;
};


class Autil
{
	DISABLE_COPY_MOVE(Autil);
public:
	char MesgSlice[sizeof(char*) * SLICE_MAX][SLICE_SIZE];
	int SliceCount;
public:
	Autil();
	virtual ~Autil();
	char* __vectorcall index(const char* table, char src);
	void __vectorcall util_Init(void);
	void __vectorcall util_Release(void);
	void __vectorcall util_SplitMessage(char* source, const char* separator);
	void __vectorcall util_EncodeMessage(char* dst, char* src);
	void __vectorcall util_DecodeMessage(char* dst, char* src);
	int __vectorcall util_GetFunctionFromSlice(int* func, int* fieldcount);
	void __vectorcall util_DiscardMessage(void);
	void __vectorcall util_SendMesg(int fd, int func, char* buffer);

	// -------------------------------------------------------------------
	// Encoding function units.  Use in Encrypting functions.
	int __vectorcall util_256to64(char* dst, char* src, int len, const char* table);
	int __vectorcall util_64to256(char* dst, char* src, const char* table);
	int __vectorcall util_256to64_shr(char* dst, char* src, int len, const char* table, const char* key);
	int __vectorcall util_shl_64to256(char* dst, const char* src, const char* table, const char* key);
	int __vectorcall util_256to64_shl(char* dst, char* src, int len, const char* table, const char* key);
	int __vectorcall util_shr_64to256(char* dst, const char* src, const char* table, const char* key);

	void __vectorcall util_swapint(int* dst, int* src, const char* rule);
	void __vectorcall util_xorstring(char* dst, char* src);
	void __vectorcall util_shrstring(char* dst, char* src, int offs);
	void __vectorcall util_shlstring(char* dst, char* src, int offs);
	// -------------------------------------------------------------------
	// Encrypting functions
	int __vectorcall util_deint(int sliceno, int* value);
	int __vectorcall util_mkint(char* buffer, int value);
	int __vectorcall util_destring(int sliceno, char* value);
	int __vectorcall util_mkstring(char* buffer, char* value);

	int __vectorcall strcmptail(char* s1, char* s2);

	BOOL __vectorcall getStringFromIndexWithDelim_body(char* src, char* delim, int index, char* buf, int buflen);
	int __vectorcall getLineFromReadBuf(char* output, int maxlen);

	mutable QReadWriteLock m_slicelock;
	mutable QReadWriteLock m_lock;
public:
	QScopedArrayPointer<char> g_net_readbuf;
	int g_net_readbuflen = 0u;

private:
	int shiftReadBuf(int size);
};

#endif
