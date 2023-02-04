#pragma once
#ifndef GAMESEVICE_H
#define GAMESEVICE_H
#include "stdafx.h"
#include "structs.h"
#include "listview.h"

#include "util.hpp"

//QtConcurrent
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QFutureSynchronizer>

#define USE_LOCKER

#define CONVERT_GAMEVAR(type, offset) (type)((ULONG_PTR)g_hGameBase + offset)
#define CONVERT_GAMEDLLVAR(type, offset) (type)((ULONG_PTR)g_hDllBase + offset)
#define CONVERT_G_GAMEVAR(type, offset) (type)((ULONG_PTR)g_GameService.g_hGameBase + offset)
#define CONVERT_G_GAMEDLLVAR(type, offset) (type)((ULONG_PTR)g_GameService.g_hDllBase + offset)

class GameService : public QObject
{
	Q_OBJECT
private:
	DISABLE_COPY_MOVE(GameService)
public://FUN

	virtual ~GameService();
	void initialize();
	void uninitialize();

	int Send(int msg, int wParam, int lParam);

	void AppendLog(const char* str);
	void AppendLog(const QString& str);
	void ClearLog() { m_log_model.clear(); };
	StringListModel* GetLogModelPointer() { return &m_log_model; }

	void AutoEatMeat(const PC& pc);
	void AutoUseMagicInNormal(const PC& pc);
	void AutoUseMagicForPetInNormal(int j, const PET& pet);

	void ClearAllData()
	{
		g_online_timer.restart();
		g_battle_timer.restart();
		g_echo_timer.restart();
		IS_ONLINE_FLAG = false;
		IS_AUTO_COMBAT = false;
		SetBattleFlag(false);
		IS_BATTLE_READY_ACT = false;
		IS_ENABLE_IGNORE_DATA = false;
		ClearBattleData();
		ClearCharData();
		ClearExp();
		ClearMap();
		ClearMapUnit();
		ResetPets();
		ResetPetsSkills();
		ResetParties();
		ResetMagics();
		ClearSocket();
	}

	inline Q_REQUIRED_RESULT int GetSocket() const
	{
#ifdef USE_LOCKER
		QReadLocker locker(&m_SOCKETLOCKER);
#endif
		if (!m_net_socket.isNull())
			return *m_net_socket;
		else
			return 0;
	}
	void SetSocket(int fd)
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_SOCKETLOCKER);
#endif
		if (!m_net_socket.isNull() && *m_net_socket != fd)
			*m_net_socket = fd;
		else
			m_net_socket.reset(q_check_ptr(new int(fd)));
	}
	void ClearSocket()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_SOCKETLOCKER);
#endif
		m_net_socket.reset(nullptr);
	}

	Q_REQUIRED_RESULT qbattle_data_t __vectorcall GetBattleData() const {
#ifdef USE_LOCKER
		QReadLocker locker(&m_BATTLELOCKER);
#endif
		return m_battleWorkData;
	};
	void __vectorcall SetBattleDialogState(BATTLE_DIALOG_TYPE type) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_BATTLELOCKER);
#endif
		m_battleWorkData.panel_type = type;
	}
	void __vectorcall SetBattleData(const qbattle_data_t& data) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_BATTLELOCKER);
#endif
		m_battleWorkData = data;
	}
	Q_REQUIRED_RESULT BATTLE_DIALOG_TYPE __vectorcall GetBattleDialogState() const {
#ifdef USE_LOCKER
		QReadLocker locker(&m_BATTLELOCKER);
#endif
		return m_battleWorkData.panel_type;
	}
	void __vectorcall SetBattleActionStatusByPosIndex(int index, int act) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_BATTLELOCKER);
#endif
		if (index >= 0 && index < m_battleWorkData.obj.size())
		{
			m_battleWorkData.obj[index].act = act;
		}
	}
	void __vectorcall ClearBattleData() {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_BATTLELOCKER);
#endif
		m_battleWorkData = {};
	}

	Q_REQUIRED_RESULT PC __vectorcall GetCharData() const {
#ifdef USE_LOCKER
		QReadLocker locker(&m_CHARLOCKER);
#endif
		return m_pc;
	}
	void __vectorcall SetCharData(const PC& data) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_CHARLOCKER);
#endif
		m_pc = data;
	}
	void __vectorcall ClearCharData() {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_CHARLOCKER);
#endif
		m_pc = {};
	}

	void SetMapFloor(int floor) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAPLOCKER);
#endif
		m_map.floor = floor;
	}
	Q_REQUIRED_RESULT int GetMapFloor() const {
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAPLOCKER);
#endif
		return m_map.floor;
	}
	void SetMapName(const QString& name) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAPLOCKER);
