// Arminius' protocol utilities ver 0.1
//
// Any questions and bugs, mailto: arminius@mail.hwaei.com.tw

// -------------------------------------------------------------------
// The following definitions is to define game-dependent codes.
// Before compiling, remove the "//".
#define __STONEAGE
#include "autil.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <random>
#include "../util.hpp"

Autil::Autil()
	:SliceCount(0)
{
}

Autil::~Autil()
{

}

// -------------------------------------------------------------------
// Initialize utilities
//
void Autil::util_Init(void)
{
	MesgSlice.reset(q_check_ptr(new char[sizeof(char*) * SLICE_MAX][SLICE_SIZE]()));
	util_Release();
	util_DiscardMessage();
}

void Autil::util_Release(void)
{
	int i;

	try
	{
		for (i = 0; i < (sizeof(char*) * SLICE_MAX); ++i) {
			ZeroMemory(MesgSlice[i], SLICE_SIZE);
		}
	}
	catch (...)
	{
		// do nothing
	}
}

// -------------------------------------------------------------------
// Discard a message from MesgSlice.
//
void Autil::util_DiscardMessage(void)
{
	SliceCount = 0;
}

// -------------------------------------------------------------------
// Split up a message into slices by spearator.  Store those slices
// into a global buffer "char **MesgSlice"
//
// arg: source=message string;  separator=message separator (1 byte)
// ret: (none)
void Autil::util_SplitMessage(char* source, size_t buflen, const char* separator, size_t separatorlen)
{
	//if (source && separator)
	//{ // NULL input is invalid.
	//	char* ptr;
	//	char* head = source;

	//	while ((ptr = (char*)safe_strstr(head, buflen, separator, 1u)) && (SliceCount <= SLICE_MAX))
	//	{
	//		ptr[0] = '\0';
	//		if (strnlen_s(head, buflen) < SLICE_SIZE)
	//		{ // discard slices too large
	//			strncpy_s(MesgSlice[SliceCount], SLICE_SIZE, head, buflen);
	//			SliceCount++;
	//		}
	//		head = ptr + 1u;
	//	}

	//	strncpy_s(source, buflen, head, buflen); // remove splited slices
	//}
	Util util;
	if (source && separator && separatorlen > 0 && buflen > 0)
	{ // NULL input is invalid, separator length must be positive, and buffer length must be positive.
		char* ptr;
		char* head = source;

		try
		{
			while ((ptr = (char*)util.safe_strstr(head, buflen, separator, separatorlen)) && (SliceCount <= SLICE_MAX))
			{
				ptr[0] = '\0';
				size_t slice_len = min(buflen, (size_t)(ptr - head));
				if (slice_len < SLICE_SIZE)
				{ // discard slices too large
					strncpy_s(MesgSlice[SliceCount], SLICE_SIZE, head, slice_len);
					SliceCount++;
				}
				head = ptr + separatorlen;
				buflen -= slice_len + separatorlen;
			}

			size_t tail_len = min(buflen, (size_t)strnlen_s(head, buflen));
			strncpy_s(source, buflen, head, tail_len); // remove splited slices
		}
		catch (...)
		{
			// do nothing
		}
	}
}

// -------------------------------------------------------------------
// Encode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_EncodeMessage(char* dst, size_t dstlen, char* src, size_t srclen)
{
	if (!dst || !src || dstlen == 0 || srclen == 0) {
		return;
	}

	std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> distribution(0, 99);
	int rn = distribution(generator);
	int t1, t2;
	QScopedArrayPointer <char> t3(q_check_ptr(new char[MAX_LBUFFER]()));
	if (t3.isNull()) return;
	QScopedArrayPointer <char> tz(q_check_ptr(new char[MAX_LBUFFER]()));
	if (tz.isNull()) return;

#ifdef _BACK_VERSION
	util_swapint(&t1, &rn, "3421"); // encode seed
#else
	util_swapint(&t1, sizeof(int), &rn, sizeof(int), "2413", 4u); // encode seed
#endif
	//  t2 = t1 ^ 0x0f0f0f0f;
	t2 = t1 ^ UINT32_MAX;
	util_256to64(tz.get(), MAX_LBUFFER, (char*)&t2, sizeof(int), DEFAULTTABLE, DEFAULTTABLESIZE);

	util_shlstring(t3.get(), MAX_LBUFFER, src, srclen, rn);
	try
	{
		//  printf("random number=%d\n", rn);
		strncat_s(tz.get(), MAX_LBUFFER, t3.get(), MAX_LBUFFER);
	}
	catch (...)
	{
		return;
	}
	util_xorstring(dst, dstlen, tz.get(), MAX_LBUFFER);
}

