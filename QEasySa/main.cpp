#pragma execution_character_set("utf-8")
#include "gameservice.h"
#include "mainform.h"

#include "model/qmfcapp.h"
#include "model/qwinwidget.h"

#ifndef BUILD_STATIC
# if defined(QEASYSA_LIB)
#  define QEASYSA_EXPORT Q_DECL_EXPORT
# else
#  define QEASYSA_EXPORT Q_DECL_IMPORT
# endif
#else
# define QEASYSA_EXPORT
#endif

//#define DEBUG_OUTPUT_ON

extern HINSTANCE g_hInstance;
extern WNDPROC g_hOldProc;
extern HANDLE g_hThread;
extern HWND g_hWnd;
extern HMODULE g_gameModuleBase;
extern std::wstring g_GameExeFilePath;
extern std::shared_ptr<spdlog::logger> g_cont;
extern MainForm* g_main;

extern LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void CreateConsole()
{
	if (!AllocConsole()) {
		// Add some error handling here.
		// You can call GetLastError() to get more info about the error.
		return;
	}

	// std::cout, std::clog, std::cerr, std::cin
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
}

void LoggerInit()
{
	_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
	//setlocale(LC_ALL, "Chinese.GB2312");
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "en_US.UTF-8");
	//setlocale(LC_ALL, "Chinese.GB2312");
	//setlocale(LC_ALL, "chs");
	// create color multi threaded logger

	//my_logger = spdlog::basic_logger_mt<spdlog::async_factory>("ACE", "log/ace_sa.log");
#ifdef DEBUG_OUTPUT_ON
	CreateConsole();
	g_cont = spdlog::stderr_color_mt<spdlog::async_factory>("SA");
	g_cont->set_level(spdlog::level::trace);
	g_cont->flush_on(spdlog::level::info);
#else

#endif

	//spdlog::flush_every(std::chrono::milliseconds(10));
}

using handle_data = struct {
	unsigned long process_id;
	HWND window_handle;
};

BOOL WINAPI IsCurrentWindow(const HWND& handle)
{
	if ((GetWindow(handle, GW_OWNER) == NULL) && (IsWindowVisible(handle)))
		return TRUE;
	else
		return FALSE;
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !IsCurrentWindow(handle))
		return TRUE;
	data.window_handle = handle;
	return FALSE;
}

HWND WINAPI FindCurrentWindow(const unsigned long& process_id)
{
	handle_data data = {};
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows(EnumWindowsCallback, (LPARAM)&data);
	return data.window_handle;
}

HWND WINAPI GetCurrentWindowHandle()
{
	unsigned long process_id = GetCurrentProcessId();
	HWND hwnd = NULL;
	while (!hwnd) { hwnd = FindCurrentWindow(process_id); }
	return hwnd;
}

int ExecMainWindow()
{
	STATICINS(GameService);

	if (!GetUniqueId(g_GameService.g_cpuid))
	{
		g_GameService.g_cpuid.clear();
		return 0;
	}

	try
	{
		QMfcApp::pluginInstance(g_hInstance);
		QWinWidget win((HWND)NULL);
		win.showCentered();
		int argc = 1;
		char argv[MAX_PATH] = {};
		GetModuleFileNameA(NULL, argv, MAX_PATH);
		char* pargv = argv;
		QApplication a(argc, &pargv);
		// 这里创建dll内部的Qt界面例如QWidget
		g_main = q_check_ptr(new MainForm());
		g_main->show();
		return a.exec();
	}
	catch (...)
	{
		return 0;
	}
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpvReserved*/)
{
	static bool ownApplication = FALSE;

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		if (!ownApplication)
		{
			ownApplication = TRUE;
			g_hInstance = hInstance;
			g_hThread = GetCurrentThread();
			//設置啟用高分屏縮放支持
			//要注意開啟後計算到的控件或界面寬度高度可能都不對,全部需要用縮放比例運算下
			QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
			//設置啟用高分屏圖片支持
			QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
			QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
			QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
			LoggerInit();
			QtConcurrent::run(ExecMainWindow);//(_beginthread_proc_type), NULL, nullptr
		}
	}
	if (dwReason == DLL_PROCESS_DETACH && ownApplication)
	{
		STATICINS(GameService);
		g_GameService.uninitialize();
		//spdlog::drop_all();
		//delete qApp;
	}

	return TRUE;
}

extern "C"
{
	__declspec(dllexport) void QEasySa()
	{
		return;
	}
}