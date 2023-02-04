#pragma once
#ifndef STDAFX_H
#define STDAFX_H
#pragma execution_character_set("utf-8")
#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <winsock2.h>
#include <windows.h>
#include <unordered_map>
#include <process.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <iterator>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <memory>
#include <functional>
#include <timeapi.h>
#include <string>
#include <thread>
#if _MSVC_LANG > 201703L
#include <ranges>
#endif

#include <QtCore/qglobal.h>
#include <QApplication>
#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QPoint>
#include <QHash>
#include <QMessageBox>
#include <QVector>
#include <QTextCodec>
#include <QCloseEvent>
#include <QShowEvent>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QShortcut>
#include <QTranslator>
#include <QTextBrowser>
#include <QLabel>
#include <QObject>

#if __has_cpp_attribute(nodiscard)
#  undef MY_REQUIRED_RESULT
#  define MY_REQUIRED_RESULT [[nodiscard]]
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")

#define MINT_USE_SEPARATE_NAMESPACE
#define DETOUR_USE_MICROSOFT_DETOURLIB

#ifdef MINT_USE_SEPARATE_NAMESPACE
#include "MINT.h"
#pragma comment(lib, "ntdll.lib")
#endif
#include <shared_mutex>

#ifdef DETOUR_USE_MICROSOFT_DETOURLIB
#include "detours.h"
#pragma comment(lib, "detours.lib")
#endif

#pragma comment(lib, "Winmm.lib")

#include "log.hpp"

#ifndef SOL_ALL_SAFETIES_ON
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#pragma comment(lib, "lua-5.4.4.lib")
#endif

#define DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

#define DISABLE_COPY_MOVE(Class) \
    DISABLE_COPY(Class) \
    DISABLE_MOVE(Class) \
public:\
	static Class& get_instance() {\
		static Class instance;\
		return instance;\
	}\

#define STATICINS(Class) Class& g_##Class = Class::get_instance()

constexpr const char* LOCAL_LANG = "gb2312";


static constexpr int __stdcall percent(const int minimum, const int maximum)
{
	const int ret = static_cast<int>(((maximum) > (0)) ? floor(((double)(minimum)) * (100.0) / ((double)(maximum))) : (0));
	return (((minimum) > (0)) && ((0) == (ret))) ? 1 : (ret);
}

template<class T, class T2>
void __stdcall MomoryMove(T dis, T2* src, size_t size)
{
	DWORD dwOldProtect = 0;
	VirtualProtect((void*)dis, size, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	memcpy((void*)dis, (void*)src, size);
	VirtualProtect((void*)dis, size, dwOldProtect, &dwOldProtect);
}

template <class ToType, class FromType>
void __stdcall getFuncAddr(ToType* addr, FromType f)
{
	union
	{
		FromType _f;
		ToType   _t;
	}ut{};

	ut._f = f;

	*addr = ut._t;
}

template<class T, class T2>
void __stdcall detour(T pfnHookFunc, DWORD bOri, T2* bOld, T2* bNew, const int nsize, const DWORD offest)
{
	DWORD hookfunAddr = 0;
	getFuncAddr(&hookfunAddr, pfnHookFunc);
	DWORD dwOffset = (hookfunAddr + offest) - (DWORD)bOri - nsize;
	memmov((DWORD)&bNew[nsize - 4], &dwOffset, sizeof(dwOffset));
	memmov((DWORD)bOld, (void*)bOri, nsize);
	memmov((DWORD)bOri, bNew, nsize);
}

static BOOL __stdcall EnableDebugPrivilege(HANDLE hProcess)
{
	if (!hProcess) return FALSE;
	HANDLE hToken;
	//BOOL fOk = FALSE;
	if (MINT::NtOpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp = {};
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);

		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		MINT::NtAdjustPrivilegesToken(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

		//fOk = (My_GetLastError() == ERROR_SUCCESS);
		MINT::NtClose(hToken);
	}
	return TRUE;//fOk;
}

static HANDLE __stdcall My_NtOpenProcess(DWORD dwProcess)
{
	using namespace MINT;
	HANDLE ProcessHandle = NULL;
	OBJECT_ATTRIBUTES ObjectAttribute = {
		sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0 };
	CLIENT_ID ClientId = {};

	InitializeObjectAttributes(&ObjectAttribute, 0, 0, 0, 0);
	ClientId.UniqueProcess = (PVOID)dwProcess;
	ClientId.UniqueThread = (PVOID)0;

	if (NT_SUCCESS(NtOpenProcess(&ProcessHandle, MAXIMUM_ALLOWED,
		&ObjectAttribute, &ClientId)))
	{
		return ProcessHandle;
	}

	return 0;
};

static HANDLE __stdcall My_ZwOpenProcess(DWORD dwProcess)
{
	using namespace MINT;
	HANDLE ProcessHandle = (HANDLE)0;
	OBJECT_ATTRIBUTES ObjectAttribute = {
		sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0 };
	ObjectAttribute.Attributes = NULL;
	CLIENT_ID ClientIds = {};
	ClientIds.UniqueProcess = (HANDLE)dwProcess;
	ClientIds.UniqueThread = (HANDLE)0;
	ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttribute,
		&ClientIds);

	return ProcessHandle;
};

