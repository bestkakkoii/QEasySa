#include "gameservice.h"
#include "net/autil.h"
#include "functiontypes.h"

extern HANDLE g_hThread;
extern std::shared_ptr<spdlog::logger> g_cont;
extern std::wstring g_cpuid;
extern HWND g_hWnd;

constexpr auto BC_FLG_NEW = (1 << 0);
constexpr auto BC_FLG_DEAD = (1 << 1);		//死亡
constexpr auto BC_FLG_PLAYER = (1 << 2);		//玩家,玩家有異常狀態時要有此值
constexpr auto BC_FLG_POISON = (1 << 3);		//中毒
constexpr auto BC_FLG_PARALYSIS = (1 << 4);		//麻痹
constexpr auto BC_FLG_SLEEP = (1 << 5);		//昏睡
constexpr auto BC_FLG_STONE = (1 << 6);		//石化
constexpr auto BC_FLG_DRUNK = (1 << 7);		//眩暈
constexpr auto BC_FLG_CONFUSION = (1 << 8);		//混亂
constexpr auto BC_FLG_HIDE = (1 << 9);		//是否隱藏，地球一周
constexpr auto BC_FLG_REVERSE = (1 << 10);		//反轉

#define USE_LOCKER

static inline constexpr DWORD __vectorcall CHECK_AND(DWORD a, DWORD b) { return a & b; };

SOCKET WSAAPI New_socket(int af, int type, int protocol)
{
	STATICINS(GameService);
	return g_GameService.New_socket(af, type, protocol);
}

int WSAAPI New_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD  dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	STATICINS(GameService);
	return g_GameService.New_WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
}

int WSAAPI New_closesocket(SOCKET s)
{
	STATICINS(GameService);
	return g_GameService.New_closesocket(s);
}

int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags)
{
	STATICINS(GameService);
	return g_GameService.New_recv(s, buf, len, flags);
}

GameService::GameService()
{
	g_battle_timer.start();
	g_online_timer.start();
	g_echo_timer.start();
	//magic
	m_magic.resize(MAX_MAGIC);

	m_party.resize(MAX_PARTY);

	m_pet.resize(MAX_PET);

	QVector<PET_SKILL> pet(MAX_SKILL);
	QVector<QVector<PET_SKILL>> m_petSkill(MAX_PET);
	for (int i = 0; i < MAX_PET; i++)
	{
		m_petSkill[i] = pet;
	}
}

GameService::~GameService()
{
	if (m_battleWorkFuture.isRunning())
	{
		m_battleWorkFuture.cancel();
		m_battleWorkFuture.waitForFinished();
	}
}

void GameService::AppendLog(const char* str)
{
	g_log_model.append(toUnicode(str));
}

void GameService::AppendLog(const QString& str)
{
	//if (::GetACP() != 950)
	//{
	//	std::wstring wsmsg = str.toStdWString();
	//	char buf[1024] = {};
	//	wchar_t wbuf[1024] = {};
	//	int size = lstrlenW(wsmsg.c_str());
	//	LCMapStringW(LOCALE_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wsmsg.c_str(), size + 1, wbuf, sizeof(wbuf));
	//	QString newstr = QString::fromStdWString(wbuf);
	//	g_log_model.append(newstr);
	//}
	//else
	g_log_model.append(str);
}

void GameService::initialize()
{
	if (m_initialized)
		return;
	m_initialized = true;

	g_hGameBase = (HMODULE)GetModuleHandle(NULL);

	//system
	g_net_socket = CONVERT_GAMEVAR(int*, 0x29CE7400);
	g_net_personalKey = CONVERT_GAMEVAR(char*, 0x19171C);//封包加解密密鑰緩存//0x19171C   //位於xgsa.exe+EF70


	g_player_xpos = CONVERT_GAMEVAR(int*, 0x180D80);
	g_player_ypos = CONVERT_GAMEVAR(int*, 0x180D80 + 4);

	g_player_xpos2 = CONVERT_GAMEVAR(int*, 0x29C56D00);
	g_player_ypos2 = CONVERT_GAMEVAR(int*, 0x29C4536C);

	g_player_xpos3 = CONVERT_GAMEVAR(int*, 0x29C6D978);
	g_player_ypos3 = CONVERT_GAMEVAR(int*, 0x29C6D978 + 4);

	g_world_state = CONVERT_GAMEVAR(int*, 0x29D1AC1C);
	g_game_state = CONVERT_GAMEVAR(int*, 0x29D1AC10);

	g_switcher_flag = CONVERT_GAMEVAR(WORD*, 0x29D01DA8);

	precv = CONVERT_GAMEVAR(int (WSAAPI*)(SOCKET, char*, int, int), 0x2B863332);

	NET_WriteWalkPos_cgitem = CONVERT_GAMEVAR(void(__cdecl*)(int, int, int, const char*), 0x12E80);

	*CONVERT_GAMEVAR(int*, 0x176DC4) = 14;//加速
	*CONVERT_GAMEVAR(int*, 0x176E34) = 1;//內建加速
	*CONVERT_GAMEVAR(int*, 0x181AD8) = 0;//內建特效開關

	//不知為啥直接用movmem會崩潰所以改用OpenProcess + WriteProcessMemory
	write(CONVERT_GAMEVAR(DWORD, 0xBE95B), "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);//快速切圖


	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_hThread);
	DetourAttach(&(PVOID&)psocket, ::New_socket);
	DetourAttach(&(PVOID&)pWSARecv, ::New_WSARecv);
	//DetourAttach(&(PVOID&)precv, ::New_recv);
	DetourAttach(&(PVOID&)pclosesocket, ::New_closesocket);
	DetourTransactionCommit();
}

void GameService::uninitialize()
{
	if (!m_initialized)
		return;
	m_initialized = false;

	*CONVERT_GAMEVAR(int*, 0x176DC4) = 1;//加速
	*CONVERT_GAMEVAR(int*, 0x176E34) = 7;//內建加速
	*CONVERT_GAMEVAR(int*, 0x181AD8) = 0;//內建特效開關

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_hThread);
	DetourDetach(&(PVOID&)psocket, ::New_socket);
	DetourDetach(&(PVOID&)pWSARecv, ::New_WSARecv);
	//DetourDetach(&(PVOID&)precv, ::New_recv);
	DetourDetach(&(PVOID&)pclosesocket, ::New_closesocket);
	DetourTransactionCommit();
}

SOCKET GameService::New_socket(int af, int type, int protocol)
{
	SOCKET skt = psocket(af, type, protocol);
	//if (!g_net_socket)
	//	g_net_socket = q_check_ptr(new int(INVALID_SOCKET));
	//if (g_net_socket)
	//	*g_net_socket = skt;
	return skt;
}

//closesocket
int GameService::New_closesocket(SOCKET s)
{
	if (*g_net_socket == s)
	{
		IS_ONLINE_FLAG = false;
		IS_AUTO_COMBAT = false;
		SetBattleFlag(false);
		IS_BATTLE_READY_ACT = false;
		IS_ENABLE_IGNORE_DATA = false;
		ClearBattleData();
		ClearCharData();
		ClearExp();
		g_online_timer.restart();
		g_battle_timer.restart();
		g_echo_timer.restart();
	}
	return pclosesocket(s);
}

//recv
int GameService::New_recv(SOCKET sockfd, char* buf, int len, int flags)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	int reveivedBytes = precv(sockfd, buf, len, flags);
	*g_net_socket = sockfd;

	if (reveivedBytes != SOCKET_ERROR)
	{
		memcpy_s(autil->g_net_readbuf.get(), NETBUFSIZ, buf, len);
		autil->g_net_readbuflen = len;
	}

	if (IS_FAST_BATTLE && IS_ENABLE_IGNORE_DATA)
	{
		IS_ENABLE_IGNORE_DATA = FALSE;
		reveivedBytes = 0;
		ZeroMemory(buf, len);
	}

	QScopedArrayPointer <char> net_buffer(new char[MAX_BUFFER]());
	if (autil->getLineFromReadBuf(net_buffer.get(), MAX_BUFFER, MAX_BUFFER - 1) == 0)
	{
		NetDispatchMessage(sockfd, net_buffer.get(), MAX_BUFFER);
	}

	return reveivedBytes;
}

int GameService::New_WSARecv(SOCKET sockfd, LPWSABUF lpBuffers, DWORD  dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags
	, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	int reveivedBytes = pWSARecv(sockfd, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);

	STATICINS(KeyManager);
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));

	if (*g_net_socket != sockfd)
		*g_net_socket = sockfd;

	g_KeyManager.SetKey(g_net_personalKey);

	if (g_KeyManager.isValid())
	{
		int nlen = strnlen_s(lpBuffers->buf, lpBuffers->len);

		if (lpBuffers->len != SOCKET_ERROR)
		{
			memcpy_s(autil->g_net_readbuf.get(), NETBUFSIZ, lpBuffers->buf, nlen);
			autil->g_net_readbuflen = nlen;
		}

		QScopedArrayPointer <char> net_buffer(new char[MAX_BUFFER]());
		while (autil->getLineFromReadBuf(net_buffer.get(), MAX_BUFFER, MAX_BUFFER - 1) == 0)
		{
			NetDispatchMessage(sockfd, net_buffer.get(), MAX_BUFFER);
		}

		ZeroMemory(autil->g_net_readbuf.get(), nlen);

		if (IS_FAST_BATTLE && IS_ENABLE_IGNORE_DATA)
		{
			IS_ENABLE_IGNORE_DATA = FALSE;
			reveivedBytes = 0;
			ZeroMemory(lpBuffers->buf, lpBuffers->len);
		}
	}
	return reveivedBytes;
}

int GameService::Send(int msg, int wParam, int lParam)
{
	DWORD dwResult = NULL;
	SendMessageTimeoutA(g_hWnd, msg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 5000, &dwResult);
	return static_cast<int>(dwResult);
}

//QHash<int, int> g_fieldcount_hash = {
//	{LSSPROTO_XYD_RECV,4},
//	{LSSPROTO_EV_RECV,3},
//	{LSSPROTO_EN_RECV,3},
//	{LSSPROTO_RS_RECV,2},
//	{LSSPROTO_B_RECV,2},
//	{LSSPROTO_TK_RECV,4},
//	{LSSPROTO_MC_RECV,10},
//	{LSSPROTO_M_RECV,7},
//	{LSSPROTO_C_RECV,2},
//	{LSSPROTO_CA_RECV,2},
//	{LSSPROTO_CD_RECV,2},
//	{LSSPROTO_S_RECV,2},
//	{LSSPROTO_KS_RECV,3},
//	{LSSPROTO_SKUP_RECV,2},
//	{LSSPROTO_EF_RECV,4},
//	{LSSPROTO_ECHO_RECV,2},
//	{LSSPROTO_NU_RECV,2},
//	{LSSPROTO_NC_RECV,2},
//	{LSSPROTO_PETST_RECV,3},
//	{LSSPROTO_SPET_RECV,3},
//};

int GameService::NetDispatchMessage(int fd, char* encoded, size_t buflen)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	int		func = 0, fieldcount = 0;
	int		iChecksum = 0, iChecksumrecv = 0;
	QScopedArrayPointer <char> raw(new char[MAX_TINYBUFF]());
	autil->util_DecodeMessage(raw.get(), MAX_TINYBUFF, encoded, buflen);
	autil->util_SplitMessage(raw.get(), MAX_TINYBUFF, SEPARATOR, 1u);

	static auto CHECKFUN = [&func](int cmpfunc)->bool
	{
		//qDebug() << func << cmpfunc << g_fieldcount_hash.value(func, 0) << fieldcount;
		return (func == cmpfunc);
	};

	if (!autil->util_GetFunctionFromSlice(&func, &fieldcount))
	{
		autil->util_DiscardMessage();
		return 1;
	}

#if 0
	if (g_cpuid.compare("BFEBFBFF000306C3") == 0)
		if (IS_DEBUG_MODE)
			AppendLog(QString("func:%1 fieldcount:%2").arg(func).arg(fieldcount));
#endif

	if (CHECKFUN(LSSPROTO_XYD_RECV)/*戰後刷新人物座標、方向2*/)
	{
		int x = 0;
		int y = 0;
		int dir = 0;
		iChecksum += autil->util_deint(2, &x);
		iChecksum += autil->util_deint(3, &y);
		iChecksum += autil->util_deint(4, &dir);

		autil->util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_XYD_recv(fd, x, y, dir);
		autil->util_DiscardMessage();
	}
	else if (CHECKFUN(LSSPROTO_EV_RECV)/*WRAP 4*/)
	{
		int seqno = 0;
		int result = 0;
		iChecksum += autil->util_deint(2, &seqno);
		iChecksum += autil->util_deint(3, &result);
		autil->util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_EV_recv(fd, seqno, result);
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_EN_RECV)/*Battle EncountFlag //開始戰鬥7*/)
	{
		int result = 0;
		int field = 0;
		iChecksum += autil->util_deint(2, &result);
		iChecksum += autil->util_deint(3, &field);
		autil->util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		IS_ENABLE_IGNORE_DATA = IS_FAST_BATTLE;
		lssproto_EN_recv(fd, result, field);
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_RS_RECV)/*戰後獎勵*/)
	{
		QScopedArrayPointer <char> data(new char[MAX_SMALLBUFF]());
		iChecksum += autil->util_destring(2, data.get(), MAX_SMALLBUFF);
		autil->util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_RS_recv(fd, data.get());
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_B_RECV)/*每回合開始的戰場資訊15*/)
	{
		QScopedArrayPointer <char> data(new char[MAX_SMALLBUFF]());
		iChecksum += autil->util_destring(2, data.get(), MAX_SMALLBUFF);
		autil->util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_B_recv(fd, data.get());
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_TK_RECV)/*收到對話36*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_MC_RECV)/*地圖數據更新37*/)
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		int tilesum;
		int objsum;
		int eventsum;
		QScopedArrayPointer <char> data(new char[MAX_SMALLBUFF]());

		iChecksum += autil->util_deint(2, &fl);
		iChecksum += autil->util_deint(3, &x1);
		iChecksum += autil->util_deint(4, &y1);
		iChecksum += autil->util_deint(5, &x2);
		iChecksum += autil->util_deint(6, &y2);
		iChecksum += autil->util_deint(7, &tilesum);
		iChecksum += autil->util_deint(8, &objsum);
		iChecksum += autil->util_deint(9, &eventsum);
		iChecksum += autil->util_destring(10, data.get(), MAX_SMALLBUFF);
		autil->util_deint(11, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}
		QString qstr = toUnicode(data.get());
		int index = qstr.indexOf("\\z");
		if (index != -1)
		{
			qstr = qstr.left(index);
		}
		SetMapName(qstr);
	}
	else if (CHECKFUN(LSSPROTO_M_RECV)/*地圖數據更新2 39*/)
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		QScopedArrayPointer <char> data(new char[MAX_SMALLBUFF]());

		iChecksum += autil->util_deint(2, &fl);
		iChecksum += autil->util_deint(3, &x1);
		iChecksum += autil->util_deint(4, &y1);
		iChecksum += autil->util_deint(5, &x2);
		iChecksum += autil->util_deint(6, &y2);
		iChecksum += autil->util_destring(7, data.get(), MAX_SMALLBUFF);
		autil->util_deint(8, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		SetMapFloor(fl);
	}
	else if (CHECKFUN(LSSPROTO_C_RECV)/*服務端發送的靜態信息，可用於顯示玩家，其它玩家，公交，寵物等信息 41*/)
	{
		QScopedArrayPointer <char> data(new char[MAX_SMALLBUFF]());

		iChecksum += autil->util_destring(2, data.get(), MAX_SMALLBUFF);
		autil->util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_C_recv(fd, data.get());
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_CA_RECV)/*42*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_CD_RECV)/*遊戲物件更新? 43*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_S_RECV)/*更新所有基礎資訊 46*/)
	{
		QScopedArrayPointer <char> data(new char[MAX_SMALLBUFF]());
		iChecksum += autil->util_destring(2, data.get(), MAX_SMALLBUFF);
		autil->util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_S_recv(fd, data.get());
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_KS_RECV)/*寵物更換狀態55*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_SKUP_RECV)/*更新點數 63*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_EF_RECV)/*天氣68*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_CHARLOGIN_RECV)/*成功登入*/)
	{
		IS_ONLINE_FLAG = true;
		g_online_timer.restart();
	}
	else if (CHECKFUN(LSSPROTO_ECHO_RECV)/*伺服器定時ECHO "hoge" 88*/)
	{
		IS_ONLINE_FLAG = true;
		if (g_enable_show_echo_ping)
		{
			g_enable_show_echo_ping = false;
			AppendLog(QString(tr("當前伺服器延遲(Ping): %1 ms")).arg(g_echo_timer.elapsed()));
		}
		else
			AppendLog(QString(tr("伺服器回響")));
	}
	else if (CHECKFUN(LSSPROTO_NU_RECV)/*不知道幹嘛的 90*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_NC_RECV)/*沈默? 101* 戰鬥結束*/)
	{
		int flg = 0;
		iChecksum += autil->util_deint(2, &flg);
		autil->util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv) {
			autil->util_DiscardMessage();
			return 1;
		}

		lssproto_NC_recv(fd, flg);
		autil->util_DiscardMessage();
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_PETST_RECV)/*寵物狀態改變 107*/)
	{

	}
	else if (CHECKFUN(LSSPROTO_SPET_RECV)/*寵物更換狀態115*/)
	{

	}
	else
	{

	}

	autil->util_DiscardMessage();
	return 0;
}