#endif
		m_map.name = name;
	}
	Q_REQUIRED_RESULT QString GetMapName() const {
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAPLOCKER);
#endif
		return m_map.name;
	}
	void SetMap(const Map& map) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAPLOCKER);
#endif
		m_map = map;
	}
	Q_REQUIRED_RESULT Map GetMap() const {
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAPLOCKER);
#endif
		return m_map;
	}
	void ClearMap()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAPLOCKER);
#endif
		m_map = {};
	}

	Q_REQUIRED_RESULT QVector<MAGIC> GetMagics() {
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAGICLOCKER);
#endif
		return m_magic;
	}
	Q_REQUIRED_RESULT MAGIC GetMagic(int i) {
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAGICLOCKER);
#endif
		if (i >= 0 && i < m_magic.size())
			return m_magic.at(i);
		else
			return MAGIC{};

	}
	void SetMagics(const QVector<MAGIC>& magic) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAGICLOCKER);
#endif
		if (magic.size() == m_magic.size() && magic.size() == MAX_MAGIC)
			m_magic = magic;
	}
	void SetMagic(int i, const MAGIC& magic) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAGICLOCKER);
#endif
		if (i >= 0 && i < m_magic.size())
			m_magic[i] = magic;
	}
	void ResetMagics()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAGICLOCKER);
#endif
		m_magic.clear();
		m_magic.resize(MAX_MAGIC);
	}

	Q_REQUIRED_RESULT QVector<PARTY> GetParties() {
#ifdef USE_LOCKER
		QReadLocker locker(&m_PARTYLOCKER);
#endif
		return m_party;
	}
	Q_REQUIRED_RESULT PARTY GetParty(int i) {
#ifdef USE_LOCKER
		QReadLocker locker(&m_PARTYLOCKER);
#endif
		if (i >= 0 && i < m_party.size())
			return m_party.at(i);
		else
			return PARTY{};
	}
	void SetParties(const QVector<PARTY>& party) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PARTYLOCKER);
#endif
		m_party = party;
	}
	void SetParty(int i, const PARTY& party) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PARTYLOCKER);
#endif
		if (i >= 0 && i < m_party.size())
			m_party[i] = party;
	}
	void ResetParties()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PARTYLOCKER);
#endif
		m_party.clear();
		m_party.resize(MAX_PARTY);
	}

	Q_REQUIRED_RESULT QVector<PET> GetPets() {
#ifdef USE_LOCKER
		QReadLocker locker(&m_PETLOCKER);
#endif
		return m_pet;
	}
	Q_REQUIRED_RESULT PET GetPet(int i) {
#ifdef USE_LOCKER
		QReadLocker locker(&m_PETLOCKER);
#endif
		if (i >= 0 && i < m_pet.size())
			return m_pet.at(i);
		else
			return PET{};
	}
	void SetPets(const QVector<PET>& pet) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PETLOCKER);
#endif
		if (pet.size() == m_pet.size() && pet.size() == MAX_PET)
			m_pet = pet;
	}
	void SetPet(int i, const PET& pet) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PETLOCKER);
#endif
		if (i >= 0 && i < m_pet.size())
			m_pet[i] = pet;
	}
	void ResetPets()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PETLOCKER);
#endif
		m_pet.clear();
		m_pet.resize(MAX_PET);
	}

	Q_REQUIRED_RESULT EXP GetExp() {
#ifdef USE_LOCKER
		QReadLocker locker(&m_EXPLOCKER);
#endif
		return m_exp;
	}
	void SetExp(const EXP& exp) {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_EXPLOCKER);
#endif
		m_exp = exp;
	}
	void ClearExp() {
#ifdef USE_LOCKER
		QWriteLocker locker(&m_EXPLOCKER);
#endif
		m_exp = {};
	}

	Q_REQUIRED_RESULT QVector<QVector<PET_SKILL>> GetPetsSkills()
	{
#ifdef USE_LOCKER
		QReadLocker locker(&m_PETSKILLLOCKER);
#endif
		return m_petSkill;
	}
	void SetPetsSkills(const QVector<QVector<PET_SKILL>>& skills)
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PETSKILLLOCKER);
#endif
		if (skills.size() == m_petSkill.size() && skills.size() == 7)
			m_petSkill = skills;
	}
	void SetPetSkills(int i, const QVector<PET_SKILL>& skills)
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PETSKILLLOCKER);
#endif
		if (i >= 0 && i < m_petSkill.size() && m_petSkill.size() == MAX_SKILL)
			m_petSkill[i] = skills;
	}
	void ResetPetsSkills()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_PETSKILLLOCKER);