static HANDLE __stdcall openProcess(DWORD dwProcess)
{
	HANDLE hprocess = NULL;
	hprocess = My_NtOpenProcess(dwProcess);
	if (hprocess == NULL)
	{
		hprocess = My_ZwOpenProcess(dwProcess);
		if (hprocess == NULL)
		{
			hprocess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcess);
			if (hprocess == NULL)
			{
				return 0;
			}
		}
	}
	return hprocess;
}

static LONG __stdcall closeHandle(HANDLE process)
{
	if (!process) return 0;
	return MINT::NtClose(process);
}

template<class T, class T2>
BOOL __stdcall write(T desiredAccess, T2* buffer, size_t dwSize)
{
	using namespace MINT;
	HANDLE hProcess = openProcess(_getpid());
	if (!hProcess) return FALSE;
	ULONG oldProtect = NULL;
	EnableDebugPrivilege(hProcess);
	VirtualProtectEx(hProcess, (LPVOID)desiredAccess, dwSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = NT_SUCCESS(NtWriteVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), (PVOID)buffer, dwSize, NULL));
	VirtualProtectEx(hProcess, (LPVOID)desiredAccess, dwSize, oldProtect, &oldProtect);
	closeHandle(hProcess);
	return ret;
}

template<class T>
void  __stdcall undetour(T ori, BYTE* oldBytes, SIZE_T size)
{
	memmov(ori, oldBytes, size);
}

static bool IsNumber(std::string str)
{
	for (char const& c : str) {
		if (c == '+' || c == '-') continue;
		if (std::isdigit(c) == 0) return false;
	}
	return true;
}

static bool isNumber(const std::wstring& str)
{
	for (wchar_t const& c : str) {
		if (std::isdigit(c) == 0) return false;
	}
	return true;
}

static std::wstring ExePath() {
	WCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

static QString __stdcall toUnicode(const char* str)
{
	QTextCodec* codec = QTextCodec::codecForName(LOCAL_LANG);
	return codec->toUnicode(str);
}

static std::string __stdcall fromUnicode(const QString& str)
{
	QTextCodec* codec = QTextCodec::codecForName(LOCAL_LANG);
	return codec->fromUnicode(str).data();
}



//根據 QVector<int> 返回最小值索引
Q_REQUIRED_RESULT inline static int lowestIndex(const QVector<int>& v)
{
#if _MSVC_LANG > 201703L
	const int* p = std::ranges::min_element(v);
#else
	const int* p = std::min_element(v.begin(), v.end());
#endif
	return p - v.begin();
}

//根據 QVector<int> 返回最小值
Q_REQUIRED_RESULT inline static int lowest(const QVector<int>& v)
{
#if _MSVC_LANG > 201703L
	const int* p = std::ranges::min_element(v);
#else
	const int* p = std::min_element(v.begin(), v.end());
#endif
	return *p;
}


//IID_IWbem
#include <wbemidl.h>
#include <comdef.h>
#pragma comment(lib, "wbemuuid.lib")
static bool __stdcall GetUniqueId(QString& cpuid)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		cpuid = "Failed to initialize COM library";
		return false;
	}

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
	);

	if (FAILED(hres))
	{
		CoUninitialize();
		cpuid = "Failed to initialize security";
		return false;
	}

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	IWbemLocator* pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres))
	{
		CoUninitialize();
		cpuid = "Failed to create IWbemLocator object";
		return false;
	}

	// Step 4: ---------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices* pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (for example, Kerberos)
		0,                       // Context object 
		&pSvc                    // pointer to IWbemServices proxy
	);

	if (FAILED(hres))
	{
		pLoc->Release();
		CoUninitialize();
		cpuid = "Could not connect";
		return false;
	}

	// Step 5: --------------------------------------------------
// Set security levels on the proxy -------------------------

	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
	);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		cpuid = "Failed to set proxy blanket";
		return false;
	}

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_Processor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		cpuid = "Query for operating system name failed.";
		return false;
	}

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------

	IWbemClassObject* pclsObj;
	ULONG uReturn = 0;

	std::wstring result;
	while (pEnumerator)
	{
		hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp = {};

		// Get the value of the Name property
		hres = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hres))
		{
			result = vtProp.bstrVal;
			VariantClear(&vtProp);
		}

		pclsObj->Release();
	}

	// Cleanup
	// ========
	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();
	CoUninitialize();
	cpuid = QString::fromStdWString(result).simplified();
	return true;
	}



#endif