#pragma region RECV
void GameService::lssproto_XYD_recv(int fd, int x, int y, int dir)
{
	if (IS_DEBUG_MODE)
		AppendLog(QString(tr("人物坐標重定位: %1,%2[%3]")).arg(x).arg(y).arg(dir));

	PC pc = GetCharData();
	dir = (dir + 3) % 8;
	pc.dir = dir;
	//*g_player_xpos = x;
	//*g_player_ypos = y;
	//*g_player_xpos2 = x;
	//*g_player_ypos2 = y;
	*g_player_xpos3 = x;
	*g_player_ypos3 = y;
	SetCharData(pc);

	//emit this->UpdateInfo(NOTIFY_STATUS);
	//BattleTotalCost += durationTimer.elapsed();
	//BattleFuture.cancel();
	//BattleFuture.waitForFinished();
}

void GameService::lssproto_EV_recv(int fd, int seqno, int result)
{
	// ????????????????????
	//if (logOutFlag)
	//	return;

	//if (eventWarpSendId == seqno)
	//{
	//	eventWarpSendFlag = 0;
	//	if (result == 0)
	//	{
	//		// ?????????????
	//		redrawMap();
	//		floorChangeFlag = FALSE;
	//		// ???????????
	//		warpEffectStart = TRUE;
	//		warpEffectOk = TRUE;
	//	}
	//}
	//else
	//{
	//	if (eventEnemySendId == seqno)
	//	{
	//		if (result == 0)
	//		{
	//			eventEnemySendFlag = 0;
	//		}
	//		//else
	//		//{
	//			// ??????process.cpp???
	//		//}
	//	}
	//}
}

void GameService::lssproto_EN_recv(int fd, int result, int field)
{
	if (result == 1)
	{
		if (IS_DEBUG_MODE)
			AppendLog(QString(tr("[info]: ---------------------戰鬥開始---------------------")));
		g_battle_timer.restart();
		IS_ONLINE_FLAG = true;
		IS_REWARD_SHOW = true;
		SetBattleFlag(true);
		IS_BATTLE_READY_ACT = false;
		//if (result == 4)
		//	vsLookFlag = 1;
		//else
		//	vsLookFlag = 0;
		//if (result == 6 || result == 2)
		//	eventEnemyFlag = 1;
		//else
		//	eventEnemyFlag = 0;
		//if (field < 0 || BATTLE_MAP_FILES <= field)
		//	BattleMapNo = 0;
		//else
		//	BattleMapNo = field;
		//if (result == 2)
		//	DuelFlag = TRUE;
		//else
		//	DuelFlag = FALSE;
		//if (result == 2 || result == 5)
		//	NoHelpFlag = TRUE;
		//else
		//	NoHelpFlag = FALSE;
		//BattleStatusReadPointer = BattleStatusWritePointer = 0;
		//BattleCmdReadPointer = BattleCmdWritePointer = 0;
#ifdef PK_SYSTEM_TIMER_BY_ZHU
		BattleCliTurnNo = -1;
#endif
		//emit this->UpdateInfo(NOTIFY_STATUS);
		//durationTimer.restart();
		m_BattleServerTurnNo = -1;
		m_BattleTurnReceiveFlag = false;
		qbattle_data_t bt = GetBattleData();
		++bt.total_battle_count;
		SetBattleData(bt);
		if (IS_DEBUG_MODE)
			AppendLog(QString(tr("[fatal]: ********** 第 %1 局 **********")).arg(bt.total_battle_count));
	}
	else {
		//sendEnFlag = 0;
		//duelSendFlag = 0;
		//jbSendFlag = 0;
	}
}

void GameService::lssproto_NC_recv(int fd, int flg)
{
	//AppendLog(QString("flg:%1 IS_BATTLE_READY_ACT:%2").arg(flg).arg(IS_BATTLE_READY_ACT ? "true" : "false"));
	if (flg == 0 && IS_BATTLE_FLAG)
	{
		lssproto_EO_send(fd, 0);
		lssproto_Echo_send(fd, (char*)"????");
		SetBattleFlag(false);
		IS_BATTLE_READY_ACT = false;
		if (IS_DEBUG_MODE)
			AppendLog(QString(tr("[warn]: ---收到NC封包，戰鬥結束---")));
	}
}


//計算最大負重
int GameService::CalcMaxLoad()
{
	PC pc = GetCharData();
	int maxload = 0;
	switch (pc.transmigration) {
	case 0:
		maxload = 3;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		maxload = 3 + pc.transmigration;
		break;
	case 5:
		maxload = 10;
		break;
	case 6:
		maxload = 15;
		break;
	}
	//取腰帶的負重
	char* p, buf[5] = { 0 };
	int i = 0;
	if (!pc.item[5].name.isEmpty()) {
		p = strstr((char*)pc.item[5].memo.data(), "負重");
		if (p == NULL)
			return maxload;
		p += 4;
		while (!(*p >= '0' && *p <= '9'))
			p++;
		while (*p >= '0' && *p <= '9') {
			buf[i] = *p;
			i++;
			p++;
		}
		if (i > 0)
			maxload += atoi(buf);
	}
	return maxload;
}

//自動吃肉
void GameService::AutoEatMeat(const PC& pc)
{
	if (IS_AUTO_EATMEAT)
	{
		for (int i = 0; i < MAX_ITEM; i++)
		{
			int stack = pc.item[i].pile == 0 ? 1 : pc.item[i].pile;
			if (pc.item[i].name.contains("石化"))
			{
				lssproto_DI_send(*g_net_socket, i);
			}
			else if (pc.item[i].name.contains("肉"))
			{
				for (int j = 0; j < stack; ++j)
					lssproto_ID_send(*g_net_socket, i, NULL);//0代表刪除使用後的物品(給人物使用)
			}
		}
	}
}

//自動精靈
void GameService::AutoUseMagicInNormal(const PC& pc)
{
	//查看是否有滋潤的精靈
	QVector<MAGIC> magic = GetMagics();
	int i = 0;
	for (; i < MAX_MAGIC; i++)
	{
		if (!magic.at(i).name.isEmpty() && magic.at(i).name.contains("滋潤的") && magic.at(i).useFlag != 0)
			break;
	}
	//人物平時精靈補血
	if (!IS_BATTLE_FLAG && i < MAX_MAGIC && ((double)pc.hp / pc.maxhp) * 100 <= 99)
	{
		lssproto_MU_send(*g_net_socket, i, 0);
	}
}

void GameService::AutoUseMagicForPetInNormal(int j, const PET& pet)
{
	//查看是否有滋潤的精靈
	int i = 0;
	QVector<MAGIC> magic = GetMagics();
	for (; i < MAX_MAGIC; i++)
	{
		if (!magic.at(i).name.isEmpty() && magic.at(i).name.contains("滋潤的") && magic.at(i).useFlag != 0)
			break;
	}
	//寵物平時精靈補血
	if (i < MAX_MAGIC)
	{
		if (!IS_BATTLE_FLAG && pet.maxHp > 0 && ((double)pet.hp / pet.maxHp) * 100 <= 99)
		{
			lssproto_MU_send(*g_net_socket, i, j + 1);
		}
	}
}

void GameService::lssproto_RS_recv(int fd, char* command)
{
	lssproto_EO_send(fd, 0);
	if (IS_REWARD_SHOW)
	{
		IS_REWARD_SHOW = false;
	}
	else
		return;

	Util util;
	//-2|0|3,0|0|4,0|0|0,,,|||
	//-2|人物是否升級|獲得經驗,第幾只寵物|是否升級|獲得經驗,第幾只寵物|是否升級|獲得經驗,,,獲得物品1|獲得物品2|獲得物品3|獲得物品4
	QString reward = toUnicode(command).simplified();
	if (reward.isEmpty()) return;

	int item_index = reward.lastIndexOf(",,,");
	QString szRewardItem = reward.mid(item_index + 3);
	reward = reward.left(item_index);

	util.Tokenize(reward, "|"); //-2;
	bool ischar_leveup = util.Tokenize(reward, "|").toInt() == 1;
	QString szchexp = util.Tokenize(reward, ",").simplified();
	int char_exp = util.a62toi(szchexp);
	struct PET_REWARD
	{
		int pet_index[3] = {};
		int ispet_leveup[3] = {};
		int pet_exp[3] = {};
	};

	PET_REWARD pet_reward = {};
	QStringList rewardPets = reward.split(",", Qt::SkipEmptyParts);
	int n = 0;
	for (const QString& it : rewardPets)
	{
		QStringList Pets = it.split("|", Qt::SkipEmptyParts);
		if (Pets.size() != 3)
			continue;
		pet_reward.pet_index[n] = Pets[0].toInt();
		pet_reward.ispet_leveup[n] = Pets[1].toInt();
		pet_reward.pet_exp[n] = util.a62toi(Pets[2].simplified());
		++n;
		if (n >= 3)
			break;
	}
	QStringList items = szRewardItem.split("|", Qt::SkipEmptyParts);

	EXP exp = GetExp();
	exp.current = char_exp;
	exp.total += char_exp;
	SetExp(exp);

	QString szreward = QString(tr("人物經驗:%1%2")).arg(char_exp).arg(ischar_leveup ? " (UP)" : "");
	for (int i = 0; i < 3; ++i)
	{
		if (pet_reward.pet_exp[i] <= 0)
			continue;
		szreward += QString(tr(" 戰寵[%1]經驗:%2%3")).arg(pet_reward.pet_index[i]).arg(pet_reward.pet_exp[i]).arg(pet_reward.ispet_leveup[i] ? " (UP)" : "");
	}

	if (items.size())
		szreward += QString(tr(" 掉落物品:%1")).arg(items.join(" "));

	QString szexp = QString("[info]: %1").arg(szreward);
	AppendLog(szexp);
	if (IS_DEBUG_MODE)
		AppendLog("");


	//自動吃肉
	PC pc = GetCharData();
	AutoEatMeat(pc);
	SetBattleFlag(false);



	//自動堆疊
	//for (int i = MAX_ITEM - 1; i > 9; i--)
	//{
	//	for (int j = 9; j < i; j++)
	//	{
	//		if (pc.item[i].name > 0 && ((pc.item[i].sendFlag >> 2) & 1) && pc.item[i].name.compare(pc.item[j].name) == 0 && pc.item[j].pile < CalcMaxLoad())
	//		{
	//			lssproto_MI_send(fd, i, j);
	//			return;
	//		}
	//	}
	//}

	//if (!m_battleWorkFuture.isRunning())
	//{
	//	m_battleWorkFuture = QtConcurrent::run([this, fd]() {
	//		if (!IS_BATTLE_FLAG)
	//		{
	//			QThread::msleep(3000);
	//			if (!IS_BATTLE_FLAG)
	//			{
	//				lssproto_EO_send(fd, 0);
	//				lssproto_Echo_send(fd, (char*)"????");
	//				lssproto_Echo_send(fd, (char*)"!!!!");
	//			}
	//		}
	//		}
	//	);
	//}
}

//根據當前腳色位置獲取我方和敵方在戰場上的位置範圍 A0~9 E10~19 或 A10~19 E0~9
inline int GameService::_BATTLE_GetPositionIndexRange(int charposition, int* a_min, int* a_max, int* e_min, int* e_max)
{
	if (!a_min || !a_max || !e_min || !e_max) return -1;

	if (charposition < 0 || charposition >= MAX_ENEMY) return -1;
	if ((charposition >= 0) && (charposition <= 9))//人物在正常位置
	{
		*a_min = 0;
		*a_max = 9;
		*e_min = 10;
		*e_max = 19;
		return 0;//正位
	}
	else if ((charposition >= 10) && (charposition <= MAX_ENEMY))//人物在逆側
	{
		*a_min = 10;
		*a_max = 19;
		*e_min = 0;
		*e_max = 9;
		return 1;//逆位
	}
	return -1;
}