#endif
		m_petSkill.clear();
		m_petSkill.resize(MAX_PET);
		QVector<PET_SKILL> pet(MAX_SKILL);
		for (int i = 0; i < MAX_PET; i++)
		{
			m_petSkill[i] = pet;
		}
	}

	Q_REQUIRED_RESULT QHash<int, MAP_UNIT> GetMapUnits()
	{
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAPUNITLOCKER);
#endif
		return m_hash_units;
	}
	Q_REQUIRED_RESULT MAP_UNIT GetMapUnit(int key)
	{
#ifdef USE_LOCKER
		QReadLocker locker(&m_MAPUNITLOCKER);
#endif
		return m_hash_units.value(key, MAP_UNIT{});
	}
	void InsertMapUnit(int key, const MAP_UNIT& unit)
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAPUNITLOCKER);
#endif
		m_hash_units.insert(key, unit);
	}
	void ClearMapUnit()
	{
#ifdef USE_LOCKER
		QWriteLocker locker(&m_MAPUNITLOCKER);
#endif
		m_hash_units.clear();
	}


	void SetBattleFlag(bool flag) {
		IS_BATTLE_FLAG = flag; EnableMoveLock(flag);
		if (m_battleWorkData.tickcount_cache > 0 && !flag)
		{
			m_battleWorkData.total_duration += m_battleWorkData.tickcount_cache;
			m_battleWorkData.tickcount_cache = 0;
		}
	}
	void EnableMoveLock(bool enable);


	SOCKET New_socket(int af, int type, int protocol);
	int New_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD  dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
		LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
	int New_recv(SOCKET s, char* buf, int len, int flags);

	int New_closesocket(SOCKET s);

	bool ITEM_SellItem(const QString& sznpcname, const QString& szitem);

	void SetUserAccountData(QString list, int n);

	//send
	void __vectorcall lssproto_Echo_send(int fd, char* test);
	void __vectorcall lssproto_EO_send(int fd, int dummy);
	void __vectorcall lssproto_B_send(int fd, std::string command);
	void __vectorcall lssproto_MSG_send(int fd, int index, const QString& message, int color = FONT_PAL_YELLOW);
	void __vectorcall lssproto_TK_send(int fd, const QString& message, int color = FONT_PAL_WHITE, int area = 5);
	void __vectorcall lssproto_W_send(int fd, int x, int y, char* direction);
	void __vectorcall lssproto_MU_send(int fd, int array, int toindex);
	void __vectorcall lssproto_ID_send(int fd, int haveitemindex, int toindex);
	void __vectorcall lssproto_CharLogout_send(int fd, int Flg);
	void __vectorcall lssproto_FS_send(int fd, int flg);
	void __vectorcall lssproto_MI_send(int fd, int fromindex, int toindex);
	void  __vectorcall lssproto_SaMenu_send(int fd, int index);
	void __vectorcall lssproto_WN_send(int fd, int seqno, int objindex, int select, const std::string& data);
	void __vectorcall lssproto_DI_send(int fd, int itemindex);
	void __vectorcall lssproto_JOINTEAM_send(int fd);
	void __vectorcall lssproto_L_send(int fd, int dir);
	void __vectorcall lssproto_PR_send(int fd, int request);
	void __vectorcall lssproto_ARRAGEITEM_send(int fd);

	bool __vectorcall WalkPos(const QPoint& p);
	void __vectorcall WM_MoveTo(const QPoint& p);
	void __vectorcall WM_TurnTo(int dir);
	void __vectorcall WM_JoinLeave(int request);
	void __vectorcall WM_EO();
	void __vectorcall WM_SetSpeed(int sp);
private://FUN
	GameService();
public:
	void NetDispatchMessage(int fd, char* encoded, ULONG& buflen);
private:
	void __vectorcall Parse_BC_StatusString(QString& data);
	inline int __vectorcall _BATTLE_GetPositionIndexRange(int charposition, int* a_min, int* a_max, int* e_min, int* e_max);
	void __vectorcall OnFastBattleWork(int fd, DWORD flag);

	void __vectorcall _BATTLE_CharDoWork(int fd, const qbattle_data_t& bt, int i, int enemy_pos);
	void __vectorcall _BATTLE_PetDoWork(int fd, int i, int enemy_pos);

	int CalcMaxLoad();

	//recv
	void __vectorcall lssproto_XYD_recv(int fd, int x, int y, int dir);
	void __vectorcall lssproto_EV_recv(int fd, int seqno, int result);
	void __vectorcall lssproto_EN_recv(int fd, int result, int field);
	void __vectorcall lssproto_RS_recv(int fd, char* command);
	void __vectorcall lssproto_B_recv(int fd, char* command);
	void __vectorcall lssproto_S_recv(int fd, char* command);
	void __vectorcall lssproto_NC_recv(int fd, int flg);
	void __vectorcall lssproto_C_recv(int fd, char* data);
	void __vectorcall lssproto_CA_recv(int fd, char* data);