// -------------------------------------------------------------------
// Decode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_DecodeMessage(char* dst, size_t dstlen, const char* src, size_t srclen)
{
	if (!dst || !src || dstlen == 0 || srclen == 0) {
		return;
	}

	constexpr auto INTCODESIZE = (sizeof(int) * 8 + 5) / 6;

	int rn;
	int t2;
	QScopedArrayPointer <char> t3(q_check_ptr(new char[MAX_LBUFFER]())); // This buffer is enough for an integer.
	if (t3.isNull()) return;
	QScopedArrayPointer <char> t4(q_check_ptr(new char[MAX_STINYBUFF]()));
	if (t4.isNull()) return;
	QScopedArrayPointer <char> tz(q_check_ptr(new char[MAX_LBUFFER]()));
	if (tz.isNull()) return;
	try
	{

		if (src[srclen - 1] == '\n')
		{
			srclen--;
		}
	}
	catch (...)
	{
		return;
	}
	util_xorstring(tz.get(), MAX_LBUFFER, src, srclen);

	rn = INTCODESIZE;

	try
	{
		strncpy_s(t4.get(), MAX_STINYBUFF, tz.get(), INTCODESIZE);
		t4[INTCODESIZE] = '\0';
	}
	catch (...)
	{
		return;
	}

	util_64to256(t3.get(), MAX_LBUFFER, t4.get(), MAX_STINYBUFF, DEFAULTTABLE, DEFAULTTABLESIZE);
	const int* t1 = reinterpret_cast<int*>(t3.get());

	t2 = *t1 ^ UINT32_MAX;
#ifdef _BACK_VERSION
	util_swapint(&rn, &t2, "4312");
#else
	util_swapint(&rn, sizeof(int), &t2, sizeof(int), "3142", 4u);
#endif
	util_shrstring(dst, dstlen, tz.get() + INTCODESIZE, MAX_LBUFFER - INTCODESIZE, rn);
}

// -------------------------------------------------------------------
// Get a function information from MesgSlice.  A function is a complete
// and identifiable message received, beginned at DEFAULTFUNCBEGIN and
// ended at DEFAULTFUNCEND.  This routine will return the function ID
// (Action ID) and how many fields this function have.
//
// arg: func=return function ID    fieldcount=return fields of the function
// ret: 1=success  0=failed (function not complete)
int Autil::util_GetFunctionFromSlice(int* func, int* fieldcount)
{
	if (!func || !fieldcount)
		return 0;

	Util util;
	QScopedArrayPointer<char> t1(q_check_ptr(new char[MAX_SMALLBUFF]));
	if (t1.isNull()) return 0;
	int i;

	try
	{
		strncpy_s(t1.get(), MAX_SMALLBUFF, MesgSlice[1], sizeof(MesgSlice[1]));
		// Robin adjust
		*func = util.safe_atoi(t1.get()) - 23;
		for (i = 0; i < SLICE_MAX; i++)
		{
			if (strncmp(MesgSlice[i], DEFAULTFUNCEND, 1u) == 0)
			{
				*fieldcount = i - 2; // - "&" - "#" - "func" 3 fields
				return 1;
			}
		}
	}
	catch (...)
	{
		return 0;
	}

	return 0; // failed: message not complete
}