//BC|戰場屬性（0:無屬性,1:地,2:水,3:火,4:風）|人物在組隊中的位置|人物名稱|人物稱號|人物形象編號|人物等級(16進制)|當前HP|最大HP|人物狀態（死亡，中毒等）|是否騎乘標志(0:未騎，1騎,-1落馬)|騎寵名稱|騎寵等級|騎寵HP|騎寵最大HP|戰寵在隊伍中的位置|戰寵名稱|未知|戰寵形象|戰寵等級|戰寵HP|戰寵最大HP|戰寵異常狀態（昏睡，死亡，中毒等）|0||0|0|0|
//敵1位置|敵1名稱|未知|敵1形象|敵1等級|敵1HP|敵1最大HP|敵人異常狀態（死亡，中毒等）|0||0|0|0|
void GameService::Parse_BC_StatusString(QString& data)
{
	Util util;
	do
	{
		qbattle_data_t _battle = GetBattleData();
		PC pc = GetCharData();

		_battle.obj.clear();
		_battle.obj.resize(MAX_ENEMY);
		_battle.allie.clear();
		_battle.allie_front.clear();
		_battle.allie_back.clear();
		_battle.enemy.clear();
		_battle.enemy_front.clear();
		_battle.enemy_back.clear();

		_battle.reserved = data;//(readString(GetGameLibraryModule() + MAKEADDR(0x5, 0x8, 0xA, 0x0, 0x2, 0x0), 2048, true));// bluecg.dll+0x58A020;;
		if (_battle.reserved.isEmpty()) break;

		//BC|0|0|QMutex||18A8D|5F|217|217|5|0|0|0|1|貝魯卡|57|330|330|F|龜之盾||187AF|6|35|35|1|0|0|0|0||0|0|0|10|龜之盾||187AF|5|30|30|1|0|0|0|0||0|0|0|11|龜之盾||187AF|4|2B|2B|1|0|0|0|0||0|0|0|12|龜之盾||187AF|6|34|34|1|0|0|0|0||0|0|0|13|龜之盾||187AF|6|33|33|1|0|0|0|0||0|0|0|
		//BC|0|0|QMutex||18A8D|5F|217|217|5|0|0|0|1|貝魯卡|57|330|330  |5|邦奇諾||187C2|61|3D5|3D5|1 |0|0|0|0||0|0|0     |F|昆伊||187C9|5|29|29|1|0|0|0|0||0|0|0|10|昆伊||187C9|3|1F|1F|1|0|0|0|0||0|0|0|
		while (data.size())
		{
			qbattle_object_t btinfo = {};
			bool ok = false;
			btinfo.pos = util.Tokenize(data, "|").toInt(&ok, 16);
			if (!ok) continue;
			if (btinfo.pos >= MAX_ENEMY) continue;


			btinfo.act = 0;//重製動作
			btinfo.name = util.Tokenize(data, "|");
			btinfo.freeName = util.Tokenize(data, "|");
			btinfo.model = util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.level = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.hp = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.maxhp = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.hp_percent = percent(btinfo.hp, btinfo.maxhp);
			btinfo.status = (DWORD)util.Tokenize(data, "|").toUInt(nullptr, 16);
			btinfo.unknown0 = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.unknown1 = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.unknown2 = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.isride = (int)util.Tokenize(data, "|").toInt(nullptr, 16);//是否騎乘標志(0:未騎，1騎,-1落馬)
			btinfo.ridepetname = util.Tokenize(data, "|");
			btinfo.ridepetlevel = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.ridepethp = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			btinfo.ridepetmaxhp = (int)util.Tokenize(data, "|").toInt(nullptr, 16);
			//unk0:%8, unk1:%9, unk2:%10, 
			if (btinfo.pos >= 0 && btinfo.pos <= 9 && IS_DEBUG_MODE)
			{
				QString log = QString(tr("[我方] 位置:%1, 名:%2, 等級:%4, 血:%5, 最大血:%6, 狀態:%7, 騎:%11, 騎名:%12, 騎等:%13, 騎血:%14, 騎最大血:%15"))
					.arg(btinfo.pos) \
					.arg(btinfo.name) \
					.arg(btinfo.level) \
					.arg(btinfo.hp) \
					.arg(btinfo.maxhp) \
					.arg("0x" + QString::number(btinfo.status, 16)) \
					.arg(btinfo.isride) \
					.arg(btinfo.ridepetname) \
					.arg(btinfo.ridepetlevel) \
					.arg(btinfo.ridepethp) \
					.arg(btinfo.ridepetmaxhp);
				AppendLog(log);
			}

			btinfo.exist = btinfo.level > 0 && btinfo.maxhp > 0 && btinfo.model > 0;

			_battle.obj[btinfo.pos] = btinfo;


			if (btinfo.name == pc.name)
			{
				int pos = *CONVERT_GAMEVAR(int*, 0x2EA93C0);
				_battle.char_position = pos > 0 ? pos : btinfo.pos;
			}

			if (btinfo.pos == _battle.char_position)//ch.name == btinfo.name && btinfo.maxhp > 0 && 
			{
				_battle.side = _BATTLE_GetPositionIndexRange(_battle.char_position, &_battle.allie_min, &_battle.allie_max, &_battle.enemy_min, &_battle.enemy_max);


				pc.hp = btinfo.hp;
				pc.maxhp = btinfo.maxhp;
				pc.hp_percent = percent(pc.hp, pc.maxhp);

				pc.level = btinfo.level;

				_battle.char_hp = pc.hp;
				_battle.char_maxhp = pc.maxhp;
				_battle.char_hp_percent = pc.hp_percent;

				SetCharData(pc);
			}

			if ((btinfo.pos >= _battle.allie_min) && (btinfo.pos <= _battle.allie_max))
			{
				if (btinfo.pos < (_battle.allie_max - BATTLE_MAX_UNIT_PER_ROW))
					_battle.allie_back.append(btinfo);//我方後排
				else
					_battle.allie_front.append(btinfo);//我方前排
				_battle.allie.append(btinfo);
			}
			else
			{
				if (btinfo.pos < (_battle.enemy_max - BATTLE_MAX_UNIT_PER_ROW))
					_battle.enemy_back.append(btinfo);//敵方後排
				else
					_battle.enemy_front.append(btinfo);//敵方前排
				_battle.enemy.append(btinfo);
			}
		}

		//戰場動態中的寵物索引
		_battle.pet_position = [&]()->int {
			if (_battle.char_position < 0)//人物索引不正確 直接返回
				return -1;
			//人物索引在右側後排 或 人物索引在 左側後排
			if ((_battle.char_position < BATTLE_MAX_UNIT_PER_ROW)
				|| ((_battle.char_position >= (BATTLE_MAX_UNIT_PER_ROW * 2)) && (_battle.char_position < (MAX_ENEMY - BATTLE_MAX_UNIT_PER_ROW))))//人物在後排
			{
				return _battle.char_position + BATTLE_MAX_UNIT_PER_ROW;//則寵物在前排 寵物索引 = 人物索引 + 單排單位數量
			}
			//人物索引在右側前排 或 人物索引在 左側前排
			else if (((_battle.char_position >= BATTLE_MAX_UNIT_PER_ROW) && (_battle.char_position < (BATTLE_MAX_UNIT_PER_ROW * 2)))
				|| ((_battle.char_position >= (MAX_ENEMY - BATTLE_MAX_UNIT_PER_ROW)) && (_battle.char_position <= MAX_ENEMY)))//人物在前排
			{
				return _battle.char_position - BATTLE_MAX_UNIT_PER_ROW;//則寵物在後排 寵物索引 = 人物索引 - 單排單位數量
			}
			return -1;
		}();

		if (((_battle.pet_position >= 0) && (_battle.pet_position < _battle.obj.size()))
			&& (_battle.obj.at(_battle.pet_position).level > 0) && (_battle.obj.at(_battle.pet_position).maxhp > 0))
		{
			int index = _battle.pet_position;
			_battle.pet_hp = _battle.obj.at(index).hp;
			_battle.pet_maxhp = _battle.obj.at(index).maxhp;
			//_battle.pet_mp = _battle.obj.at(index).mp;
			//_battle.pet_maxmp = _battle.obj.at(index).maxmp;
			//_battle.pet_mp_percent = percent(_battle.obj.at(index).mp, _battle.obj.at(index).maxmp);
			_battle.pet_hp_percent = percent(_battle.pet_hp, _battle.pet_maxhp);
		}

		_battle.allie_count = _battle.allie.size();
		_battle.allie_front_count = _battle.allie_front.size();
		_battle.allie_back_count = _battle.allie_back.size();
		_battle.enemy_count = _battle.enemy.size();
		_battle.enemy_front_count = _battle.enemy_front.size();
		_battle.enemy_back_count = _battle.enemy_back.size();
		_battle.total_count = _battle.allie_count + _battle.enemy_count;

		QVector<int> enemy_level;
		for (const qbattle_object_t& it : _battle.enemy)
			enemy_level.append(it.level);
		_battle.lowest_enemy_level = lowest(enemy_level);
		if (_battle.enemy.size() > 0)
		{
			int low = lowestIndex(enemy_level);
			if (low >= 0 && low < _battle.enemy.size())
				_battle.lowest_enemy_level_index = _battle.enemy.at(low).pos;
		}

		for (int i = 0; i < MAX_ENEMY; ++i)
		{
			_battle.obj[i].pos = i;
		}



		//由前排到後排 由右至左
		//13 11 10 12 14 敵後
		//18 16 15 17 19 敵前
		//8 6 5 7 9 我後
		//3 1 0 2 4 我前
		static auto cmp = [](const qbattle_object_t& a, const qbattle_object_t& b)->bool {
			const int* a_it = std::find(std::begin(full_target_order), std::end(full_target_order), a.pos);
			const int* b_it = std::find(std::begin(full_target_order), std::end(full_target_order), b.pos);
			//以索引的位置排序
			return a_it < b_it;
		};

		if (_battle.allie.size())
			std::sort(_battle.allie.begin(), _battle.allie.end(), cmp);
		if (_battle.allie_front.size())
			std::sort(_battle.allie_front.begin(), _battle.allie_front.end(), cmp);
		if (_battle.allie_back.size())
			std::sort(_battle.allie_back.begin(), _battle.allie_back.end(), cmp);

		if (_battle.enemy.size())
			std::sort(_battle.enemy.begin(), _battle.enemy.end(), cmp);
		if (_battle.enemy_front.size())
			std::sort(_battle.enemy_front.begin(), _battle.enemy_front.end(), cmp);
		if (_battle.enemy_back.size())
			std::sort(_battle.enemy_back.begin(), _battle.enemy_front.end(), cmp);

		SetBattleData(_battle);


		if (IS_DEBUG_MODE)
		{
			AppendLog(QString(tr("人物:%1 戰寵:%2 我方數量:%3 敵方數量:%4 方向:%5, 我方最小:%6, 我方最大:%7, 敵方最小:%8, 敵方最大:%9")) \
				.arg(_battle.char_position).arg(_battle.pet_position).arg(_battle.allie.size()).arg(_battle.enemy.size()) \
				.arg(_battle.side).arg(_battle.allie_min).arg(_battle.allie_max).arg(_battle.enemy_min).arg(_battle.enemy_max));
		}
	} while (false);
}

void GameService::lssproto_B_recv(int fd, char* command)
{
	Util util;
	int i = 0, j = 0;
	QString data = toUnicode(command);

	//BC|0|0|QMutex||18A8D|5F|217|217|5|0|0|0|1|貝魯卡|57|330|330|F|龜之盾||187AF|6|35|35|1|0|0|0|0||0|0|0|10|龜之盾||187AF|5|30|30|1|0|0|0|0||0|0|0|11|龜之盾||187AF|4|2B|2B|1|0|0|0|0||0|0|0|12|龜之盾||187AF|6|34|34|1|0|0|0|0||0|0|0|13|龜之盾||187AF|6|33|33|1|0|0|0|0||0|0|0|
	PC pc = {};
	qbattle_data_t bt = {};
	if (*(command + 1) == 'C')
	{
		//BC|戰場屬性（0:無屬性,1:地,2:水,3:火,4:風）|人物在組隊中的位置|人物名稱|人物稱號|人物形象編號|人物等級(16進制)|當前HP|最大HP|人物狀態（死亡，中毒等）|
		// 是否騎乘標志(0:未騎，1騎,-1落馬)|騎寵名稱|騎寵等級|騎寵HP|騎寵最大HP|
		// 戰寵在隊伍中的位置|戰寵名稱|未知|戰寵形象|戰寵等級|戰寵HP|戰寵最大HP|戰寵異常狀態（昏睡，死亡，中毒等）|0||0|0|0|
		//敵1位置|敵1名稱|未知|敵1形象|敵1等級|敵1HP|敵1最大HP|敵人異常狀態（死亡，中毒等）|0||0|0|0|

		util.Tokenize(data, "|");
		int field_attr = util.Tokenize(data, "|").toInt();
		Parse_BC_StatusString(data);
		bt = GetBattleData();
		pc = GetCharData();
		//檢測戰寵是否休息,被打飛
		int nLockFightPet = 0;
		if (nLockFightPet >= 1 && nLockFightPet <= 5)
		{
			if (bt.pet_position == -1)
			{//被打飛則置位
				int oldpet = pc.battlePetNo;
				if (oldpet >= 0)
					pc.selectPetNo[oldpet] = FALSE;
				pc.battlePetNo = -1;
			}
		}

		//檢測人物是否全部死亡
		int allie_size = bt.allie.size();
		for (i = 0; i < allie_size; ++i)
		{
			if (bt.allie.at(i).exist)
			{
				if ((bt.allie.at(i).pos >= bt.allie_min) && (bt.allie.at(i).hp > 1))
					break;
			}
		}

		//檢查戰鬥是否已結束，敵是否全部死亡
		int enemy_size = bt.enemy.size();
		for (j = 0; j < enemy_size; ++j)
		{
			if (bt.enemy.at(j).exist)
			{
				if ((bt.enemy.at(j).pos >= bt.enemy_min) && (bt.enemy.at(j).hp > 0) || (bt.enemy.at(j).status & BC_FLG_HIDE))
					break;
			}
		}

		if ((i >= allie_size) || (j >= enemy_size))
		{
			//全部死亡置平時狀態
			if (IS_DEBUG_MODE)
			{
				if ((i >= allie_size))
					AppendLog(QString(tr("~~~~~~~~~~~~~~~~~~~~~~ 我方全部陣亡 發送通知 ~~~~~~~~~~~~~~~~~~~~~~ ")));
				else if ((j >= enemy_size))
					AppendLog(QString(tr("~~~~~~~~~~~~~~~~~~~~~~ 敵人全部陣亡 發送通知 ~~~~~~~~~~~~~~~~~~~~~~ ")));
			}

			SetCharData(pc);
			m_BattleTurnReceiveFlag = true;
			return;
		}
		else
		{
			bool bAutoEscape = false;
			if (!bAutoEscape)
			{
				SetBattleFlag(true);
				IS_BATTLE_READY_ACT = false;
			}
			return;
		}
	}

	else if (*(command + 1) == 'P')//BP|人物索引|BP_FLG_JOIN|剩余MP,和求救有關
	{
		bt = GetBattleData();
		pc = GetCharData();

		int BattleMyNo = 0, BattleBpFlag = 0, BattleMyMp = 0;
		sscanf_s(command + 3, "%X|%X|%X", &BattleMyNo, &BattleBpFlag, &BattleMyMp);
		pc.mp = BattleMyMp;
		pc.maxmp = 100;
		pc.mp_percent = percent(pc.mp, pc.maxmp);

		bt.char_position = BattleMyNo;

		bt.char_mp = BattleMyMp;
		bt.char_maxmp = pc.maxmp;
		bt.char_mp_percent = pc.mp_percent;
		SetBattleData(bt);
		SetCharData(pc);
		return;
	}

	else if (*(command + 1) == 'A')
	{
		bt = GetBattleData();

		//當前是第幾回合，BA|endBlt（哪些位置上有敵人及我方哪些位置上已發動了攻擊）|當前是第幾回合
		//敵人前排從上到下依次為(13，11，F，10，12)，敵人後排從上到下為(E,C,A,B,D)
		//我方寵物從上到下依次為（9，7，5，6，8），我方人員從上到下為(4,2,0,1,3)

		int n = 0;
		for (const qbattle_object_t& it : bt.enemy)
		{
			if (it.exist && it.maxhp > 0 && it.hp > 0)
			{
				++n;
			}
		}
		//沒有敵人存活
		if (!n) return;

		int nAnimeFlagValue = 0;
		sscanf_s(command + 3, "%X|%X", &nAnimeFlagValue, &m_BattleServerTurnNo);
		//AppendLog(QString("BA 動畫標誌: %1, 回合:%2 State:%3").arg(nAnimeFlagValue).arg(m_BattleServerTurnNo).arg(pc.state));
		if (!m_BattleTurnReceiveFlag)
		{
			if (IS_DEBUG_MODE)
				AppendLog(QString(tr("[fatal]: 第 %1 場 / 第 %2 回合 ")).arg(bt.total_battle_count).arg(m_BattleServerTurnNo));
			m_BattleClientTurnNo = m_BattleServerTurnNo;
			m_BattleTurnReceiveFlag = true;
		}

		DWORD dif = nAnimeFlagValue & ~m_BattleAnimeFlag;
		m_BattleAnimeFlag = nAnimeFlagValue;

		//本回合我方還未出手,發送進攻指令
		OnFastBattleWork(fd, nAnimeFlagValue);
	}
	else if (*(command + 1) == 'U')//逃跑
	{
		IS_BATTLE_READY_ACT = true;
		if (IS_DEBUG_MODE)
			AppendLog(QString("收到BU封包:%1").arg(toUnicode(command)));
	}
	else
	{
		m_BattleTurnReceiveFlag = false;
		IS_BATTLE_READY_ACT = true;
		if (IS_DEBUG_MODE)
			AppendLog(QString("[警告]: %1 %2").arg(QObject::tr("未知的B類封包:")).arg(toUnicode(command)));
	}

}