public://FLAG VER
	bool IS_FAST_BATTLE = true;
	bool IS_ENABLE_IGNORE_DATA = false;
	bool IS_ONLINE_FLAG = false;
	bool IS_BATTLE_FLAG = false;
	bool IS_BATTLE_READY_ACT = false;
	bool IS_REWARD_SHOW = false;

	bool IS_DEBUG_MODE = false;
	bool IS_AUTO_EATMEAT = false;//自動吃光肉
	bool IS_AUTO_LOGIN = false;//自動登入
	bool IS_AUTO_BATTLE_MAGIC_HEAL = false;//戰鬥精靈補血
	bool IS_AUTO_COMBAT = false;//原地遇敵
	bool IS_AUTO_JOIN = false;//自動組隊
	bool IS_AUTO_RECOMBAT = false;//登入後自動遇敵

	int BATTLE_AUTO_MAGIC_HEAL_VALUE = 35;

	QString g_autojoin_leader = "\0";
	int g_speed = 14;

public://VER
	char* g_net_personalKey = nullptr;

	int* g_player_xpos = nullptr;
	int* g_player_ypos = nullptr;

	int* g_player_xpos2 = nullptr;
	int* g_player_ypos2 = nullptr;

	int* g_player_xpos3 = nullptr;
	int* g_player_ypos3 = nullptr;

	int* g_game_state = nullptr;
	int* g_world_state = nullptr;

	WORD* g_switcher_flag = nullptr;

	HMODULE g_hGameBase = NULL;

	QString g_cpuid = "\0";
	int g_battle_nopet_cache = 0;

	QElapsedTimer g_battle_timer;
	QElapsedTimer g_online_timer;
	QElapsedTimer g_echo_timer;

	bool g_enable_show_echo_ping = false;

	int g_server_selected = 0;
	QString g_account = "\0";
	QString g_password = "\0";

	QString g_caption = "";

private://VER
	bool m_initialized = false;

	QSharedPointer<int> m_net_socket = nullptr;

	int m_BattleTurnReceiveFlag = false;
	int m_BattleAnimeFlag = 0;
	int m_BattleServerTurnNo = 0;
	int m_BattleClientTurnNo = 0;
	int m_BattleTotalCount = 0;

	//time

	LSTIME m_SaTime = {};
	long m_serverTime = 0;
	long m_FirstTime = 0;
	int m_SaTimeZoneNo = 0;

	StringListModel m_log_model;

	QFutureSynchronizer<void> m_LogSynchronizer;

	//lockers
#ifdef USE_LOCKER
	mutable QReadWriteLock m_SOCKETLOCKER;
	mutable QReadWriteLock m_BATTLELOCKER;
	mutable QReadWriteLock m_CHARLOCKER;
	mutable QReadWriteLock m_PETLOCKER;
	mutable QReadWriteLock m_PETSKILLLOCKER;
	mutable QReadWriteLock m_MAGICLOCKER;
	mutable QReadWriteLock m_MAPLOCKER;
	mutable QReadWriteLock m_PARTYLOCKER;
	mutable QReadWriteLock m_EXPLOCKER;
	mutable QReadWriteLock m_MAPUNITLOCKER;
#endif

	QFuture<void> m_battleWorkFuture;

	//QVector<BC_CHAR_STATE_STRING> bc_char = { 0 };		//戰鬥中人物隊伍信息
	//QVector<BC_ENEMY_STATE_STRING> bc_enemy = { 0 };		//戰鬥中敵人隊伍信息
	qbattle_data_t m_battleWorkData = {};
	PC m_pc = {};

	//magic
	QVector<MAGIC> m_magic = {};
	QVector<PARTY> m_party = {};
	//pet
	QVector<PET> m_pet = {};
	QVector<QVector<PET_SKILL>> m_petSkill = {};

	Map m_map = {};

	EXP m_exp = {};

	QHash<int, MAP_UNIT> m_hash_units = {};

	void(__cdecl* NET_WriteWalkPos_cgitem)(int, int, int, const char*) = NULL;

	SOCKET(WSAAPI* psocket)(int af, int type, int protocol) = socket;

	int(WSAAPI* pWSARecv)(SOCKET s, LPWSABUF lpBuffers, DWORD  dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) = WSARecv;

	int (WSAAPI* precv)(SOCKET s, char* buf, int len, int flags) = NULL;//recv;

	int(WSAAPI* pclosesocket)(SOCKET s) = closesocket;

signals:
	void sigNotifyUpdateData();
};


#endif