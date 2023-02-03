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


Autil::Autil()
{
	SliceCount = 0;
	g_net_readbuf.reset(q_check_ptr(new char[NETBUFSIZ]()));
}

Autil::~Autil()
{

}

char* Autil::index(const char* table, char src)
{
	char* p = (char*)table;
	while (*p != src) {
		p++;
	}
	return p;
}

// -------------------------------------------------------------------
// Initialize utilities
//
void Autil::util_Init(void)
{
	int i;

	for (i = 0; i < SLICE_MAX; i++) {
		memset(MesgSlice[i], 0, SLICE_SIZE);
	}
	SliceCount = 0;
}

// -------------------------------------------------------------------
// Split up a message into slices by spearator.  Store those slices
// into a global buffer "char **MesgSlice"
//
// arg: source=message string;  separator=message separator (1 byte)
// ret: (none)
void Autil::util_SplitMessage(char* source, const char* separator)
{
	if (source && separator)
	{ // NULL input is invalid.
		char* ptr;
		char* head = source;

		while ((ptr = (char*)strstr(head, separator)) && (SliceCount <= SLICE_MAX))
		{
			ptr[0] = '\0';
			if (strlen(head) < SLICE_SIZE)
			{ // discard slices too large
				strcpy(MesgSlice[SliceCount], head);
				SliceCount++;
			}
			head = ptr + 1;
		}
		strcpy(source, head); // remove splited slices
	}
}

// -------------------------------------------------------------------
// Encode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_EncodeMessage(char* dst, char* src)
{
	int rn = rand() % 99;
	int t1, t2;
	char t3[65500], tz[65500];

#ifdef _BACK_VERSION
	util_swapint(&t1, &rn, "3421"); // encode seed
#else
	util_swapint(&t1, &rn, "2413"); // encode seed
#endif
	//  t2 = t1 ^ 0x0f0f0f0f;
	t2 = t1 ^ 0xffffffff;
	util_256to64(tz, (char*)&t2, sizeof(int), DEFAULTTABLE);

	util_shlstring(t3, src, rn);
	//  printf("random number=%d\n", rn);
	strcat_s(tz, t3);
	util_xorstring(dst, tz);
}

// -------------------------------------------------------------------
// Decode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_DecodeMessage(char* dst, char* src)
{
#define INTCODESIZE (sizeof(int) * 8 + 5) / 6

	int rn;
	int* t1, t2;
	char t3[4096], t4[4096]; // This buffer is enough for an integer.
	char tz[65500];

	if (src[strlen(src) - 1] == '\n')
		src[strlen(src) - 1] = 0;
	util_xorstring(tz, src);

	rn = INTCODESIZE;
	//  printf("INTCODESIZE=%d\n", rn);

	strncpy_s(t4, tz, INTCODESIZE);
	t4[INTCODESIZE] = '\0';
	util_64to256(t3, t4, DEFAULTTABLE);
	t1 = (int*)t3;
	//  t2 = *t1 ^ 0x0f0f0f0f;
	t2 = *t1 ^ 0xffffffff;
#ifdef _BACK_VERSION
	util_swapint(&rn, &t2, "4312");
#else
	util_swapint(&rn, &t2, "3142");
#endif
	//  printf("random number=%d\n", rn);
	util_shrstring(dst, tz + INTCODESIZE, rn);
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
	char t1[MAX_SMALLBUFF] = {};
	int i;

	//  if (strcmp(MesgSlice[0], DEFAULTFUNCBEGIN)!=0) util_DiscardMessage();

	strcpy(t1, MesgSlice[1]);
	// Robin adjust
	//*func=atoi(t1);
	*func = atoi(t1) - 23;
	for (i = 0; i < SLICE_MAX; i++)
		if (strcmp(MesgSlice[i], DEFAULTFUNCEND) == 0)
		{
			*fieldcount = i - 2; // - "&" - "#" - "func" 3 fields
			return 1;
		}

	return 0; // failed: message not complete
}