//快戰戰鬥動作
void GameService::OnFastBattleWork(int fd, DWORD diff)
{
	if (!IS_FAST_BATTLE) return;
	qbattle_data_t bt = GetBattleData();
	if ((bt.char_position < 0) || (bt.char_position >= bt.obj.size())) return;
	BATTLE_DIALOG_TYPE dialogtype = GetBattleDialogState();
	//PC pc = GetCharData();
	qbattle_object_t CHAR = bt.obj.at(bt.char_position);

	//J|精靈技能編號|施放對象
	//I|物品位置|使用對象（14代表我方全體，15代表敵方全體，0代表自己，5代表寵物）
	//H|攻擊對象編號(人物攻擊)，P|魔法技能編號|施放對象
	//W|寵物技能編號|施放對象
	//G代表人物防禦，E代表人物逃跑，T代表捕獲，S代表換寵
	QVector<int> poses;
	for (int i = bt.allie_max; i >= bt.allie_min; i--)
	{
		if (i < 0 || i >= bt.obj.size()) continue;
		if (CHECK_AND(diff, posset[i]))
		{
			if (bt.obj.at(i).name.isEmpty()) continue;
			if (bt.obj.at(i).act == DOUBLE_YES) continue;
			if (IS_DEBUG_MODE)
			{
				if (i == bt.char_position)
				{
					AppendLog("[info]: " + QObject::tr("收到BA封包 單位 %1[自己:%2] 已出手").arg(bt.obj.at(i).name).arg(i));
				}
				else if (i == bt.pet_position)
				{
					AppendLog("[info]: " + QObject::tr("收到BA封包 單位 %1[戰寵:%2] 已出手").arg(bt.obj.at(i).name).arg(i));
				}
				else
				{
					AppendLog(QObject::tr("收到BA封包 單位 %1[%2] 已出手").arg(bt.obj.at(i).name).arg(i));
				}
			}
			SetBattleActionStatusByPosIndex(i, DOUBLE_YES);
			break;
		}
		else if (bt.obj.at(i).act != YES)
		{
			if ((i == bt.char_position) || ((bt.pet_position >= 0) && (bt.pet_hp > 0) && (i == bt.pet_position)))
				poses.append(i);
		}
	}

	int enemy_pos = [&bt, this]()->int
	{
		for (const int i : full_target_order)
		{
			if (i >= bt.enemy_min && bt.obj.at(i).exist && bt.obj.at(i).maxhp > 0)
			{
				if (IS_DEBUG_MODE)
				{
					AppendLog(QString("%1: Lv:%2 %3[%4] %5/%6(%7%8) valid:%9") \
						.arg(bt.obj.at(i).hp > 0 ? "[info]: 存活敵人" : "[warn]: 死亡敵人") \
						.arg(bt.obj.at(i).level).arg(bt.obj.at(i).name).arg(bt.obj.at(i).pos) \
						.arg(bt.obj.at(i).hp).arg(bt.obj.at(i).maxhp).arg(bt.obj.at(i).hp_percent) \
						.arg("%").arg(bt.obj.at(i).exist ? "合法" : "不合法"));
				}
				if (bt.obj.at(i).hp > 0)
				{
					return i;
				}
			}
		}
		return 19;
	}();

	int possize = poses.size();
	if (!possize) return;

	if (possize > 1)
		std::reverse(poses.begin(), poses.end());
	for (const int i : poses)
	{
		if (i == bt.char_position)
		{
			_BATTLE_CharDoWork(fd, bt, i, enemy_pos);
		}
		else if ((bt.pet_position >= 0) && (bt.pet_hp > 0) && (i == bt.pet_position))
		{
			_BATTLE_PetDoWork(fd, i, enemy_pos);
		}
	}
}

void GameService::_BATTLE_CharDoWork(int fd, const qbattle_data_t& bt, int i, int enemy_pos)
{
	int skill_index = 0;
	int target_index = 0;
	QVector<MAGIC> magic = GetMagics();
	for (; skill_index < MAX_MAGIC; skill_index++)
	{
		if (!magic.at(skill_index).name.isEmpty() && magic.at(skill_index).useFlag != 0)
		{
			if (magic.at(skill_index).name.contains("滋潤的"))
			{
				target_index = SINGLE;
				break;
			}
			else if (magic.at(skill_index).name.contains("恩惠的"))
			{
				target_index = ALL;
				break;
			}
			else if (magic.at(skill_index).name.contains("治癒的"))
			{
				target_index = SELF;
				break;
			}
		}
	}
	if (skill_index >= MAX_MAGIC)
	{
		skill_index = INVALID;
		target_index = INVALID;
	}

	auto checkhp = [&bt, &target_index, this]()->int
	{
		if (IS_AUTO_BATTLE_MAGIC_HEAL)
		{
			for (const qbattle_object_t& it : bt.allie)
			{
				if (SELF == target_index && (it.pos != bt.char_position)) continue;

				if ((it.pos >= bt.allie_min) && (it.pos <= bt.allie_max))
				{
					if (it.exist && (it.hp > 0) && (it.maxhp > 0) && it.hp_percent > 0
						&& (BATTLE_AUTO_MAGIC_HEAL_VALUE > 0)
						&& (it.hp_percent < BATTLE_AUTO_MAGIC_HEAL_VALUE))
						return it.pos;
				}
			}
		}
		return INVALID;
	};

	const int allie_pos = checkhp();
	QString cmd = "\0";

	if (INVALID == allie_pos)
	{
		cmd = "H|";
		cmd += QString::number(enemy_pos, 16).toUpper();
	}
	else
	{
		if (skill_index != INVALID)
		{
			cmd = "J|";
			cmd += QString::number(skill_index, 16).toUpper() + "|";
			if (SINGLE == target_index)
				cmd += QString::number(allie_pos, 16).toUpper();
			else if ((SELF == target_index) && (allie_pos == bt.char_position))
				cmd += "0";
			else if (ALL == target_index)
				cmd += QString::number(target_index, 16).toUpper();
			else
				cmd.clear();
		}
	}

	if (IS_DEBUG_MODE)
		AppendLog(QString(tr("[人物出手]")));
	if (cmd.isEmpty())
	{
		if (IS_DEBUG_MODE)
			AppendLog(QString(tr("[警告]: 出現未預料的錯誤，切換為防禦")));
		cmd = "G";
	}

	if (cmd.startsWith("J"))
	{
		AppendLog(QString(tr("[warn]: 我方[%1(%2)] 當前血量 (%3%4) 低於 %5%6")).arg(bt.obj.at(i).name).arg(bt.obj.at(i).pos) \
			.arg(bt.obj.at(i).hp_percent).arg("%").arg(BATTLE_AUTO_MAGIC_HEAL_VALUE).arg("%"));
		AppendLog(QString(tr("[warn]: 使用精靈:%1")).arg(magic[skill_index].name));
	}
	if (IS_DEBUG_MODE)
	{
		AppendLog(QString(tr("%1人物發送封包: %2(@%3)")).arg(cmd.startsWith("J") ? tr("[warn]: {MAGIC} ") : "").arg(cmd)
			.arg(cmd.startsWith("J") ? allie_pos : enemy_pos));
	}
	std::string scmd = cmd.toStdString();
	lssproto_B_send(fd, scmd);
	SetBattleActionStatusByPosIndex(i, YES);
}

void GameService::_BATTLE_PetDoWork(int fd, int i, int enemy_pos)
{
	if (IS_DEBUG_MODE)
		AppendLog(QString(tr("[戰寵出手]")));
	QString cmd = "W|0|";
	cmd += QString::number(enemy_pos, 16).toUpper();
	if (IS_DEBUG_MODE)
		AppendLog(QString("戰寵發送封包: %1(@%2)").arg(cmd).arg(enemy_pos));
	std::string scmd = cmd.toStdString();
	lssproto_B_send(fd, scmd);
	SetBattleActionStatusByPosIndex(i, YES);
}