// -------------------------------------------------------------------
// Send a message
//
// arg: fd=socket fd   func=function ID   buffer=data to send
void Autil::util_SendMesg(int fd, int func, char* buffer, size_t buflen)
{
	if (!buffer)
		return;

	QScopedArrayPointer<char> t1(q_check_ptr(new char[MAX_SMALLBUFF]));
	if (t1.isNull()) return;
	QScopedArrayPointer<char> t2(q_check_ptr(new char[MAX_SMALLBUFF]));
	if (t2.isNull()) return;
	try
	{
		_snprintf_s(t1.get(), MAX_SMALLBUFF, _TRUNCATE, "&;%d%s;#;", func + 13, buffer);
	}
	catch (...)
	{
	}
	util_EncodeMessage(t2.get(), MAX_SMALLBUFF, t1.get(), MAX_SMALLBUFF);
	try
	{
		size_t nSize = strnlen_s(t2.get(), MAX_SMALLBUFF);
		t2[nSize] = '\n';
		nSize += 1;
		int ret = send((SOCKET)fd, t2.get(), nSize, 0);
	}
	catch (...)
	{
	}
}

// -------------------------------------------------------------------
// Convert 8-bit strings into 6-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=8-bit string;  src=6-bit string;  len=src strlen;
//      table=mapping table
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen)
{
	unsigned int dw, dwcounter;
	size_t i;

	if (!dst || !src || !table)
		return 0;
	dw = 0;
	dwcounter = 0;
	try
	{
		for (i = 0; i < srclen; i++)
		{
			dw = (((unsigned int)src[i] & 0xff) << ((i % 3) << 1)) | dw;
			dst[dwcounter++] = table[dw & 0x3f];
			dw = (dw >> 6);
			if (i % 3 == 2)
			{
				dst[dwcounter++] = table[dw & 0x3f];
				dw = 0;
			}
		}
		if (dw)
			dst[dwcounter++] = table[dw];
		dst[dwcounter] = '\0';
	}
	catch (...)
	{
		return 0;
	}
	return dwcounter;
}