// -------------------------------------------------------------------
// Discard a message from MesgSlice.
//
void Autil::util_DiscardMessage(void)
{
	SliceCount = 0;
	/*
	  int i,j;
	  void *ptr;

	  i=0;
	  while ((i<SliceCount)&&(strcmp(MesgSlice[i], DEFAULTFUNCBEGIN)!=0)) i++;

	  if (i>=SliceCount) {
		// discard all message
		for (j=0; j<SliceCount; j++) strcpy(MesgSlice[j],"");
	  } else {
		for (j=0; j<SliceCount-i; j++) {
		  ptr=MesgSlice[j];
		  MesgSlice[j]=MesgSlice[j+i];
		  MesgSlice[j+i]=ptr;
		}
	  }
	*/
}

// -------------------------------------------------------------------
// Send a message
//
// arg: fd=socket fd   func=function ID   buffer=data to send
void Autil::util_SendMesg(int fd, int func, char* buffer)
{
	char t1[MAX_SMALLBUFF], t2[MAX_SMALLBUFF];
	//qDebug() << "util_SendMesg SOCKET:" << fd;
	//qDebug() << "MSG:" << QString(buffer);
	sprintf_s(t1, sizeof(t1), "&;%d%s;#;", func + 13, buffer);
	//qDebug() << "ENCODE MSG:" << QString(t1);
	util_EncodeMessage(t2, t1);
	//qDebug() << "ENCODE MSG2:" << QString(t2);
	size_t nSize = strlen(t2);
	t2[nSize] = '\n';
	nSize += 1;
	//qDebug() << "SIZE:" << nSize;
	int ret = send((SOCKET)fd, t2, nSize, 0);
	//qDebug() << "RESULT:" << ret;
}

// -------------------------------------------------------------------
// Convert 8-bit strings into 6-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=8-bit string;  src=6-bit string;  len=src strlen;
//      table=mapping table
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64(char* dst, char* src, int len, const char* table)
{
	unsigned int dw, dwcounter;
	int i;

	if (!dst || !src || !table)
		return 0;
	dw = 0;
	dwcounter = 0;
	for (i = 0; i < len; i++)
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
	return dwcounter;
}