//各種遊戲數據
void GameService::lssproto_S_recv(int fd, char* cdata)
{
	Util util;
	/*================================
	C warp 用
	D 修正時間
	X 騎寵
	P 人物狀態
	F 家族狀態
	M HP,MP,EXP
	K 寵物狀態
	E nowEncountPercentage
	J 魔法
	N 隊伍資訊
	I 道具
	W 寵物技能
	S 職業技能
	G 職業技能冷卻時間
	================================*/
	QScopedPointer<PC> pc(q_check_ptr(new PC(GetCharData())));
	const QVector<PARTY> party = GetParties();
	switch (cdata[0])
	{
	case 'C':
	{
		int fl = 0, maxx = 0, maxy = 0, gx = 0, gy = 0;

		//floorChangeFlag = TRUE;
		//if (!loginFlag && ProcNo == PROC_GAME)
		//{
		//	if (!warpEffectFlag)
		//	{
		//		SubProcNo = 200;
		//		if (MenuToggleFlag & JOY_CTRL_M)
		//			MapWmdFlagBak = TRUE;
		//	}
		//	warpEffectFlag = FALSE;
		//	warpEffectStart = TRUE;
		//}
		cdata++;
		QString data = toUnicode(cdata);
		fl = util.getIntegerToken(data, S_DELIM, 1);
		maxx = util.getIntegerToken(data, S_DELIM, 2);
		maxy = util.getIntegerToken(data, S_DELIM, 3);
		gx = util.getIntegerToken(data, S_DELIM, 4);
		gy = util.getIntegerToken(data, S_DELIM, 5);
		SetMapFloor(fl);
		//setMap(fl, gx, gy);
		//nowFloorGxSize = maxx;
		//nowFloorGySize = maxy;
		//mapEmptyFlag = FALSE;
		//nowEncountPercentage = minEncountPercentage;
		//nowEncountExtra = 0;
		//transmigrationEffectFlag = 0;
#ifdef __SKYISLAND
		extern void SkyIslandSetNo(int fl);
		SkyIslandSetNo(fl);
#endif
		break;
	}
	case 'D':
	{
		cdata++;
		QString data = toUnicode(cdata);
		pc->id = util.getIntegerToken(data, S_DELIM, 1);
		m_serverTime = util.getIntegerToken(data, S_DELIM, 2);
		m_FirstTime = GetTickCount64();
		m_SaTimeZoneNo = util.getLSTime(&m_SaTime, m_FirstTime, m_serverTime);
		SetCharData(*pc);
		//PaletteChange(m_SaTimeZoneNo, 0);
		break;
		//andy_add
	}
	case 'X':
	{
		QString data = toUnicode(cdata);
		pc->lowsride = util.getIntegerToken(data, S_DELIM, 2);
		SetCharData(*pc);
		break;
	}
	case 'P':
	{
		QString name, freeName;
		int i = 0, kubun = 0;
		unsigned int mask = 0u;

		cdata++;
		QString data = toUnicode(cdata);
		kubun = util.getInteger62Token(data, S_DELIM, 1);
		//if (!bNewServer)
		//	pc->ridePetNo = -1;

		if (kubun == 1)
		{
			pc->hp = util.getIntegerToken(data, S_DELIM, 2);		// 0x00000002
			pc->maxhp = util.getIntegerToken(data, S_DELIM, 3);		// 0x00000004
			pc->hp_percent = percent(pc->hp, pc->maxhp);
			pc->mp = util.getIntegerToken(data, S_DELIM, 4);		// 0x00000008
			pc->maxmp = util.getIntegerToken(data, S_DELIM, 5);		// 0x00000010
			pc->mp_percent = percent(pc->mp, pc->maxmp);
			pc->vital = util.getIntegerToken(data, S_DELIM, 6);		// 0x00000020
			pc->str = util.getIntegerToken(data, S_DELIM, 7);		// 0x00000040
			pc->tgh = util.getIntegerToken(data, S_DELIM, 8);		// 0x00000080
			pc->dex = util.getIntegerToken(data, S_DELIM, 9);		// 0x00000100
			pc->exp = util.getIntegerToken(data, S_DELIM, 10);		// 0x00000200
			pc->maxExp = util.getIntegerToken(data, S_DELIM, 11);		// 0x00000400
			EXP exp = GetExp();
			exp.left = pc->maxExp - pc->exp;
			SetExp(exp);

			pc->level = util.getIntegerToken(data, S_DELIM, 12);		// 0x00000800
			pc->atk = util.getIntegerToken(data, S_DELIM, 13);		// 0x00001000
			pc->def = util.getIntegerToken(data, S_DELIM, 14);		// 0x00002000
			pc->quick = util.getIntegerToken(data, S_DELIM, 15);		// 0x00004000
			pc->charm = util.getIntegerToken(data, S_DELIM, 16);		// 0x00008000
			pc->luck = util.getIntegerToken(data, S_DELIM, 17);		// 0x00010000
			pc->earth = util.getIntegerToken(data, S_DELIM, 18);		// 0x00020000
			pc->water = util.getIntegerToken(data, S_DELIM, 19);		// 0x00040000
			pc->fire = util.getIntegerToken(data, S_DELIM, 20);		// 0x00080000
			pc->wind = util.getIntegerToken(data, S_DELIM, 21);		// 0x00100000
			pc->gold = util.getIntegerToken(data, S_DELIM, 22);		// 0x00200000
			pc->titleNo = util.getIntegerToken(data, S_DELIM, 23);		// 0x00400000
			pc->dp = util.getIntegerToken(data, S_DELIM, 24);		// 0x00800000
			pc->transmigration = util.getIntegerToken(data, S_DELIM, 25);// 0x01000000
			pc->ridePetNo = util.getIntegerToken(data, S_DELIM, 26);	// 0x02000000
			pc->learnride = util.getIntegerToken(data, S_DELIM, 27);	// 0x04000000
			pc->baseGraNo = util.getIntegerToken(data, S_DELIM, 28);	// 0x08000000
#ifdef _NEW_RIDEPETS
			pc->lowsride = util.getIntegerToken(data, S_DELIM, 29);		// 0x08000000
#endif
#ifdef _SFUMATO
			pc->sfumato = 0xff0000;
#endif
			util.getStringToken(data, S_DELIM, 30, name);
			util.makeStringFromEscaped(name);
			pc->name = name;
			util.getStringToken(data, S_DELIM, 31, freeName);
			util.makeStringFromEscaped(freeName);
			pc->freeName = freeName;

#ifdef _NEW_ITEM_
			pc->道具欄狀態 = util.getIntegerToken(data, S_DELIM, 32);
#endif
#ifdef _SA_VERSION_25
			int pointindex = util.getIntegerToken(data, S_DELIM, 33);
			QStringList pontname = {
				"薩姆吉爾村",
				"瑪麗娜絲村",
				"加加村",
				"卡魯它那村",
			};
			pc->chusheng = pontname[pointindex];
#ifdef _MAGIC_ITEM_
			pc->法寶道具狀態 = util.getIntegerToken(data, S_DELIM, 34);
			pc->道具光環效果 = util.getIntegerToken(data, S_DELIM, 35);
#endif
#endif
			//平時精靈補血
			SetCharData(*pc);
			AutoUseMagicInNormal(*pc);
		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (kubun & mask)
				{
					if (mask == 0x00000002) // ( 1 << 1 )
					{
						pc->hp = util.getIntegerToken(data, S_DELIM, i);// 0x00000002
						i++;
					}
					else if (mask == 0x00000004) // ( 1 << 2 )
					{
						pc->maxhp = util.getIntegerToken(data, S_DELIM, i);// 0x00000004
						i++;
					}
					else if (mask == 0x00000008)
					{
						pc->mp = util.getIntegerToken(data, S_DELIM, i);// 0x00000008
						i++;
					}
					else if (mask == 0x00000010)
					{
						pc->maxmp = util.getIntegerToken(data, S_DELIM, i);// 0x00000010
						i++;
					}
					else if (mask == 0x00000020)
					{
						pc->vital = util.getIntegerToken(data, S_DELIM, i);// 0x00000020
						i++;
					}
					else if (mask == 0x00000040)
					{
						pc->str = util.getIntegerToken(data, S_DELIM, i);// 0x00000040
						i++;
					}
					else if (mask == 0x00000080)
					{
						pc->tgh = util.getIntegerToken(data, S_DELIM, i);// 0x00000080
						i++;
					}
					else if (mask == 0x00000100)
					{
						pc->dex = util.getIntegerToken(data, S_DELIM, i);// 0x00000100
						i++;
					}
					else if (mask == 0x00000200)
					{
						pc->exp = util.getIntegerToken(data, S_DELIM, i);// 0x00000200
						i++;
					}
					else if (mask == 0x00000400)
					{
						pc->maxExp = util.getIntegerToken(data, S_DELIM, i);// 0x00000400
						i++;
					}
					else if (mask == 0x00000800)
					{
						pc->level = util.getIntegerToken(data, S_DELIM, i);// 0x00000800
						i++;
					}
					else if (mask == 0x00001000)
					{
						pc->atk = util.getIntegerToken(data, S_DELIM, i);// 0x00001000
						i++;
					}
					else if (mask == 0x00002000)
					{
						pc->def = util.getIntegerToken(data, S_DELIM, i);// 0x00002000
						i++;
					}
					else if (mask == 0x00004000)
					{
						pc->quick = util.getIntegerToken(data, S_DELIM, i);// 0x00004000
						i++;
					}
					else if (mask == 0x00008000)
					{
						pc->charm = util.getIntegerToken(data, S_DELIM, i);// 0x00008000
						i++;
					}
					else if (mask == 0x00010000)
					{
						pc->luck = util.getIntegerToken(data, S_DELIM, i);// 0x00010000
						i++;
					}
					else if (mask == 0x00020000)
					{
						pc->earth = util.getIntegerToken(data, S_DELIM, i);// 0x00020000
						i++;
					}
					else if (mask == 0x00040000)
					{
						pc->water = util.getIntegerToken(data, S_DELIM, i);// 0x00040000
						i++;
					}
					else if (mask == 0x00080000)
					{
						pc->fire = util.getIntegerToken(data, S_DELIM, i);// 0x00080000
						i++;
					}
					else if (mask == 0x00100000)
					{
						pc->wind = util.getIntegerToken(data, S_DELIM, i);// 0x00100000
						i++;
					}
					else if (mask == 0x00200000)
					{
						pc->gold = util.getIntegerToken(data, S_DELIM, i);// 0x00200000
						i++;
					}
					else if (mask == 0x00400000)
					{
						pc->titleNo = util.getIntegerToken(data, S_DELIM, i);// 0x00400000
						i++;
					}
					else if (mask == 0x00800000)
					{
						pc->dp = util.getIntegerToken(data, S_DELIM, i);// 0x00800000
						i++;
					}
					else if (mask == 0x01000000)
					{
						pc->transmigration = util.getIntegerToken(data, S_DELIM, i);// 0x01000000
						i++;
					}
					else if (mask == 0x02000000)
					{
						util.getStringToken(data, S_DELIM, i, name);// 0x01000000
						util.makeStringFromEscaped(name);
						pc->name = name;
						i++;
					}
					else if (mask == 0x04000000)
					{
						util.getStringToken(data, S_DELIM, i, freeName);// 0x02000000
						util.makeStringFromEscaped(freeName);
						pc->freeName = freeName;
						i++;
					}
					else if (mask == 0x08000000) // ( 1 << 27 )
					{
						pc->ridePetNo = util.getIntegerToken(data, S_DELIM, i);// 0x08000000
						i++;
					}
					else if (mask == 0x10000000) // ( 1 << 28 )
					{
						pc->learnride = util.getIntegerToken(data, S_DELIM, i);// 0x10000000
						i++;
					}
					else if (mask == 0x20000000) // ( 1 << 29 )
					{
						pc->baseGraNo = util.getIntegerToken(data, S_DELIM, i);// 0x20000000
						i++;
					}
					else if (mask == 0x40000000) // ( 1 << 30 )
					{
						pc->skywalker = util.getIntegerToken(data, S_DELIM, i);// 0x40000000
						i++;
					}
#ifdef _CHARSIGNADY_NO_
					else if (mask == 0x80000000) // ( 1 << 31 )
					{
						pc->簽到標記 = util.getIntegerToken(data, S_DELIM, i);// 0x80000000
						i++;
					}
#endif
				}
			}
			EXP exp = GetExp();
			exp.left = pc->maxExp - pc->exp;
			SetExp(exp);
		}
		//emit this->UpdateInfo(NOTIFY_CHARDETAIL);
		//updataPcAct();
		PARTY party = GetParty(0);
		if ((pc->status & CHR_STATUS_LEADER) != 0 && party.useFlag != 0)
		{
			party.level = pc->level;
			party.maxHp = pc->maxhp;
			party.hp = pc->hp;
			party.name = pc->name;
			SetParty(0, party);
		}

		//if (!bNewServer)
		//	pc->ridePetNo = -1;
		/*if ((bNewServer & 0xf000000) == 0xf000000 && sPetStatFlag == 1)
			saveUserSetting();*/
		break;
	}
	case 'F':
	{
		QString familyName;

		cdata++;
		QString data = toUnicode(cdata);
		util.getStringToken(data, S_DELIM, 1, familyName);
		util.makeStringFromEscaped(familyName);
		pc->familyName = familyName;
		pc->familyleader = util.getIntegerToken(data, S_DELIM, 2);
		pc->channel = util.getIntegerToken(data, S_DELIM, 3);
		pc->familySprite = util.getIntegerToken(data, S_DELIM, 4);
		pc->big4fm = util.getIntegerToken(data, S_DELIM, 5);
#ifdef _CHANNEL_MODIFY
		if (pc->familyleader == FMMEMBER_NONE) {
			pc->etcFlag &= ~PC_ETCFLAG_CHAT_FM;
			TalkMode = 0;
		}
#endif
		SetCharData(*pc);
		break;
	}
	// HP,MP,EXP
	case 'M':
	{
		cdata++;
		QString data = toUnicode(cdata);
		pc->hp = util.getIntegerToken(data, S_DELIM, 1);
		pc->mp = util.getIntegerToken(data, S_DELIM, 2);
		pc->exp = util.getIntegerToken(data, S_DELIM, 3);
		EXP exp = GetExp();
		exp.left = pc->maxExp - pc->exp;
		SetExp(exp);
		//updataPcAct();
		PARTY party = GetParty(0);
		if ((pc->status & CHR_STATUS_LEADER) != 0 && party.useFlag != 0)
		{
			party.hp = pc->hp;
			SetParty(0, party);
		}
		SetCharData(*pc);
		break;
	}
	case 'K':
	{
		QString name, freeName;
		int no = 0, kubun = 0, i = 0;
		unsigned int mask = 0u;

		no = cdata[1] - '0';
		if (no < MAX_PET)
		{
			cdata += 3;
			QString data = toUnicode(cdata);
			kubun = util.getInteger62Token(data, S_DELIM, 1);

			PET pet = GetPet(no);
			if (kubun == 0)
			{
				if (pet.useFlag)
				{
					if (no == pc->battlePetNo)
						pc->battlePetNo = -1;
					if (no == pc->mailPetNo)
						pc->mailPetNo = -1;
					pc->selectPetNo[no] = FALSE;
					SetCharData(*pc);
				}
				pet.useFlag = 0;
			}
			else
			{
				pet.useFlag = 1;
				if (kubun == 1)
				{

					pet.graNo = util.getIntegerToken(data, S_DELIM, 2);		// 0x00000002
					pet.hp = util.getIntegerToken(data, S_DELIM, 3);		// 0x00000004
					pet.maxHp = util.getIntegerToken(data, S_DELIM, 4);		// 0x00000008
					pet.mp = util.getIntegerToken(data, S_DELIM, 5);		// 0x00000010
					pet.maxMp = util.getIntegerToken(data, S_DELIM, 6);		// 0x00000020
					pet.exp = util.getIntegerToken(data, S_DELIM, 7);		// 0x00000040
					pet.maxExp = util.getIntegerToken(data, S_DELIM, 8);		// 0x00000080
					pet.level = util.getIntegerToken(data, S_DELIM, 9);		// 0x00000100
					pet.atk = util.getIntegerToken(data, S_DELIM, 10);		// 0x00000200
					pet.def = util.getIntegerToken(data, S_DELIM, 11);		// 0x00000400
					pet.quick = util.getIntegerToken(data, S_DELIM, 12);		// 0x00000800
					pet.ai = util.getIntegerToken(data, S_DELIM, 13);		// 0x00001000
					pet.earth = util.getIntegerToken(data, S_DELIM, 14);		// 0x00002000
					pet.water = util.getIntegerToken(data, S_DELIM, 15);		// 0x00004000
					pet.fire = util.getIntegerToken(data, S_DELIM, 16);		// 0x00008000
					pet.wind = util.getIntegerToken(data, S_DELIM, 17);		// 0x00010000
					pet.maxSkill = util.getIntegerToken(data, S_DELIM, 18);		// 0x00020000
					pet.changeNameFlag = util.getIntegerToken(data, S_DELIM, 19);// 0x00040000
					pet.trn = util.getIntegerToken(data, S_DELIM, 20);
#ifdef _SHOW_FUSION
					pet.fusion = util.getIntegerToken(data, S_DELIM, 21);
					util.getStringToken(data, S_DELIM, 22, name);// 0x00080000
					util.makeStringFromEscaped(name);
					pet.name = name;
					util.getStringToken(data, S_DELIM, 23, freeName);// 0x00100000
					util.makeStringFromEscaped(freeName);
					pet.freeName = freeName;
#else
					util.getStringToken(data, S_DELIM, 21, sizeof(name) - 1, name);// 0x00080000
					util.makeStringFromEscaped(name);
					if (strlen(name) <= PET_NAME_LEN)
						pet[no].name = toUnicode(name);
					util.getStringToken(data, S_DELIM, 22, sizeof(freeName) - 1, freeName);// 0x00100000
					util.makeStringFromEscaped(freeName);
					if (strlen(freeName) <= PET_NAME_LEN)
						pet[no].freeName = toUnicode(freeName);
#endif
#ifdef _PETCOM_
					pet[no].oldhp = util.getIntegerToken(data, S_DELIM, 24);
					pet[no].oldatk = util.getIntegerToken(data, S_DELIM, 25);
					pet[no].olddef = util.getIntegerToken(data, S_DELIM, 26);
					pet[no].oldquick = util.getIntegerToken(data, S_DELIM, 27);
					pet[no].oldlevel = util.getIntegerToken(data, S_DELIM, 28);
#endif
#ifdef _RIDEPET_
					pet[no].rideflg = util.getIntegerToken(data, S_DELIM, 29);
#endif
#ifdef _PETBLESS_
					pet[no].blessflg = util.getIntegerToken(data, S_DELIM, 30);
					pet[no].blesshp = util.getIntegerToken(data, S_DELIM, 31);
					pet[no].blessatk = util.getIntegerToken(data, S_DELIM, 32);
					pet[no].blessdef = util.getIntegerToken(data, S_DELIM, 33);
					pet[no].blessquick = util.getIntegerToken(data, S_DELIM, 34);
#endif

				}
				else
				{
					mask = 2;
					i = 2;
					for (; mask > 0; mask <<= 1)
					{
						if (kubun & mask)
						{
							if (mask == 0x00000002)
							{
								pet.graNo = util.getIntegerToken(data, S_DELIM, i);// 0x00000002
								i++;
							}
							else if (mask == 0x00000004)
							{
								pet.hp = util.getIntegerToken(data, S_DELIM, i);// 0x00000004
								i++;
							}
							else if (mask == 0x00000008)
							{
								pet.maxHp = util.getIntegerToken(data, S_DELIM, i);// 0x00000008
								i++;
							}
							else if (mask == 0x00000010)
							{
								pet.mp = util.getIntegerToken(data, S_DELIM, i);// 0x00000010
								i++;
							}
							else if (mask == 0x00000020)
							{
								pet.maxMp = util.getIntegerToken(data, S_DELIM, i);// 0x00000020
								i++;
							}
							else if (mask == 0x00000040)
							{
								pet.exp = util.getIntegerToken(data, S_DELIM, i);// 0x00000040
								i++;
							}
							else if (mask == 0x00000080)
							{
								pet.maxExp = util.getIntegerToken(data, S_DELIM, i);// 0x00000080
								i++;
							}
							else if (mask == 0x00000100)
							{
								pet.level = util.getIntegerToken(data, S_DELIM, i);// 0x00000100
								i++;
							}
							else if (mask == 0x00000200)
							{
								pet.atk = util.getIntegerToken(data, S_DELIM, i);// 0x00000200
								i++;
							}
							else if (mask == 0x00000400)
							{
								pet.def = util.getIntegerToken(data, S_DELIM, i);// 0x00000400
								i++;
							}
							else if (mask == 0x00000800)
							{
								pet.quick = util.getIntegerToken(data, S_DELIM, i);// 0x00000800
								i++;
							}
							else if (mask == 0x00001000)
							{
								pet.ai = util.getIntegerToken(data, S_DELIM, i);// 0x00001000
								i++;
							}
							else if (mask == 0x00002000)
							{
								pet.earth = util.getIntegerToken(data, S_DELIM, i);// 0x00002000
								i++;
							}
							else if (mask == 0x00004000)
							{
								pet.water = util.getIntegerToken(data, S_DELIM, i);// 0x00004000
								i++;
							}
							else if (mask == 0x00008000)
							{
								pet.fire = util.getIntegerToken(data, S_DELIM, i);// 0x00008000
								i++;
							}
							else if (mask == 0x00010000)
							{
								pet.wind = util.getIntegerToken(data, S_DELIM, i);// 0x00010000
								i++;
							}
							else if (mask == 0x00020000)
							{
								pet.maxSkill = util.getIntegerToken(data, S_DELIM, i);// 0x00020000
								i++;
							}
							else if (mask == 0x00040000)
							{
								pet.changeNameFlag = util.getIntegerToken(data, S_DELIM, i);// 0x00040000
								i++;
							}
							else if (mask == 0x00080000)
							{
								util.getStringToken(data, S_DELIM, i, name);// 0x00080000
								util.makeStringFromEscaped(name);
								pet.name = name;
								i++;
							}
							else if (mask == 0x00100000)
							{
								util.getStringToken(data, S_DELIM, i, freeName);// 0x00100000
								util.makeStringFromEscaped(freeName);
								pet.freeName = freeName;
								i++;
							}
#ifdef _PETCOM_
							else if (mask == 0x200000)
							{
								pet[no].oldhp = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x400000)
							{
								pet[no].oldatk = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x800000)
							{
								pet[no].olddef = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x1000000)
							{
								pet[no].oldquick = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x2000000)
							{
								pet[no].oldlevel = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
#endif
#ifdef _PETBLESS_
							else if (mask == 0x4000000)
							{
								pet[no].blessflg = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x8000000)
							{
								pet[no].blesshp = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x10000000)
							{
								pet[no].blessatk = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x20000000)
							{
								pet[no].blessquick = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
							else if (mask == 0x40000000)
							{
								pet[no].blessdef = util.getIntegerToken(data, S_DELIM, i);
								i++;
							}
#endif
						}
					}
				}

			}
			SetPet(no, pet);
			AutoUseMagicForPetInNormal(no, pet);
			//emit this->UpdateInfo(NOTIFY_PETDETAIL);
		}
		break;
	}
	case 'E':
	{
		cdata++;
		//minEncountPercentage = util.getIntegerToken(data, S_DELIM, 1);
		//maxEncountPercentage = util.getIntegerToken(data, S_DELIM, 2);
		//nowEncountPercentage = minEncountPercentage;
		break;
	}
	case 'J':
	{
		QString name, memo;
		int no = 0;

		no = cdata[1] - '0';
		if (no < MAX_MAGIC)
		{

			cdata += 3;
			QString data = toUnicode(cdata);
			MAGIC magic = GetMagic(no);
			magic.useFlag = util.getIntegerToken(data, S_DELIM, 1);
			if (magic.useFlag != 0)
			{
				magic.mp = util.getIntegerToken(data, S_DELIM, 2);
				magic.field = util.getIntegerToken(data, S_DELIM, 3);
				magic.target = util.getIntegerToken(data, S_DELIM, 4);
				if (magic.target >= 100)
				{
					magic.target %= 100;
					magic.deadTargetFlag = 1;
				}
				else
					magic.deadTargetFlag = 0;
				util.getStringToken(data, S_DELIM, 5, name);
				util.makeStringFromEscaped(name);
				magic.name = name;
				util.getStringToken(data, S_DELIM, 6, memo);
				util.makeStringFromEscaped(memo);
				magic.memo = memo;
			}
			SetMagic(no, magic);
		}
		break;
	}
	case 'N':
	{
#if 0
		ACTION* ptAct;
		char name[256];
		int no, kubun, i, checkPartyCount, gx, gy, no2;
		unsigned int mask;

		no = data[1] - 48;
		data += 3;
		kubun = util.getInteger62Token(data, S_DELIM, 1);
		if (kubun == 0)
		{
			if (party[no].useFlag != 0 && party[no].id != pc->id)
			{
				ptAct = getCharObjAct(party[no].id);
				if (ptAct != NULL)
					delCharParty(ptAct);
			}
			gx = -1;
			gy = -1;
			if (party[no].ptAct != NULL)
			{
				gx = party[no].ptAct->nextGx;
				gy = party[no].ptAct->nextGy;
			}
			party[no].useFlag = 0;
			party[no].ptAct = NULL;
			checkPartyCount = 0;
			no2 = -1;
#ifdef MAX_AIRPLANENUM
			for (i = 0; i < MAX_AIRPLANENUM; i++)
#else
			for (i = 0; i < MAX_PARTY; i++)
#endif
			{
				if (party[i].useFlag != 0)
				{
					checkPartyCount++;
					if (no2 == -1 && i > no)
						no2 = i;
				}
			}
			if (checkPartyCount <= 1)
			{
				partyModeFlag = 0;
				clearPartyParam();
#ifdef _CHANNEL_MODIFY
				pc->etcFlag &= ~PC_ETCFLAG_CHAT_MODE;
				if (TalkMode == 2)
					TalkMode = 0;
#endif
			}
			else
			{
				if (no2 >= 0 || gx >= 0 || gy >= 0)
					goFrontPartyCharacter(no2, gx, gy);
			}
			break;
		}
		partyModeFlag = 1;
		prSendFlag = 0;
		party[no].useFlag = 1;

		if (kubun == 1)
		{
			party[no].id = util.getIntegerToken(data, S_DELIM, 2);	// 0x00000002
			party[no].level = util.getIntegerToken(data, S_DELIM, 3);	// 0x00000004
			party[no].maxHp = util.getIntegerToken(data, S_DELIM, 4);	// 0x00000008
			party[no].hp = util.getIntegerToken(data, S_DELIM, 5);	// 0x00000010
			party[no].mp = util.getIntegerToken(data, S_DELIM, 6);	// 0x00000020
			util.getStringToken(data, S_DELIM, 7, sizeof(name) - 1, name);	// 0x00000040
			util.makeStringFromEscaped(name);
			if (strlen(name) <= sizeof(party[no].name) - 1)
				party[no].name = toUnicode(name);
			else
				party[no].name = "???";
		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (kubun & mask)
				{
					if (mask == 0x00000002)
					{
						party[no].id = util.getIntegerToken(data, S_DELIM, i);// 0x00000002
						i++;
					}
					else if (mask == 0x00000004)
					{
						party[no].level = util.getIntegerToken(data, S_DELIM, i);// 0x00000004
						i++;
					}
					else if (mask == 0x00000008)
					{
						party[no].maxHp = util.getIntegerToken(data, S_DELIM, i);// 0x00000008
						i++;
					}
					else if (mask == 0x00000010)
					{
						party[no].hp = util.getIntegerToken(data, S_DELIM, i);// 0x00000010
						i++;
					}
					else if (mask == 0x00000020)
					{
						party[no].mp = util.getIntegerToken(data, S_DELIM, i);// 0x00000020
						i++;
					}
					else if (mask == 0x00000040)
					{
						util.getStringToken(data, S_DELIM, i, sizeof(name) - 1, name);// 0x00000040
						util.makeStringFromEscaped(name);
						if (strlen(name) <= sizeof(party[no].name) - 1)
							party[no].name = toUnicode(name);
						else
							party[no].name = "???";
						i++;
					}
				}
			}
		}
		if (party[no].id != pc->id)
		{
			ptAct = getCharObjAct(party[no].id);
			if (ptAct != NULL)
			{
				party[no].ptAct = ptAct;
				setCharParty(ptAct);
				// NPC???????
				if (no == 0)
					setCharLeader(ptAct);
			}
			else
				party[no].ptAct = NULL;
		}
		else
		{
			party[no].ptAct = pc->ptAct;
			setPcParty();
			// PC???????
			if (no == 0)
				setPcLeader();
		}
#endif
	}
	break;
	case 'I':
	{
		int i = 0, no = 0;
		QString temp;

		cdata++;
		QString data = toUnicode(cdata);
		for (i = 0; i < MAX_ITEM; i++)
		{
#ifdef _ITEM_JIGSAW
#ifdef _NPC_ITEMUP
#ifdef _ITEM_COUNTDOWN
			no = i * 16;
#else
			no = i * 15;
#endif
#else
			no = i * 14;
#endif
#else
#ifdef _PET_ITEM
			no = i * 13;
#else
#ifdef _ITEM_PILENUMS
#ifdef _ALCHEMIST //#ifdef _ITEMSET7_TXT
			no = i * 14;
#else

			no = i * 11;

#endif//_ALCHEMIST
#else

			no = i * 10;

			//end modified by lsh

#endif//_ITEM_PILENUMS
#endif//_PET_ITEM
#endif//_ITEM_JIGSAW
			util.getStringToken(data, S_DELIM, no + 1, temp);
			util.makeStringFromEscaped(temp);
			if (temp.isEmpty())
			{
				pc->item[i].useFlag = 0;
				continue;
			}
			pc->item[i].useFlag = 1;
			pc->item[i].name = temp;
			util.getStringToken(data, S_DELIM, no + 2, temp);
			util.makeStringFromEscaped(temp);
			pc->item[i].name2 = temp;
			pc->item[i].color = util.getIntegerToken(data, S_DELIM, no + 3);
			if (pc->item[i].color < 0)
				pc->item[i].color = 0;
			util.getStringToken(data, S_DELIM, no + 4, temp);
			util.makeStringFromEscaped(temp);
			pc->item[i].memo = temp;
			pc->item[i].graNo = util.getIntegerToken(data, S_DELIM, no + 5);
			pc->item[i].field = util.getIntegerToken(data, S_DELIM, no + 6);
			pc->item[i].target = util.getIntegerToken(data, S_DELIM, no + 7);
			if (pc->item[i].target >= 100)
			{
				pc->item[i].target %= 100;
				pc->item[i].deadTargetFlag = 1;
			}
			else
				pc->item[i].deadTargetFlag = 0;
			pc->item[i].level = util.getIntegerToken(data, S_DELIM, no + 8);
			pc->item[i].sendFlag = util.getIntegerToken(data, S_DELIM, no + 9);

			// 顯示物品耐久度
			util.getStringToken(data, S_DELIM, no + 10, temp);
			util.makeStringFromEscaped(temp);
			pc->item[i].damage = temp;

#ifdef _ITEM_PILENUMS
			util.getStringToken(data, S_DELIM, no + 11, temp);
			util.makeStringFromEscaped(temp);
			pc->item[i].pile = util.safe_atoi(temp);
#endif
#ifdef _ALCHEMIST //_ITEMSET7_TXT
			util.getStringToken(data, S_DELIM, no + 12, temp);
			util.makeStringFromEscaped(temp);
			pc->item[i].alch = temp;
#endif
#ifdef _PET_ITEM
			pc->item[i].type = util.getIntegerToken(data, S_DELIM, no + 13);
#else
#ifdef _MAGIC_ITEM_
			pc->item[i].道具類型 = util.getIntegerToken(data, '|', no + 13);
#endif
#endif
#ifdef _ITEM_JIGSAW
			util.getStringToken(data, S_DELIM, no + 14, temp);
			pc->item[i].jigsaw = temp;
#endif
#ifdef _NPC_ITEMUP
			pc->item[i].itemup = util.getIntegerToken(data, S_DELIM, no + 15);
#endif
#ifdef _ITEM_COUNTDOWN
			pc->item[i].counttime = util.getIntegerToken(data, S_DELIM, no + 16);
#endif

			//吃掉全部肉
			pc->item[i].pile = pc->item[i].pile == 0 ? 1 : pc->item[i].pile;
			AutoEatMeat(*pc);

		}
		SetCharData(*pc);
		break;
	}
	//接收到的寵物技能
	case 'W':
	{
		int i = 0, no = 0, no2 = 0;
		QString temp;

		no = cdata[1] - '0';
		QString data = toUnicode(cdata);
		if (no < MAX_PET)
		{
			QVector<PET_SKILL> petskill(MAX_SKILL);
			data += 3;
			for (i = 0; i < MAX_SKILL; i++)
				petskill[i].useFlag = 0;
			for (i = 0; i < MAX_SKILL; i++)
			{
				no2 = i * 5;
				util.getStringToken(data, S_DELIM, no2 + 4, temp);
				util.makeStringFromEscaped(temp);
				if (temp.isEmpty())
					continue;
				petskill[i].useFlag = 1;
				petskill[i].name = temp;
				petskill[i].skillId = util.getIntegerToken(data, S_DELIM, no2 + 1);
				petskill[i].field = util.getIntegerToken(data, S_DELIM, no2 + 2);
				petskill[i].target = util.getIntegerToken(data, S_DELIM, no2 + 3);
				util.getStringToken(data, S_DELIM, no2 + 5, temp);
				util.makeStringFromEscaped(temp);
				petskill[i].memo = temp;
				SetPetSkills(no, petskill);
			}
		}
	}
	break;
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	case 'S':
	{
		char name[CHAR_NAME_LEN + 1];
		char memo[PROFESSION_MEMO_LEN + 1];
		int i, count = 0;

		data++;
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
		{
			profession_skill[i].useFlag = 0;
			profession_skill[i].kind = 0;
		}
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
		{
			count = i * 9;
			profession_skill[i].useFlag = util.getIntegerToken(data, S_DELIM, 1 + count);
			profession_skill[i].skillId = util.getIntegerToken(data, S_DELIM, 2 + count);
			profession_skill[i].target = util.getIntegerToken(data, S_DELIM, 3 + count);
			profession_skill[i].kind = util.getIntegerToken(data, S_DELIM, 4 + count);
			profession_skill[i].icon = util.getIntegerToken(data, S_DELIM, 5 + count);
			profession_skill[i].costmp = util.getIntegerToken(data, S_DELIM, 6 + count);
			profession_skill[i].skill_level = util.getIntegerToken(data, S_DELIM, 7 + count);
			memset(name, 0, sizeof(name));
			util.getStringToken(data, S_DELIM, 8 + count, sizeof(name) - 1, name);
			util.makeStringFromEscaped(name);
			if (strlen(name) <= CHAR_NAME_LEN)
				strcpy(profession_skill[i].name, name);
			memset(memo, 0, sizeof(memo));
			util.getStringToken(data, S_DELIM, 9 + count, sizeof(memo) - 1, memo);
			util.makeStringFromEscaped(memo);
			if (strlen(memo) <= PROFESSION_MEMO_LEN)
				strcpy(profession_skill[i].memo, memo);
		}
#ifdef _SKILLSORT
		SortSkill();
#endif
	}
	break;
#endif
#ifdef _PRO3_ADDSKILL
	case 'G':
	{
		int i, count = 0;
		data++;
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
			profession_skill[i].cooltime = 0;
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
		{
			count = i * 1;
			profession_skill[i].cooltime = util.getIntegerToken(data, S_DELIM, 1 + count);
		}
	}
	break;
#endif
#ifdef _PET_ITEM
	case 'B':
	{
		int i = 0, no = 0, nPetIndex = 0;
		QString szData;

		nPetIndex = cdata[1] - '0';
		cdata += 2;
		QString data = toUnicode(cdata);
		PET pet = GetPet(nPetIndex);
		for (i = 0; i < MAX_PET_ITEM; i++)
		{
#ifdef _ITEM_JIGSAW
#ifdef _NPC_ITEMUP
#ifdef _ITEM_COUNTDOWN
			no = i * 16;
#else
			no = i * 15;
#endif
#else
			no = i * 14;
#endif
#else
			no = i * 13;
#endif
			util.getStringToken(data, S_DELIM, no + 1, szData);
			util.makeStringFromEscaped(szData);
			if (szData.isEmpty())	// 沒道具
			{
				pet.item[i] = {};
				continue;
			}
			pet.item[i].useFlag = 1;
			pet.item[i].name = szData;
			util.getStringToken(data, S_DELIM, no + 2, szData);
			util.makeStringFromEscaped(szData);
			pet.item[i].name2 = szData;
			pet.item[i].color = util.getIntegerToken(data, S_DELIM, no + 3);
			if (pet.item[i].color < 0)
				pet.item[i].color = 0;
			util.getStringToken(data, S_DELIM, no + 4, szData);
			util.makeStringFromEscaped(szData);
			pet.item[i].memo = szData;
			pet.item[i].graNo = util.getIntegerToken(data, S_DELIM, no + 5);
			pet.item[i].field = util.getIntegerToken(data, S_DELIM, no + 6);
			pet.item[i].target = util.getIntegerToken(data, S_DELIM, no + 7);
			if (pet.item[i].target >= 100)
			{
				pet.item[i].target %= 100;
				pet.item[i].deadTargetFlag = 1;
			}
			else
				pet.item[i].deadTargetFlag = 0;
			pet.item[i].level = util.getIntegerToken(data, S_DELIM, no + 8);
			pet.item[i].sendFlag = util.getIntegerToken(data, S_DELIM, no + 9);

			// 顯示物品耐久度
			util.getStringToken(data, S_DELIM, no + 10, szData);
			util.makeStringFromEscaped(szData);
			pet.item[i].damage = szData;
			pet.item[i].pile = util.getIntegerToken(data, S_DELIM, no + 11);
#ifdef _ALCHEMIST //_ITEMSET7_TXT
			util.getStringToken(data, S_DELIM, no + 12, szData);
			util.makeStringFromEscaped(szData);
			pet.item[i].alch = szData;
#endif
			pet.item[i].type = util.getIntegerToken(data, S_DELIM, no + 13);
#ifdef _ITEM_JIGSAW
			util.getStringToken(data, S_DELIM, no + 14, szData);
			util.makeStringFromEscaped(szData);
			pet.item[i].jigsaw = szData;
			//可拿給寵物裝備的道具,就不會是拼圖了,以下就免了
			//if( i == JigsawIdx )
			//	SetJigsaw( pc->item[i].graNo, pc->item[i].jigsaw );
#endif
#ifdef _NPC_ITEMUP
			pet.item[i].itemup = util.getIntegerToken(data, S_DELIM, no + 15);
#endif
#ifdef _ITEM_COUNTDOWN
			pet.item[i].counttime = util.getIntegerToken(data, '|', no + 16);
#endif
		}
		SetPet(nPetIndex, pet);
	}
	break;
#endif
	}
}