// -------------------------------------------------------------------
// Convert 6-bit strings into 8-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=6-bit string;  src=8-bit string;  table=mapping table
// ret: 0=failed  >0=bytes converted
int Autil::util_64to256(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen)
{
	unsigned int dw, dwcounter;
	unsigned int i, j;
	char* ptr = nullptr;

	dw = 0;
	dwcounter = 0;
	if (!dst || !src || !table)
		return 0;
	char c;
	try
	{
		for (i = 0; i < strnlen_s(src, srclen); i++)
		{
			c = src[i];
			for (j = 0; j < strnlen_s(table, tablelen); j++)
			{
				if (table[j] == c)
				{
					ptr = (char*)table + j;
					break;
				}
			}
			if (!ptr)
				return 0;
			if (i % 4)
			{
				dw = ((unsigned int)(ptr - table) & 0x3f) << ((4 - (i % 4)) << 1) | dw;
				dst[dwcounter++] = dw & 0xff;
				dw = dw >> 8;
			}
			else
			{
				dw = (unsigned int)(ptr - table) & 0x3f;
			}
		}
		if (dw)
			dst[dwcounter++] = dw & 0xff;
		dst[dwcounter] = '\0';
	}
	catch (...)
	{
		return 0;
	}

	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64_shr(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen)
{
	unsigned int dw, dwcounter, j;
	size_t i;

	if (!dst || !src || !table || !key)
		return 0;
	if (strnlen_s(key, keylen) < 1)
		return 0; // key can't be empty.
	dw = 0;
	dwcounter = 0;
	j = 0;
	try
	{
		for (i = 0; i < srclen; i++)
		{
			dw = (((unsigned int)src[i] & 0xff) << ((i % 3) << 1)) | dw;
			dst[dwcounter++] = table[((dw & 0x3f) + key[j]) % 64]; // check!
			j++;
			if (!key[j])
				j = 0;
			dw = (dw >> 6);
			if (i % 3 == 2)
			{
				dst[dwcounter++] = table[((dw & 0x3f) + key[j]) % 64]; // check!
				j++;
				if (!key[j])
					j = 0;
				dw = 0;
			}
		}
		if (dw)
			dst[dwcounter++] = table[(dw + key[j]) % 64]; // check!
		dst[dwcounter] = '\0';
	}
	catch (...)
	{
		return 0;
	}
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shr.
//
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_shl_64to256(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen)
{
	unsigned int dw, dwcounter, i, j, k;
	char* ptr = NULL;

	if (!dst || !src || !table || !key)
		return 0;

	if (!key || (strnlen_s(key, keylen) < 1))
		return 0; // must have key

	dw = 0;
	dwcounter = 0;
	j = 0;
	if (!dst || !src || !table)
		return 0;
	char c;
	try
	{
		for (i = 0; i < strnlen_s(src, keylen); i++)
		{
			c = src[i];
			for (k = 0; k < strnlen_s(table, tablelen); k++)
			{
				if (table[k] == c)
				{
					ptr = (char*)table + k;
					break;
				}
			}
			if (!ptr)
				return 0;
			if (i % 4)
			{
				// check!
				dw = ((((unsigned int)(ptr - table) & 0x3f) + 64 - key[j]) % 64)
					<< ((4 - (i % 4)) << 1) |
					dw;
				j++;
				if (!key[j])
					j = 0;
				dst[dwcounter++] = dw & 0xff;
				dw = dw >> 8;
			}
			else
			{
				// check!
				dw = (((unsigned int)(ptr - table) & 0x3f) + 64 - key[j]) % 64;
				j++;
				if (!key[j])
					j = 0;
			}
		}
		if (dw)
			dst[dwcounter++] = dw & 0xff;
		dst[dwcounter] = '\0';

	}
	catch (...)
	{
		return 0;
	}
	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64_shl(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen)
{
	unsigned int dw, dwcounter;
	size_t i, j;

	if (!dst || !src || !table || !key)
		return 0;
	if (strnlen_s(key, keylen) < 1)
		return 0; // key can't be empty.
	dw = 0;
	dwcounter = 0;
	j = 0;
	try
	{
		for (i = 0; i < srclen; i++)
		{
			dw = (((unsigned int)src[i] & 0xff) << ((i % 3) << 1)) | dw;
			dst[dwcounter++] = table[((dw & 0x3f) + 64 - key[j]) % 64]; // check!
			j++;
			if (!key[j])
				j = 0;
			dw = (dw >> 6);
			if (i % 3 == 2)
			{
				dst[dwcounter++] = table[((dw & 0x3f) + 64 - key[j]) % 64]; // check!
				j++;
				if (!key[j])
					j = 0;
				dw = 0;
			}
		}
		if (dw)
			dst[dwcounter++] = table[(dw + 64 - key[j]) % 64]; // check!
		dst[dwcounter] = '\0';
	}
	catch (...)
	{
		return 0;
	}
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shl.
//
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_shr_64to256(char* dst, size_t dstlen, const char* src, size_t srclen, const char* table, size_t tablelen, const char* key, size_t keylen)
{
	unsigned int dw, dwcounter, i, j, k;
	char* ptr = NULL;
	if (!dst || !src || !table || !key)
		return 0;

	if (!key || (strnlen_s(key, keylen) < 1))
		return 0; // must have key
	dw = 0;
	dwcounter = 0;
	j = 0;
	if (!dst || !src || !table)
		return 0;
	char c;
	try
	{
		for (i = 0; i < strnlen_s(src, srclen); i++)
		{
			c = src[i];
			for (k = 0; k < strnlen_s(table, tablelen); k++)
			{
				if (table[k] == c)
				{
					ptr = (char*)table + k;
					break;
				}
			}
			if (!ptr)
				return 0;
			if (i % 4)
			{
				// check!
				dw = ((((unsigned int)(ptr - table) & 0x3f) + key[j]) % 64)
					<< ((4 - (i % 4)) << 1) |
					dw;
				j++;
				if (!key[j])
					j = 0;
				dst[dwcounter++] = dw & 0xff;
				dw = dw >> 8;
			}
			else
			{
				// check!
				dw = (((unsigned int)(ptr - table) & 0x3f) + key[j]) % 64;
				j++;
				if (!key[j])
					j = 0;
			}
		}
		if (dw)
			dst[dwcounter++] = dw & 0xff;
		dst[dwcounter] = '\0';
	}
	catch (...)
	{
		return 0;
	}
	return dwcounter;
}

// -------------------------------------------------------------------
// Swap a integer (4 byte).
// The value "rule" indicates the swaping rule.  It's a 4 byte string
// such as "1324" or "2431".
//
void Autil::util_swapint(int* dst, size_t dstlen, int* src, size_t srclen, const char* rule, size_t rulelen)
{
	if (!dst || !src || !rule)
		return;

	if (dstlen < sizeof(int) || srclen < sizeof(int) || rulelen < 4u)
	{
		return;
	}
	char* ptr = reinterpret_cast<char*>(src);
	char* qtr = reinterpret_cast<char*>(dst);
	constexpr size_t size = sizeof(int);
	try
	{
		for (size_t i = 0u; i < size; i++)
		{
			qtr[rule[i] - '1'] = ptr[i];
		}
	}
	catch (...)
	{
		return;
	}
}

// -------------------------------------------------------------------
// Xor a string.  Be careful that your string contains '0xff'.  Your
// data may lose.
//
void Autil::util_xorstring(char* dst, size_t dstlen, const char* src, size_t srclen)
{
	unsigned int i;
	if (!dst || !src)
		return;

	if (strnlen_s(src, srclen) < 1u)
		return;

	//dst必須大於等於src
	if (dstlen < strnlen_s(src, srclen))
		return;

	try
	{
		for (i = 0; i < strnlen_s(src, srclen); i++)
			dst[i] = src[i] ^ 255;
		dst[i] = '\0';
	}
	catch (...) {}
}

// -------------------------------------------------------------------
// Shift the string right.
//
void Autil::util_shrstring(char* dst, size_t dstlen, const char* src, size_t srclen, int offs)
{
	size_t len = strnlen_s(src, srclen);
	if (!dst || !src || (len < 1u))
		return;

	try
	{
		offs = len - (offs % len);
		const char* ptr = src + offs;
		strncpy_s(dst, dstlen, ptr, srclen - offs);
		strncat_s(dst, MAX_SMALLBUFF, src, offs);
		dst[len] = '\0';
	}
	catch (...)
	{
	}
}

// -------------------------------------------------------------------
// Shift the string left.
//
void Autil::util_shlstring(char* dst, size_t dstlen, const char* src, size_t srclen, int offs)
{
	size_t len = strnlen_s(src, srclen);
	if (!dst || !src || (len < 1u))
		return;

	try
	{
		offs = offs % len;
		const char* ptr = src + offs;
		strncpy_s(dst, dstlen, ptr, srclen - offs);
		strncat_s(dst, MAX_LBUFFER, src, offs);
		dst[len] = '\0';
	}
	catch (...)
	{
	}
}

// -------------------------------------------------------------------
// Convert a message slice into integer.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkint
int Autil::util_deint(int sliceno, int* value)
{
	if (!value)
		return 0;

	STATICINS(KeyManager);
	int* t1, t2;
	//char t3[MAX_STINYBUFF]; // This buffer is enough for an integer.
	QScopedArrayPointer <char> t3(new char[MAX_STINYBUFF]());
	if (t3.isNull()) return 0;

	util_shl_64to256(t3.get(), MAX_STINYBUFF, MesgSlice[sliceno], sizeof(MesgSlice[sliceno]), DEFAULTTABLE, DEFAULTTABLESIZE, g_KeyManager.GetKey().c_str(), g_KeyManager.size());
	try
	{
		t1 = (int*)t3.get();
		t2 = *t1 ^ UINT32_MAX;
	}
	catch (...)
	{
		return 0;
	}
#ifdef _BACK_VERSION
	util_swapint(value, &t2, "3421");
#else
	util_swapint(value, sizeof(int), &t2, sizeof(int), "2413", 4u);
#endif
	return *value;
}

// -------------------------------------------------------------------
// Pack a integer into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_deint
int Autil::util_mkint(char* buffer, size_t buflen, int value)
{
	if (!buffer)
		return 0;

	STATICINS(KeyManager);
	int t1, t2;
	//char t3[MAX_STINYBUFF]; // This buffer is enough for an integer.
	QScopedArrayPointer <char> t3(new char[MAX_STINYBUFF]());
	if (t3.isNull()) return 0;

#ifdef _BACK_VERSION
	util_swapint(&t1, &value, "4312");
#else
	util_swapint(&t1, sizeof(int), &value, sizeof(int), "3142", 4u);
#endif
	try
	{
		t2 = t1 ^ UINT32_MAX;
	}
	catch (...)
	{
		return 0;
	}
	util_256to64_shr(t3.get(), MAX_STINYBUFF, (char*)&t2, sizeof(int), DEFAULTTABLE, DEFAULTTABLESIZE, g_KeyManager.GetKey().c_str(), g_KeyManager.size());
	try
	{
		strncat_s(buffer, buflen, SEPARATOR, 1); // It's important to append a SEPARATOR between fields
		strncat_s(buffer, buflen, t3.get(), MAX_STINYBUFF);
	}
	catch (...)
	{
		return 0;
	}

	return value;
}

// -------------------------------------------------------------------
// Convert a message slice into string.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkstring
int Autil::util_destring(int sliceno, char* value, size_t valuelen)
{
	if (!value)
		return 0;

	STATICINS(KeyManager);
	util_shr_64to256(value, valuelen, MesgSlice[sliceno], sizeof(MesgSlice[sliceno]), DEFAULTTABLE, DEFAULTTABLESIZE, g_KeyManager.GetKey().c_str(), g_KeyManager.size());

	try
	{
		return strnlen_s(value, valuelen);
	}
	catch (...)
	{
		return 0;
	}

	return 0;
}

// -------------------------------------------------------------------
// Convert a string into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_destring
int Autil::util_mkstring(char* buffer, size_t buflen, const char* value, size_t valuelen)
{
	if (!buffer || !value)
		return 0;

	STATICINS(KeyManager);
	QScopedArrayPointer <char> t1(new char[MAX_LBUFFER]());
	if (t1.isNull()) return 0;
	size_t size = 0;

	try
	{
		size = strnlen_s(value, valuelen);
	}
	catch (...)
	{
		return 0;
	}

	util_256to64_shl(t1.get(), MAX_LBUFFER, value, size, DEFAULTTABLE, DEFAULTTABLESIZE, g_KeyManager.GetKey().c_str(), g_KeyManager.size());
	try
	{
		strncat_s(buffer, MAX_SMALLBUFF, SEPARATOR, 1u); // It's important to append a SEPARATOR between fields
		strncat_s(buffer, MAX_SMALLBUFF, t1.get(), MAX_LBUFFER);
		return strnlen_s(value, valuelen);
	}
	catch (...)
	{
		return 0;
	}
	return 0;
}


//int Autil::strcmptail(char* s1, char* s2)
//{
//	int i;
//	int len1 = strlen(s1);
//	int len2 = strlen(s2);
//
//	for (i = 0;; i++)
//	{
//		int ind1 = len1 - 1 - i;
//		int ind2 = len2 - 1 - i;
//		if (ind1 < 0 || ind2 < 0)
//			return 0;
//		if (s1[ind1] != s2[ind2])
//			return 1;
//	}
//}

//#define IS_2BYTEWORD(_a_) ((char)(0x80) <= (_a_) && (_a_) <= (char)(0xFF))
//char* ScanOneByte(char* src, char delim)
//{
//	if (!src)
//		return NULL;
//	for (; src[0] != '\0'; src++)
//	{
//		if (IS_2BYTEWORD(src[0]))
//		{
//			if (src[1] != 0)
//			{
//				src++;
//			}
//			continue;
//		}
//		if (src[0] == delim)
//		{
//			return src;
//		}
//	}
//	return NULL;
//}

//void Autil::util_InitReadBuf(char* dst, size_t dstlen, const char* src, size_t srclen)
//{
//	//if (g_net_readbuf.isNull())
//	//	g_net_readbuf.reset(q_check_ptr(new char[NETBUFSIZ]()));
//	//else
//	//	ZeroMemory(g_net_readbuf.get(), NETBUFSIZ);
//
//	//memcpy_s(g_net_readbuf.get(), NETBUFSIZ, encoded, buflen);
//	//g_net_readbuflen = buflen;
//}

int Autil::util_getLineFromReadBuf(char* dst, size_t dstlen, char* src, ULONG& srclen, size_t maxlen)
{
	if (!dst || !src)
		return -1;

	auto shiftReadBuf = [](char* dst, ULONG& dstlen, int size)->int
	{
		size_t i;

		if ((size_t)size > dstlen)
			return -1;
		for (i = size; i < dstlen; i++)
		{
			try
			{
				dst[i - size] = dst[i];
			}
			catch (...)
			{
				return -1;
			}
		}
		dstlen -= size;
		return 0;
	};

	size_t i, j;
	for (i = 0; i < srclen && i < (maxlen - 1); i++)
	{
		try
		{
			if (src[i] == '\n')
			{
				memcpy_s(dst, dstlen, src, i);
				dst[i] = '\0';
				for (j = i + 1; j > 0; j--)
				{
					if (dst[j] == '\r')
					{
						dst[j] = '\0';
						break;
					}
				}

				shiftReadBuf(src, srclen, i + 1);
				src[srclen] = '\0';

				return 0;
			}
		}
		catch (...)
		{
			return -1;
		}
	}

	return -1;
}

//char* strncpy2(char* dest, const char* src, size_t n)
//{
//	if (n > 0)
//	{
//		char* d = dest;
//		const char* s = src;
//		unsigned int i;
//		for (i = 0; i < n; i++)
//		{
//			if (*(s + i) == 0)
//			{
//				*(d + i) = '\0';
//				return dest;
//			}
//			if (*(s + i) & 0x80)
//			{
//				*(d + i) = *(s + i);
//				i++;
//				if (i >= n)
//				{
//					*(d + i - 1) = '\0';
//					break;
//				}
//				*(d + i) = *(s + i);
//			}
//			else
//				*(d + i) = *(s + i);
//		}
//	}
//	return dest;
//}

//void strcpysafe(char* dest, size_t n, const char* src)
//{
//	if (!src)
//	{
//		*dest = '\0';
//		return;
//	}
//	if (n <= 0)
//		return;
//	else if (n < strlen(src) + 1)
//	{
//		strncpy2(dest, src, n - 1);
//		dest[n - 1] = '\0';
//	}
//	else
//		strcpy(dest, src);
//}
//
//void strncpysafe(char* dest, const size_t n, const char* src, const int length)
//{
//	unsigned int Short;
//	Short = min(strlen(src), (unsigned int)length);
//	if (n < Short + 1)
//	{
//		strncpy2(dest, src, n - 1);
//		dest[n - 1] = '\0';
//	}
//	else if (n <= 0)
//	{
//		return;
//	}
//	else
//	{
//		strncpy2(dest, src, Short);
//		dest[Short] = '\0';
//	}
//}

//bool Autil::getStringFromIndexWithDelim_body(char* src, char* delim, int index, char* buf, int buflen)
//{
//	int i;
//	int length = 0;
//	int addlen = 0;
//	int oneByteMode = 0;
//
//	size_t delsize = strnlen_s(delim, 256);
//	if (delsize == 1)
//	{
//		oneByteMode = 1;
//	}
//	for (i = 0; i < index; i++)
//	{
//		char* last;
//		src += addlen;
//
//		if (oneByteMode)
//		{
//			last = ScanOneByte(src, delim[0]);
//		}
//		else
//		{
//			last = strstr(src, delim);
//		}
//		if (last == NULL)
//		{
//			//strcpysafe(buf, buflen, src);
//			_snprintf_s(buf, buflen, _TRUNCATE, "%s", src);
//			if (i == index - 1)
//			{
//				if (buf[0] == 0)
//					return FALSE;
//				return TRUE;
//			}
//			buf[0] = 0;
//			return FALSE;
//		}
//		length = last - src;
//		addlen = length + delsize;
//	}
//	_snprintf_s(buf, buflen, _TRUNCATE, "%s", src);
//	//strncpysafe(buf, buflen, src, length);
//	if (buf[0] == 0)
//		return FALSE;
//	return TRUE;
//}