// -------------------------------------------------------------------
// Convert 6-bit strings into 8-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=6-bit string;  src=8-bit string;  table=mapping table
// ret: 0=failed  >0=bytes converted
int Autil::util_64to256(char* dst, char* src, const char* table)
{
	unsigned int dw, dwcounter;
	unsigned int i, j;
	char* ptr = NULL;

	dw = 0;
	dwcounter = 0;
	if (!dst || !src || !table)
		return 0;
	char c;
	for (i = 0; i < strlen(src); i++)
	{
		c = src[i];
		for (j = 0; j < strlen(table); j++)
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
	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64_shr(char* dst, char* src, int len, const char* table, const char* key)
{
	unsigned int dw, dwcounter, j;
	int i;

	if (!dst || !src || !table || !key)
		return 0;
	if (strlen(key) < 1)
		return 0; // key can't be empty.
	dw = 0;
	dwcounter = 0;
	j = 0;
	for (i = 0; i < len; i++)
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
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shr.
//
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_shl_64to256(char* dst, const char* src, const char* table, const char* key)
{
	unsigned int dw, dwcounter, i, j, k;
	char* ptr = NULL;

	if (!key || (strlen(key) < 1))
		return 0; // must have key

	dw = 0;
	dwcounter = 0;
	j = 0;
	if (!dst || !src || !table)
		return 0;
	char c;
	for (i = 0; i < strlen(src); i++)
	{
		c = src[i];
		for (k = 0; k < strlen(table); k++)
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
	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64_shl(char* dst, char* src, int len, const char* table, const char* key)
{
	unsigned int dw, dwcounter;
	int i, j;

	if (!dst || !src || !table || !key)
		return 0;
	if (strlen(key) < 1)
		return 0; // key can't be empty.
	dw = 0;
	dwcounter = 0;
	j = 0;
	for (i = 0; i < len; i++)
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
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shl.
//
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_shr_64to256(char* dst, const char* src, const char* table, const char* key)
{
	unsigned int dw, dwcounter, i, j, k;
	char* ptr = NULL;
	if (!key || (strlen(key) < 1))
		return 0; // must have key
	dw = 0;
	dwcounter = 0;
	j = 0;
	if (!dst || !src || !table)
		return 0;
	char c;
	for (i = 0; i < strlen(src); i++)
	{
		c = src[i];
		for (k = 0; k < strlen(table); k++)
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
	return dwcounter;
}

// -------------------------------------------------------------------
// Swap a integer (4 byte).
// The value "rule" indicates the swaping rule.  It's a 4 byte string
// such as "1324" or "2431".
//
void Autil::util_swapint(int* dst, int* src, const char* rule)
{
	char* ptr, * qtr;
	int i;

	ptr = (char*)src;
	qtr = (char*)dst;
	for (i = 0; i < 4; i++)
		qtr[rule[i] - '1'] = ptr[i];
}

// -------------------------------------------------------------------
// Xor a string.  Be careful that your string contains '0xff'.  Your
// data may lose.
//
void Autil::util_xorstring(char* dst, char* src)
{
	unsigned int i;

	for (i = 0; i < strlen(src); i++)
		dst[i] = src[i] ^ 255;
	dst[i] = '\0';
}

// -------------------------------------------------------------------
// Shift the string right.
//
void Autil::util_shrstring(char* dst, char* src, int offs)
{
	char* ptr;
	int len = strlen(src);
	if (!dst || !src || (strlen(src) < 1))
		return;

	offs = strlen(src) - (offs % strlen(src));
	ptr = src + offs;
	strcpy(dst, ptr);
	strncat_s(dst, MAX_SMALLBUFF, src, offs);
	dst[strlen(src)] = '\0';
}

// -------------------------------------------------------------------
// Shift the string left.
//
void Autil::util_shlstring(char* dst, char* src, int offs)
{
	char* ptr;
	if (!dst || !src || (strlen(src) < 1))
		return;

	offs = offs % strlen(src);
	ptr = src + offs;
	strcpy(dst, ptr);
	strncat_s(dst, 65500, src, offs);
	dst[strlen(src)] = '\0';
}

// -------------------------------------------------------------------
// Convert a message slice into integer.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkint
int Autil::util_deint(int sliceno, int* value)
{
	STATICINS(KeyManager);
	int* t1, t2;
	char t3[4096]; // This buffer is enough for an integer.

	util_shl_64to256(t3, MesgSlice[sliceno], DEFAULTTABLE, g_KeyManager.GetKey().c_str());
	t1 = (int*)t3;
	t2 = *t1 ^ 0xffffffff;
#ifdef _BACK_VERSION
	util_swapint(value, &t2, "3421");
#else
	util_swapint(value, &t2, "2413");
#endif
	return *value;
}

// -------------------------------------------------------------------
// Pack a integer into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_deint
int Autil::util_mkint(char* buffer, int value)
{
	STATICINS(KeyManager);
	int t1, t2;
	char t3[4096]; // This buffer is enough for an integer.

#ifdef _BACK_VERSION
	util_swapint(&t1, &value, "4312");
#else
	util_swapint(&t1, &value, "3142");
#endif
	t2 = t1 ^ 0xffffffff;
	util_256to64_shr(t3, (char*)&t2, sizeof(int), DEFAULTTABLE, g_KeyManager.GetKey().c_str());
	strcat(buffer, ";"); // It's important to append a SEPARATOR between fields
	strcat(buffer, t3);

	return value;
}

// -------------------------------------------------------------------
// Convert a message slice into string.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkstring
int Autil::util_destring(int sliceno, char* value)
{
	STATICINS(KeyManager);
	util_shr_64to256(value, MesgSlice[sliceno], DEFAULTTABLE, g_KeyManager.GetKey().c_str());

	return strlen(value);
}

// -------------------------------------------------------------------
// Convert a string into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_destring
int Autil::util_mkstring(char* buffer, char* value)
{
	STATICINS(KeyManager);
	char t1[SLICE_SIZE] = {};

	util_256to64_shl(t1, value, strlen(value), DEFAULTTABLE, g_KeyManager.GetKey().c_str());
	strcat_s(buffer, MAX_SMALLBUFF, ";"); // It's important to append a SEPARATOR between fields
	strcat_s(buffer, MAX_SMALLBUFF, t1);

	return strlen(value);
}

void Autil::util_Release(void)
{
	int i;

	for (i = 0; i < SLICE_MAX; i++) {
		memset(MesgSlice[i], 0, SLICE_SIZE);
	}
}

int Autil::strcmptail(char* s1, char* s2)
{
	int i;
	int len1 = strlen(s1);
	int len2 = strlen(s2);

	for (i = 0;; i++)
	{
		int ind1 = len1 - 1 - i;
		int ind2 = len2 - 1 - i;
		if (ind1 < 0 || ind2 < 0)
			return 0;
		if (s1[ind1] != s2[ind2])
			return 1;
	}
}

#define IS_2BYTEWORD(_a_) ((char)(0x80) <= (_a_) && (_a_) <= (char)(0xFF))
char* ScanOneByte(char* src, char delim)
{
	if (!src)
		return NULL;
	for (; src[0] != '\0'; src++)
	{
		if (IS_2BYTEWORD(src[0]))
		{
			if (src[1] != 0)
			{
				src++;
			}
			continue;
		}
		if (src[0] == delim)
		{
			return src;
		}
	}
	return NULL;
}

char* strncpy2(char* dest, const char* src, size_t n)
{
	if (n > 0)
	{
		char* d = dest;
		const char* s = src;
		unsigned int i;
		for (i = 0; i < n; i++)
		{
			if (*(s + i) == 0)
			{
				*(d + i) = '\0';
				return dest;
			}
			if (*(s + i) & 0x80)
			{
				*(d + i) = *(s + i);
				i++;
				if (i >= n)
				{
					*(d + i - 1) = '\0';
					break;
				}
				*(d + i) = *(s + i);
			}
			else
				*(d + i) = *(s + i);
		}
	}
	return dest;
}

void strcpysafe(char* dest, size_t n, const char* src)
{
	if (!src)
	{
		*dest = '\0';
		return;
	}
	if (n <= 0)
		return;
	else if (n < strlen(src) + 1)
	{
		strncpy2(dest, src, n - 1);
		dest[n - 1] = '\0';
	}
	else
		strcpy(dest, src);
}

void strncpysafe(char* dest, const size_t n, const char* src, const int length)
{
	unsigned int Short;
	Short = min(strlen(src), (unsigned int)length);
	if (n < Short + 1)
	{
		strncpy2(dest, src, n - 1);
		dest[n - 1] = '\0';
	}
	else if (n <= 0)
	{
		return;
	}
	else
	{
		strncpy2(dest, src, Short);
		dest[Short] = '\0';
	}
}

BOOL Autil::getStringFromIndexWithDelim_body(char* src, char* delim, int index, char* buf, int buflen)
{
	int i;
	int length = 0;
	int addlen = 0;
	int oneByteMode = 0;

	if (strlen(delim) == 1)
	{
		oneByteMode = 1;
	}
	for (i = 0; i < index; i++)
	{
		char* last;
		src += addlen;

		if (oneByteMode)
		{
			last = ScanOneByte(src, delim[0]);
		}
		else
		{
			last = strstr(src, delim);
		}
		if (last == NULL)
		{
			strcpysafe(buf, buflen, src);
			if (i == index - 1)
			{
				if (buf[0] == 0)
					return FALSE;
				return TRUE;
			}
			buf[0] = 0;
			return FALSE;
		}
		length = last - src;
		addlen = length + strlen(delim);
	}
	strncpysafe(buf, buflen, src, length);
	if (buf[0] == 0)
		return FALSE;
	return TRUE;
}

int Autil::shiftReadBuf(int size)
{
	int i;

	if (size > g_net_readbuflen)
		return -1;
	for (i = size; i < g_net_readbuflen; i++)
	{
		g_net_readbuf[i - size] = g_net_readbuf[i];
	}
	g_net_readbuflen -= size;
	return 0;
}

int Autil::getLineFromReadBuf(char* output, int maxlen)
{
	if (output)
	{
		int i;

		int j;
		for (i = 0; i < g_net_readbuflen && i < (maxlen - 1); i++)
		{
			if (g_net_readbuf[i] == '\n')
			{
				memcpy(output, g_net_readbuf.get(), i);
				output[i] = '\0';
				for (j = i + 1; j > 0; j--)
				{
					if (output[j] == '\r')
					{
						output[j] = '\0';
						break;
					}
				}

				shiftReadBuf(i + 1);
				g_net_readbuf[g_net_readbuflen] = '\0';

				return 0;
			}
		}
	}
	return -1;
}