/*===========================
* 周圍單位資料
1 OBJTYPE_CHARA
2 OBJTYPE_ITEM
3 OBJTYPE_GOLD
4 NPC&other player
===========================*/
void GameService::lssproto_C_recv(int fd, char* data)
{
	Util util;
	MAP_UNIT unit = { 0 };
	//int i, j;
	//char bigtoken[2048], smalltoken[2048] = {}, name[2048], freeName[2048], info[1024], fmname[2048], petname[1024];

#ifdef _CHARTITLE_STR_
	char titlestr[128];
	int titleindex = 0;
	*titlestr = 0;
#endif
	//int petlevel;
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	int profession_class, profession_level, profession_skill_point;
#endif
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	int herofloor;
#endif
#ifdef _NPC_PICTURE
	int picture;
#endif
#ifdef _NPC_EVENT_NOTICE
	int noticeNo;
#endif
	//if (!IS_ONLINE_FLAG)
	//	return;
	//if (IS_BATTLE_FLAG)
	//	return;

	QString qdata = toUnicode(data);
	QStringList list = qdata.split(",", Qt::SkipEmptyParts);
	for (const QString& szobj : list)
	{
		QStringList objinfos = szobj.split("|");
		switch (objinfos.size())
		{
		case 30://NPC
		{
			//1|13|59D|17|13|4|16054|1|0|薩姆吉爾的肉店||1|1|0|||0<-16 # 17->|0|0|0|0|0|0|0|0|0|0|25923|0|0,
			unit.charType = objinfos.at(1).toInt();
			unit.id = util.a62toi(objinfos.at(2));
			unit.x = objinfos.at(3).toInt();
			unit.y = objinfos.at(4).toInt();
			unit.dir = objinfos.at(5).toInt();
			unit.graNo = objinfos.at(6).toInt();
			unit.level = objinfos.at(7).toInt();
			unit.nameColor = objinfos.at(8).toInt();
			unit.name = objinfos.at(9);
			unit.freeName = objinfos.at(10);
			unit.walkable = objinfos.at(11).toInt();
			unit.height = objinfos.at(12).toInt();
			unit.charNameColor = objinfos.at(13).toInt();
			unit.fmname = objinfos.at(14);
			unit.petname = objinfos.at(15);
			unit.petlevel = objinfos.at(16).toInt();

			if (unit.charNameColor < 0)
				unit.charNameColor = 0;
			m_hash_units.insert(unit.id, unit);
		}
		//HUMAN
		//1|1|8if|17|15|0|100008|14|0|QPointer||1|1|0|||0|0|0|0|0|0|0|0|0|0|0|65|0|0
		//PET
		//1|3|8jo|16|16|5|100296|1|0|凱比||1|1|0|||0|0|0|0|0|0|0|0|0|0|0|3968|0|0
		//ITEM
		//2|8jp|16|16|24008|0|小塊肉
		}


	}
#if 0
	for (i = 0; ; i++)
	{

		util.getStringToken(data, ',', i + 1, sizeof(bigtoken) - 1, bigtoken);
		if (strlen(bigtoken) == 0)
			break;
#ifdef _OBJSEND_C
		util.getStringToken(bigtoken, '|', 1, sizeof(smalltoken) - 1, smalltoken);
		if (strlen(smalltoken) == 0)
			return;
		switch (atoi(smalltoken))
		{
		case 1://OBJTYPE_CHARA
			charType = util.getIntegerToken(bigtoken, '|', 2);
			util.getStringToken(bigtoken, '|', 3, sizeof(smalltoken) - 1, smalltoken);
			id = a62toi(smalltoken);

			extern BOOL 人物屏蔽開關;
			if (人物屏蔽開關) {
				if (id != pc.id) {
					if (charType < 4)
						continue;
				}
			}

			util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
			x = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 5, sizeof(smalltoken) - 1, smalltoken);
			y = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 6, sizeof(smalltoken) - 1, smalltoken);
			dir = (atoi(smalltoken) + 3) % 8;
			util.getStringToken(bigtoken, '|', 7, sizeof(smalltoken) - 1, smalltoken);
			graNo = atoi(smalltoken);
			if (graNo == 9999) continue;
			util.getStringToken(bigtoken, '|', 8, sizeof(smalltoken) - 1, smalltoken);
			level = atoi(smalltoken);
			nameColor = util.getIntegerToken(bigtoken, '|', 9);
			util.getStringToken(bigtoken, '|', 10, sizeof(name) - 1, name);
			util.makeStringFromEscaped(name);
			util.getStringToken(bigtoken, '|', 11, sizeof(freeName) - 1, freeName);
			util.makeStringFromEscaped(freeName);
			util.getStringToken(bigtoken, '|', 12, sizeof(smalltoken) - 1, smalltoken);
			walkable = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 13, sizeof(smalltoken) - 1, smalltoken);
			height = atoi(smalltoken);
			charNameColor = util.getIntegerToken(bigtoken, '|', 14);
			util.getStringToken(bigtoken, '|', 15, sizeof(fmname) - 1, fmname);
			util.makeStringFromEscaped(fmname);
			util.getStringToken(bigtoken, '|', 16, sizeof(petname) - 1, petname);
			util.makeStringFromEscaped(petname);
			util.getStringToken(bigtoken, '|', 17, sizeof(smalltoken) - 1, smalltoken);
			petlevel = atoi(smalltoken);
