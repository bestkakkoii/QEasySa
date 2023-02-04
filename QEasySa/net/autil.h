#pragma once
#ifndef AUTIL_H
#define AUTIL_H
#include "../stdafx.h"
#include <QReadWriteLock>

constexpr auto SLICE_MAX = 20;
constexpr auto SLICE_SIZE = 65500;
constexpr auto NETBUFSIZ = 1024 * 64;
constexpr auto MAX_LBUFFER = 65500;
constexpr auto MAX_BUFFER = 32768;
constexpr auto MAX_SMALLBUFF = 16384;
constexpr auto MAX_TINYBUFF = 8192;
constexpr auto MAX_STINYBUFF = 4096;

constexpr auto DEFAULTTABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}";
constexpr auto DEFAULTTABLESIZE = 64;
constexpr auto DEFAULTFUNCBEGIN = "&";
constexpr auto DEFAULTFUNCEND = "#";
constexpr auto SEPARATOR = ";";

class KeyManager
{
	DISABLE_COPY_MOVE(KeyManager);

public:
	virtual ~KeyManager() { m_key.clear(); }
	void SetKey(const std::string& key)
	{
		EncryptKey(key);
	}
	std::string GetKey() const
	{
		return DecryptKey();
	}
	bool isValid() const
	{
		return !m_key.empty();
	}

	bool isValid(const std::string& key) const
	{
		return !key.empty();
	}

	size_t size() const
	{
		std::string key = DecryptKey();
		return key.length();
	}


private:
	KeyManager() = default;

	//key加密
	void EncryptKey(const std::string& key)
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
	std::string DecryptKey() const
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
	Autil();
	virtual ~Autil();
	void util_SplitMessage(char* source, size_t buflen, const char* separator, size_t separatorlen);
	int util_GetFunctionFromSlice(int* func, int* fieldcount);
	void util_SendMesg(int fd, int func, char* buffer, size_t buflen);
	void util_DecodeMessage(char* dst, size_t dstlen, const char* src, size_t srclen);
	void util_DiscardMessage(void);

	// -------------------------------------------------------------------
	// Encrypting functions
	int util_deint(int sliceno, int* value);
	int util_mkint(char* buffer, size_t buflen, int value);
	int util_destring(int sliceno, char* value, size_t valuelen);
	int util_mkstring(char* buffer, size_t buflen, const char* value, size_t valuelen);

	//bool getStringFromIndexWithDelim_body(char* src, char* delim, int index, char* buf, int buflen);
	int getLineFromReadBuf(char* output, size_t outputlen, int maxlen);

public:
	QScopedArrayPointer<char> g_net_readbuf;
	int g_net_readbuflen = 0u;

private:
	char MesgSlice[sizeof(char*) * SLICE_MAX][SLICE_SIZE] = {};
	int SliceCount;

private:
	int shiftReadBuf(int size);
	void util_Init(void);
	void util_EncodeMessage(char* dst, size_t dstlen, char* src, size_t srclen);
	void util_Release(void);
	//int strcmptail(char* s1, char* s2);

	// -------------------------------------------------------------------
	// Encoding function units.  Use in Encrypting functions.
	int util_256to64(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen);
	int util_64to256(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen);
	int util_256to64_shr(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen);
	int util_shl_64to256(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen);
	int util_256to64_shl(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen);
	int util_shr_64to256(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen);

	void util_swapint(int* dst, size_t dstlen, int* src, size_t srclen, const char* rule, size_t rulelen);
	void util_xorstring(char* dst, size_t dstlen, const char* src, size_t srclen);
	void util_shrstring(char* dst, size_t dstlen, const char* src, size_t srclen, int offs);
	void util_shlstring(char* dst, size_t dstlen, const char* src, size_t srclen, int offs);
};

#endif