#ifdef _NPC_EVENT_NOTICE
			util.getStringToken(bigtoken, '|', 18, sizeof(smalltoken) - 1, smalltoken);
			noticeNo = atoi(smalltoken);
#endif
#ifdef _CHARTITLE_STR_
			util.getStringToken(bigtoken, '|', 23, sizeof(titlestr) - 1, titlestr);
			titleindex = atoi(titlestr);
			memset(titlestr, 0, 128);
			if (titleindex > 0) {
				extern char* FreeGetTitleStr(int id);
				sprintf(titlestr, "%s", FreeGetTitleStr(titleindex));
			}
#endif
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
			util.getStringToken(bigtoken, '|', 18, sizeof(smalltoken) - 1, smalltoken);
			profession_class = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 19, sizeof(smalltoken) - 1, smalltoken);
			profession_level = atoi(smalltoken);
			//			util.getStringToken(bigtoken, '|', 20, sizeof(smalltoken) - 1, smalltoken);
			//			profession_exp = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 20, sizeof(smalltoken) - 1, smalltoken);
			profession_skill_point = atoi(smalltoken);
#ifdef _ALLDOMAN // Syu ADD 排行榜NPC
			util.getStringToken(bigtoken, '|', 21, sizeof(smalltoken) - 1, smalltoken);
			herofloor = atoi(smalltoken);
#endif
#ifdef _NPC_PICTURE
			util.getStringToken(bigtoken, '|', 22, sizeof(smalltoken) - 1, smalltoken);
			picture = atoi(smalltoken);
#endif
			//    #ifdef _GM_IDENTIFY		// Rog ADD GM識別
			//			util.getStringToken(bigtoken , '|', 23 , sizeof( gm_name ) - 1, gm_name );
			//			util.makeStringFromEscaped( gm_name );
			//  #endif
#endif
			if (charNameColor < 0)
				charNameColor = 0;
			if (pc.id == id)
			{
				if (pc.ptAct == NULL)
				{
					createPc(graNo, x, y, dir);
					updataPcAct();
				}
				else
					setPcGraNo(graNo, pc.dir);

#ifdef _CHARTITLE_STR_
				getCharTitleSplit(titlestr, &pc.ptAct->TitleText);
#endif
				updateMapArea();
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
				//    #ifdef _GM_IDENTIFY		// Rog ADD GM識別
				//				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_exp, profession_skill_point , gm_name);
				//				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point , gm_name);
				//    #else
				//				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_exp, profession_skill_point);
#ifdef _ALLDOMAN // Syu ADD 排行榜NPC
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point, herofloor);
#else
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point);
#endif
				//    #endif
#else
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height);
#endif
				setPcNameColor(charNameColor);
				if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
				{
					party[0].level = pc.level;
					strcpy(party[0].name, pc.name);
				}
#ifdef MAX_AIRPLANENUM
				for (j = 0; j < MAX_AIRPLANENUM; j++)
#else
				for (j = 0; j < MAX_PARTY; j++)
#endif
				{
					if (party[j].useFlag != 0 && party[j].id == id)
					{
						party[j].ptAct = pc.ptAct;
						setPcParty();
						if (j == 0)
							setPcLeader();
						break;
					}
				}
			}
			else
			{
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, gm_name);
#else
#ifdef _NPC_PICTURE
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, picture);
#else
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class);
#endif
#endif
#else
#ifdef _NPC_EVENT_NOTICE
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, noticeNo
#ifdef _CHARTITLE_STR_
					, titlestr
#endif

				);
#else
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType);
#endif
#endif
				ptAct = getCharObjAct(id);
#ifdef _NPC_EVENT_NOTICE
				//	noticeNo=120137;
				if (charType == 13 && noticeNo > 0) {
					setNpcNotice(ptAct, noticeNo);
				}
#endif
				if (ptAct != NULL)
				{
#ifdef MAX_AIRPLANENUM
					for (j = 0; j < MAX_AIRPLANENUM; j++)
#else
					for (j = 0; j < MAX_PARTY; j++)
#endif
					{
						if (party[j].useFlag != 0 && party[j].id == id)
						{
							party[j].ptAct = ptAct;
							setCharParty(ptAct);
							if (j == 0)
								setCharLeader(ptAct);
							break;
						}
					}
					setCharNameColor(ptAct, charNameColor);
				}
			}
			break;
		case 2://OBJTYPE_ITEM
			util.getStringToken(bigtoken, '|', 2, sizeof(smalltoken) - 1, smalltoken);
			id = a62toi(smalltoken);
			util.getStringToken(bigtoken, '|', 3, sizeof(smalltoken) - 1, smalltoken);
			x = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
			y = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 5, sizeof(smalltoken) - 1, smalltoken);
			graNo = atoi(smalltoken);
			classNo = util.getIntegerToken(bigtoken, '|', 6);
			util.getStringToken(bigtoken, '|', 7, sizeof(info) - 1, info);
			util.makeStringFromEscaped(info);
			setItemCharObj(id, graNo, x, y, 0, classNo, info);
			break;
		case 3://OBJTYPE_GOLD
			util.getStringToken(bigtoken, '|', 2, sizeof(smalltoken) - 1, smalltoken);
			id = a62toi(smalltoken);
			util.getStringToken(bigtoken, '|', 3, sizeof(smalltoken) - 1, smalltoken);
			x = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
			y = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 5, sizeof(smalltoken) - 1, smalltoken);
			money = atoi(smalltoken);
			sprintf_s(info, "%d Stone", money);
			if (money > 10000)
				setMoneyCharObj(id, 24050, x, y, 0, money, info);
			else if (money > 1000)
				setMoneyCharObj(id, 24051, x, y, 0, money, info);
			else
				setMoneyCharObj(id, 24052, x, y, 0, money, info);
			break;
		case 4:
			util.getStringToken(bigtoken, '|', 2, sizeof(smalltoken) - 1, smalltoken);
			id = a62toi(smalltoken);
			util.getStringToken(bigtoken, '|', 3, sizeof(name) - 1, name);
			util.makeStringFromEscaped(name);
			util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
			dir = (atoi(smalltoken) + 3) % 8;
			util.getStringToken(bigtoken, '|', 5, sizeof(smalltoken) - 1, smalltoken);
			graNo = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 6, sizeof(smalltoken) - 1, smalltoken);
			x = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 7, sizeof(smalltoken) - 1, smalltoken);
			y = atoi(smalltoken);

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0, "");
#else
#ifdef _NPC_PICTURE
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0, 0);
#else
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0);
#endif
#endif
#else
#ifdef _NPC_EVENT_NOTICE
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0
#ifdef _CHARTITLE_STR_
				, titlestr
#endif
			);
#else
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2);
#endif
#endif
			ptAct = getCharObjAct(id);
			break;
		}
#else
		util.getStringToken(bigtoken, '|', 11, sizeof(smalltoken) - 1, smalltoken);
		if (strlen(smalltoken) > 0) {
			// NPC?
			unit.charType = util.getIntegerToken(bigtoken, '|', 1);
			util.getStringToken(bigtoken, '|', 2, sizeof(smalltoken) - 1, smalltoken);
			unit.id = a62toi(smalltoken);
			util.getStringToken(bigtoken, '|', 3, sizeof(smalltoken) - 1, smalltoken);
			unit.x = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
			unit.y = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 5, sizeof(smalltoken) - 1, smalltoken);
			unit.dir = (atoi(smalltoken) + 3) % 8;
			util.getStringToken(bigtoken, '|', 6, sizeof(smalltoken) - 1, smalltoken);
			unit.graNo = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 7, sizeof(smalltoken) - 1, smalltoken);
			unit.level = atoi(smalltoken);
			unit.nameColor = util.getIntegerToken(bigtoken, '|', 8);
			util.getStringToken(bigtoken, '|', 9, sizeof(name) - 1, name);
			util.makeStringFromEscaped(name);
			unit.name = toUnicode(name);
			util.getStringToken(bigtoken, '|', 10, sizeof(freeName) - 1, freeName);
			util.makeStringFromEscaped(freeName);
			unit.freeName = toUnicode(freeName);
			util.getStringToken(bigtoken, '|', 11, sizeof(smalltoken) - 1, smalltoken);
			unit.walkable = atoi(smalltoken);
			util.getStringToken(bigtoken, '|', 12, sizeof(smalltoken) - 1, smalltoken);
			unit.height = atoi(smalltoken);
			unit.charNameColor = util.getIntegerToken(bigtoken, '|', 13);
			util.getStringToken(bigtoken, '|', 14, sizeof(fmname) - 1, fmname);
			util.makeStringFromEscaped(fmname);

			util.getStringToken(bigtoken, '|', 15, sizeof(petname) - 1, petname);
			util.makeStringFromEscaped(petname);

			util.getStringToken(bigtoken, '|', 16, sizeof(smalltoken) - 1, smalltoken);
			unit.petlevel = atoi(smalltoken);
			if (unit.charNameColor < 0)
				unit.charNameColor = 0;

		}
		else
		{
			util.getStringToken(bigtoken, '|', 6, sizeof(smalltoken) - 1, smalltoken);
			if (strlen(smalltoken) > 0) {
				util.getStringToken(bigtoken, '|', 1, sizeof(smalltoken) - 1, smalltoken);
				unit.id = a62toi(smalltoken);
				util.getStringToken(bigtoken, '|', 2, sizeof(smalltoken) - 1, smalltoken);
				unit.x = atoi(smalltoken);
				util.getStringToken(bigtoken, '|', 3, sizeof(smalltoken) - 1, smalltoken);
				unit.y = atoi(smalltoken);
				util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
				unit.graNo = atoi(smalltoken);
				unit.classNo = util.getIntegerToken(bigtoken, '|', 5);
				util.getStringToken(bigtoken, '|', 6, sizeof(info) - 1, info);
				util.makeStringFromEscaped(info);
				unit.info = toUnicode(info);
			}
			else
			{
				util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
				if (strlen(smalltoken) > 0) {
					util.getStringToken(bigtoken, '|', 1, sizeof(smalltoken) - 1, smalltoken);
					unit.id = a62toi(smalltoken);
					util.getStringToken(bigtoken, '|', 2, sizeof(smalltoken) - 1, smalltoken);
					unit.x = atoi(smalltoken);
					util.getStringToken(bigtoken, '|', 3, sizeof(smalltoken) - 1, smalltoken);
					unit.y = atoi(smalltoken);
					util.getStringToken(bigtoken, '|', 4, sizeof(smalltoken) - 1, smalltoken);
					unit.money = atoi(smalltoken);
					sprintf_s(info, "%d Stone", unit.money);
					unit.info = toUnicode(info);
				}
			}
		}
#endif
	}
#endif

}
#pragma endregion

#pragma region SEND
//心跳包
void GameService::lssproto_Echo_send(int fd, char* test)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkstring(buffer.get(), MAX_SMALLBUFF, test, 4u);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_ECHO_SEND, buffer.get(), MAX_SMALLBUFF);
}

//重定位
void GameService::lssproto_EO_send(int fd, int dummy)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, dummy);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_EO_SEND, buffer.get(), MAX_SMALLBUFF);
}

//戰鬥動作
void GameService::lssproto_B_send(int fd, std::string command)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	char buf[20] = {};
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", command.c_str());
	iChecksum += autil->util_mkstring(buffer.get(), MAX_SMALLBUFF, buf, sizeof(buf));
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_B_SEND, buffer.get(), MAX_SMALLBUFF);
}

//送信
void GameService::lssproto_MSG_send(int fd, int index, const QString& message, int color)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	char buf[1024] = {};
	std::string smsg = message.toStdString();
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", smsg.c_str());

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, index);
	iChecksum += autil->util_mkstring(buffer.get(), MAX_SMALLBUFF, buf, sizeof(buf));
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, color);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_MSG_SEND, buffer.get(), MAX_SMALLBUFF);
}

//發話
void GameService::lssproto_TK_send(int fd, const QString& message, int color, int area)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;


	char buf[1024] = {};
	wchar_t wbuf[1024] = {};
	std::wstring wsmsg = L"P|" + message.toStdWString();

	// LCMapStringA轉成簡體中文  tc -> sc
	int size = lstrlenW(wsmsg.c_str());
	LCMapStringW(LOCALE_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wsmsg.c_str(), size + 1, wbuf, sizeof(wbuf));

	//utf16 -> utf8 -> gb2312
	QString newstr = QString::fromStdWString(wbuf);
	std::string smsg = fromUnicode(newstr);
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", smsg.c_str());


	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkstring(buffer.get(), MAX_SMALLBUFF, buf, sizeof(buf));
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, color);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, area);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_TK_SEND, buffer.get(), MAX_SMALLBUFF);
}

//計算從起始點到目的地的走路坐標和方向，返回所有步數
int CalcWalkPos(WALKARRAY* walk, int xstart, int ystart, int xend, int yend)
{
	//x1,y1起始坐標，x2,y2目標坐標,xf,yf走路方向
	int i, x, y, x1, y1, x2, y2, xf, yf, dx, dy;
	//用於記錄是否已向離目的地較遠的方向移動一步
	bool flag;
	WALKARRAY walkpos[100] = {};
	if (xstart == xend && ystart == yend)
		return 0;
	x1 = xstart;
	y1 = ystart;
	x2 = xend;
	y2 = yend;
	//記錄當前起始位置
	x = x1;
	y = y1;
	//確定走路方向
	i = 0;
	if (x1 <= x2)
		xf = 1;
	else
		xf = -1;
	if (y1 <= y2)
		yf = 1;
	else
		yf = -1;
	//用於控制走路方式，為假沿x軸或y軸走，為真則沿x,y軸各步一步
	flag = false;
	do {
		//當前位置與目的地的距離x軸和y軸各有多遠
		dx = qAbs(x2 - x);
		dy = qAbs(y2 - y);
		if (dx > 0 && dy == 0) {//只沿x軸方向走路
			if (x != x2)x += xf;
		}
		else if (dy > 0 && dx == 0) {//只沿y軸方向走路
			if (y != y2)y += yf;
		}
		else {//沿xy軸方向同時走路
			//“之”字型走路，若沿x軸走一步，則下一步沿x,y軸各步一步
			if (dx > dy && flag == false) {//沿x軸走一步
				if (x != x2)x += xf;
				flag = true;
			}
			else if (dx < dy && flag == false) {//沿y軸走一步
				if (y != y2)y += yf;
				flag = true;
			}
			else {//沿x,y軸各走一步
				if (x != x2)x += xf;
				if (y != y2)y += yf;
				flag = false;
			}
		}
		//記下向前走一步後的新坐標
		walkpos[i].x = x;
		walkpos[i].y = y;
		//求新坐標與上一個坐標間的距離
		dx = walkpos[i].x - x1;
		dy = walkpos[i].y - y1;
		//根據兩點間的距離來確定走路的方向
		if (dx == -1 && dy == 0) {//g
			strcpy_s(walkpos[i].direction, "g");
		}
		else if (dx == -1 && dy == 1) {//f
			strcpy_s(walkpos[i].direction, "f");
		}
		else if (dx == 0 && dy == 1) {//e
			strcpy_s(walkpos[i].direction, "e");
		}
		else if (dx == 1 && dy == 1) {//d
			strcpy_s(walkpos[i].direction, "d");
		}
		else if (dx == 1 && dy == 0) {//c
			strcpy_s(walkpos[i].direction, "c");
		}
		else if (dx == 1 && dy == -1) {//b
			strcpy_s(walkpos[i].direction, "b");
		}
		else if (dx == 0 && dy == -1) {//a
			strcpy_s(walkpos[i].direction, "a");
		}
		else if (dx == -1 && dy == -1) {//h
			strcpy_s(walkpos[i].direction, "h");
		}
		else
			return 0;
		//把新坐標做為下一次走路的起始位置
		x1 = walkpos[i].x;
		y1 = walkpos[i].y;
		i++;
	} while (x != x2 || y != y2);
	int n, j;
	n = i / 2;
	x1 = xstart;
	y1 = ystart;
	j = 0;
	while (j < n) {
		walk[j].x = x1;
		walk[j].y = y1;
		strcpy_s(walk[j].direction, walkpos[2 * j].direction);
		strncat_s(walk[j].direction, sizeof(walk[j].direction), walkpos[2 * j + 1].direction, sizeof(walkpos[2 * j + 1].direction));
		j++;
		x1 = walkpos[2 * j - 1].x;
		y1 = walkpos[2 * j - 1].y;
	}
	if (i % 2 != 0) {
		if (i == 1) {
			walk[n].x = xstart;
			walk[n].y = ystart;
		}
		else {
			walk[n].x = walkpos[i - 2].x;
			walk[n].y = walkpos[i - 2].y;
		}
		strcpy_s(walk[n].direction, walkpos[i - 1].direction);
		n++;
	}
	return n;
}

//計算人物走路後當前所處的位置
void CalcCharPosition(WALKARRAY* walk)
{
	STATICINS(GameService);
	int n = strlen(walk->direction);
	int* x = g_GameService.g_player_xpos3;
	int* y = g_GameService.g_player_xpos3;
	for (int i = 0; i < n; i++) {
		if (walk->direction[i] == 'a') {
			*y -= 1;
		}
		else if (walk->direction[i] == 'b') {
			*x += 1;
			*y -= 1;
		}
		else if (walk->direction[i] == 'c') {
			*x += 1;
		}
		else if (walk->direction[i] == 'd') {
			*x += 1;
			*y += 1;
		}
		else if (walk->direction[i] == 'e') {
			*y += 1;
		}
		else if (walk->direction[i] == 'f') {
			*x -= 1;
			*y += 1;
		}
		else if (walk->direction[i] == 'g') {
			*x -= 1;
		}
		else if (walk->direction[i] == 'h') {
			*x -= 1;
			*y -= 1;
		}
	}
	//*g_GameService.g_player_xpos = x;
	//*g_GameService.g_player_ypos = y;
	//*g_GameService.g_player_xpos2 = x;
	//*g_GameService.g_player_ypos2 = y;

}

bool GameService::WalkPos(const QPoint& p)
{
	STATICINS(GameService);
	int x1, y1;

	x1 = *g_GameService.g_player_xpos3;
	y1 = *g_GameService.g_player_ypos3;
	if (x1 == 0 || y1 == 0)
		return false;
	if (x1 == p.x() && y1 == p.y())
		return true;
	WALKARRAY walk[100];
	int n = CalcWalkPos(walk, x1, y1, p.x(), p.y());
	if (n <= 0)
		return false;
	for (int i = 0; i < n; i++) {
		g_GameService.lssproto_W_send(*g_GameService.g_net_socket, walk[i].x, walk[i].y, walk[i].direction);
		CalcCharPosition(&walk[i]);
	}
	return true;
}

void GameService::lssproto_W_send(int fd, int x, int y, char* direction)
{
	if (*g_world_state != 9 && *g_game_state != 3 || IS_BATTLE_FLAG) return;
	NET_WriteWalkPos_cgitem(*g_net_socket, x, y, direction);
	//QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	//QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	//int iChecksum = 0;

	//buffer[0] = '\0';
	//iChecksum += autil->util_mkint(buffer.get(), x);
	//iChecksum += autil->util_mkint(buffer.get(), y);
	//iChecksum += autil->util_mkstring(buffer.get(), direction);
	//autil->util_mkint(buffer.get(), iChecksum);
	//autil->util_SendMesg(fd, LSSPROTO_W2_SEND, buffer.get());
}

//使用精靈
void GameService::lssproto_MU_send(int fd, int array, int toindex)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_ypos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, array);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, toindex);//0代表人，1-5代表寵物
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_MU_SEND, buffer.get(), MAX_SMALLBUFF);
}

//使用物品
void GameService::lssproto_ID_send(int fd, int haveitemindex, int toindex)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_ypos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, haveitemindex);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, toindex);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_ID_SEND, buffer.get(), MAX_SMALLBUFF);
}

//0斷線1回點
void GameService::lssproto_CharLogout_send(int fd, int Flg)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, Flg);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_CHARLOGOUT_SEND, buffer.get(), MAX_SMALLBUFF);
}

//菜單
void GameService::lssproto_FS_send(int fd, int flg)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, flg);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_FS_SEND, buffer.get(), MAX_SMALLBUFF);
}

//物品移動
void GameService::lssproto_MI_send(int fd, int fromindex, int toindex)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, fromindex);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, toindex);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_MI_SEND, buffer.get(), MAX_SMALLBUFF);
}

//菜單
void GameService::lssproto_SaMenu_send(int fd, int index)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';

	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, index);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_SAMENU_SEND, buffer.get(), MAX_SMALLBUFF);
}

//賣物(243) 格式[位置|數量] 10\z1 
bool GameService::ITEM_SellItem(const QString& sznpcname, const QString& szitem)
{
	int id = -1;
	QVector<MAP_UNIT> units;
	for (const MAP_UNIT unit : m_hash_units)
	{
		units.append(unit);
	}
	//根據距離排序
	QPoint current(*g_player_xpos, *g_player_ypos);
	std::sort(units.begin(), units.end(), [current](const MAP_UNIT& a, const MAP_UNIT& b) {
		return (QPoint(a.x, a.y) - current).manhattanLength() < (QPoint(b.x, b.y) - current).manhattanLength();
		}
	);

	for (const MAP_UNIT unit : units)
	{
		if (!unit.name.isEmpty() && unit.name.contains(sznpcname))
		{
			id = unit.id;
			break;
		}
	}

	if (-1 == id)
		return false;

	//find items
	QStringList sellitem_list;
	PC pc = GetCharData();
	for (int i = 0; i < MAX_ITEM; ++i)
	{

		if (pc.item[i].name.isEmpty())
			continue;
		if (pc.item[i].name.contains(szitem))
			sellitem_list.push_back(QString("%1\\z%2").arg(i).arg(pc.item[i].pile == 0 ? 1 : pc.item[i].pile));
	}

	if (!sellitem_list.size()) return false;

	for (const QString& qcmd : sellitem_list)
	{
		std::string scmd = fromUnicode(qcmd);
		lssproto_WN_send(*g_net_socket, DLG_SELL, id, BUTTON_NOTUSED, (char*)scmd.c_str());
	}
	return true;
}

//多功能面板
void GameService::lssproto_WN_send(int fd, int seqno, int objindex, int select, const std::string& data)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_ypos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, seqno);//dialg id base + 29C36E18
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, objindex); //npcid base + 29C36E1C
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, select);//button
	iChecksum += autil->util_mkstring(buffer.get(), MAX_SMALLBUFF, data.c_str(), data.length());
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_WN_SEND, buffer.get(), MAX_SMALLBUFF);
}

//丟棄物品
void GameService::lssproto_DI_send(int fd, int itemindex)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_ypos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, itemindex);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_DI_SEND, buffer.get(), MAX_SMALLBUFF);
}

//組隊
void GameService::lssproto_JOINTEAM_send(int fd)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;
	int ecx = 0;
	try
	{
		int* unk = CONVERT_GAMEVAR(int*, 0x187060);
		int edx = (*unk) * 0x10C; //imul edx,[xgsa.exe+187060],0000010C
		int eax = *(int*)(edx + CONVERT_GAMEVAR(int, 0x2F525C));//mov eax,[edx+xgsa.exe+2F525C]
		ecx = *(int*)(eax + 0x24C); //mov ecx,[eax+0000024C]
	}
	catch (...)
	{
		return;
	}

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_ypos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, ecx);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_JOINTEAM_SEND, buffer.get(), MAX_SMALLBUFF);
}

void GameService::lssproto_L_send(int fd, int dir)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, dir);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_L_SEND, buffer.get(), MAX_SMALLBUFF);
}

//0退隊
void GameService::lssproto_PR_send(int fd, int request)
{
	QScopedPointer<Autil> autil(q_check_ptr(new Autil()));
	QScopedArrayPointer <char> buffer(new char[MAX_SMALLBUFF]());
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_xpos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, *g_player_ypos);
	iChecksum += autil->util_mkint(buffer.get(), MAX_SMALLBUFF, request);
	autil->util_mkint(buffer.get(), MAX_SMALLBUFF, iChecksum);
	autil->util_SendMesg(fd, LSSPROTO_PR_SEND, buffer.get(), MAX_SMALLBUFF);
}
#pragma endregion

#pragma region Functional
void GameService::EnableMoveLock(bool enable)
{
	if (enable)
	{
		//移動動畫
		write(CONVERT_GAMEVAR(DWORD, 0xBD8CD), "\x90\x90\x90\x90\x90", 5);
		//移動封包
		write(CONVERT_GAMEVAR(DWORD, 0xF73FE), "\x90\x90\x90\x90\x90", 5);
	}
	else
	{
		if (!IS_AUTO_COMBAT)
		{
			//移動動畫
			write(CONVERT_GAMEVAR(DWORD, 0xBD8CD), "\xE8\x1E\x1C\x00\x00", 5);
			//移動封包
			write(CONVERT_GAMEVAR(DWORD, 0xF73FE), "\xE8\x7D\xBA\xF1\xFF", 5);
		}
	}
}
#pragma endregion
