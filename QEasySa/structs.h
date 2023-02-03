#pragma once
#ifndef STRUCTS_H
#define STRUCTS_H
#include <QtCore/qglobal.h>
#include <QObject>

constexpr auto MAX_PET = 5;
constexpr auto MAX_MAGIC = 9;
constexpr auto MAX_PARTY = 5;
constexpr auto MAX_ITEM = 25;
constexpr auto MAX_ENEMY = 20;
constexpr auto MAX_SKILL = 7;
constexpr auto MAX_PET_ITEM = 7;
constexpr int BATTLE_MAX_UNIT_PER_ROW = 5;
constexpr int BATTLE_MAX_ROW = 4;
constexpr auto S_DELIM = '|';

constexpr auto CHAR_NAME_LEN = 16;
constexpr auto CHAR_FREENAME_LEN = 32;
constexpr auto MAGIC_NAME_LEN = 28;
constexpr auto MAGIC_MEMO_LEN = 72;
constexpr auto ITEM_NAME_LEN = 28;
constexpr auto ITEM_NAME2_LEN = 16;
constexpr auto ITEM_MEMO_LEN = 84;
constexpr auto PET_NAME_LEN = 16;
constexpr auto PET_FREENAME_LEN = 32;
constexpr auto CHAR_FMNAME_LEN = 33;      // 家族名稱
constexpr auto SKILL_NAME_LEN = 24;
constexpr auto SKILL_MEMO_LEN = 72;

constexpr auto NIGHT_TO_MORNING = 700;
constexpr auto MORNING_TO_NOON = 930;
constexpr auto NOON_TO_EVENING = 200;
constexpr auto EVENING_TO_NIGHT = 300;
constexpr auto LSTIME_SECONDS_PER_DAY = 5400;
constexpr auto LSTIME_HOURS_PER_DAY = 1024;
constexpr auto LSTIME_DAYS_PER_YEAR = 100;

#define _PETS_SELECTCON
#define _SHOW_FUSION

#define _ITEM_PILENUMS
#define _ALCHEMIST
#define _PET_ITEM
#define _ITEM_JIGSAW
#define _NPC_ITEMUP

constexpr int full_target_order[MAX_ENEMY] = { 9, 7, 5, 6, 8, 4, 2, 0, 1, 3, 19, 17, 15, 16, 18, 14, 12, 10, 11, 13 };

constexpr DWORD posset[MAX_ENEMY] = {
	0x1ul,    0x2ul,     0x4ul,     0x8ul,     0x10ul,   //100  8
	0x20ul,   0x40ul,    0x80ul,    0x100ul,   0x200ul,  //40   2
	0x400ul,  0x800ul,   0x1000ul,  0x2000ul,  0x4000ul, //20   1
	0x8000ul, 0x10000ul, 0x20000ul, 0x40000ul, 0x80000ul //80   4 //200 10
};

enum
{
	NO,
	YES,
	DOUBLE_YES,
};

enum
{
	INVALID = -1,
	SELF,
	SINGLE,
	ALL = 20,
};

enum BUTTON_TYPE
{
	BUTTON_NOTUSED = 0,
	BUTTON_OK = 1,	   //確定
	BUTTON_CANCEL = 2, //取消
	BUTTON_YES = 4,	   //是
	BUTTON_NO = 8,	   //否
	BUTTON_CLOSE = 9,  //關閉
	BUTTON_PREVIOUS = 16,  //上一頁
	BUTTON_NEXT = 32,
};

enum LSTIME_SECTION
{
	LS_NOON,
	LS_EVENING,
	LS_NIGHT,
	LS_MORNING,
};

enum BATTLE_DIALOG_TYPE
{
	BATTLE_DIALOG_CHAR,
	BATTLE_DIALOG_CHAR_NO_PET_1,
	BATTLE_DIALOG_CHAR_NO_PET_2,
	BATTLE_DIALOG_PET,
};

enum
{
	CHR_STATUS_P = 0x0001,			// 
	CHR_STATUS_N = 0x0002,			// ??
	CHR_STATUS_Q = 0x0004,			// ?
	CHR_STATUS_S = 0x0008,			// ?
	CHR_STATUS_D = 0x0010,			// ??
	CHR_STATUS_C = 0x0020,			// ??
	CHR_STATUS_W = 0x0040,			// ??????
	CHR_STATUS_H = 0x0080,			// ??????
	CHR_STATUS_LEADER = 0x0100,			// ????
	CHR_STATUS_PARTY = 0x0200,			// ???????
	CHR_STATUS_BATTLE = 0x0400,			// ?
	CHR_STATUS_USE_MAGIC = 0x0800,			// ?
	CHR_STATUS_HELP = 0x1000,			// ???
	CHR_STATUS_FUKIDASHI = 0x2000,			// ???
	CHR_STATUS_WATCH = 0x4000,			// ??
	CHR_STATUS_TRADE = 0x8000,			// 交易中
#ifdef _ANGEL_SUMMON
	CHR_STATUS_ANGEL = 0x10000			// 使者任務中
#endif
};

enum {
	FONT_PAL_WHITE,
	FONT_PAL_AQUA,
	FONT_PAL_PURPLE,
	FONT_PAL_BLUE,
	FONT_PAL_YELLOW,
	FONT_PAL_GREEN,
	FONT_PAL_RED,
	FONT_PAL_GRAY,
	FONT_PAL_BLUE2,
	FONT_PAL_GREEN2,
	FONT_PAL_10,
	FONT_PAL_11,
	FONT_PAL_12,
	FONT_PAL_13,
	FONT_PAL_14,
	FONT_PAL_15,
	FONT_PAL_16,
	FONT_PAL_17,
	FONT_PAL_18,
	FONT_PAL_19,
	FONT_PAL_20,
	FONT_PAL_21,
	FONT_PAL_22,
	FONT_PAL_23,
	FONT_PAL_24,
	FONT_PAL_25,
	FONT_PAL_NUM
};

enum CustomMessages
{
	WM_SA_WALKPOS = WM_USER + 1,
};

#pragma pack(4)

typedef struct tagEXP
{
	int total = 0;
	int current = 0;
	int left = 0;
	int average = 0;
	int expect_to_next = 0;
}EXP;

typedef struct tagMap
{
	int floor = 0;
	QString name = "\0";
}Map;

typedef struct tagEscapeChar
{
	char escapechar = '\0';
	char escapedchar = '\0';
} EscapeChar;

typedef struct qbattle_object_s
{
	bool exist = false;
	int pos = -1;
	QString name = "\0";
	QString freeName = "\0";
	int model = 0;
	int level = 0;
	int hp = 0;
	int maxhp = 0;
	int hp_percent = 0;
	DWORD status = 0;
	int unknown0 = 0;
	int unknown1 = 0;
	int unknown2 = 0;
	int isride;
	QString ridepetname;
	int ridepetlevel;
	int ridepethp;
	int ridepetmaxhp;
	int act = 0; //是否出手;
} qbattle_object_t;

typedef struct qbattle_data_s
{
	int round = 0;
	float tickcount = 0.0f;
	float tickcount_cache = 0.0f;
	int total_battle_count = 0;
	float total_duration = 0.0f;
	int size = 0;
	BATTLE_DIALOG_TYPE panel_type = BATTLE_DIALOG_CHAR;
	int side = -1;
	int char_position = 0;
	int char_hp = 0;
	int char_maxhp = 0;
	int char_mp = 0;
	int char_maxmp = 0;
	int char_hp_percent = -1;
	int char_mp_percent = -1;

	int pet_position = 0;
	int pet_hp = 0;
	int pet_maxhp = 0;
	int pet_mp = 0;
	int pet_maxmp = 0;
	int pet_hp_percent = -1;
	int pet_mp_percent = -1;

	int lowest_enemy_level = -1;
	int lowest_enemy_level_index = -1;

	int sneakstate = 0; //是否偷襲
	int enemy_min = 10;
	int enemy_max = 19;
	int allie_min = 0;
	int allie_max = 9;
	int enemy_count = 0;
	int allie_count = 0;
	int total_count = 0;
	int enemy_front_count = 0;
	int enemy_back_count = 0;
	int allie_front_count = 0;
	int allie_back_count = 0;

	QString reserved = "\0";
	QVector<qbattle_object_t> obj = {};
	QVector<qbattle_object_t> allie = {};
	QVector<qbattle_object_t> allie_front = {};
	QVector<qbattle_object_t> allie_back = {};
	QVector<qbattle_object_t> enemy = {};
	QVector<qbattle_object_t> enemy_front = {};
	QVector<qbattle_object_t> enemy_back = {};
} qbattle_data_t;

//BC|戰場屬性（0:無屬性,1:地,2:水,3:火,4:風）|人物在組隊中的位置|人物名稱|人物稱號|人物形象編號|人物等級(16進制)|當前HP|最大HP|人物狀態（死亡，中毒等）|是否騎乘標志(0:未騎，1騎,-1落馬)|騎寵名稱|騎寵等級|騎寵HP|騎寵最大HP|戰寵在隊伍中的位置|戰寵名稱|未知|戰寵形象|戰寵等級|戰寵HP|戰寵最大HP|戰寵異常狀態（昏睡，死亡，中毒等）|0||0|0|0|
//敵人第1排從上到下依次為(13，11，F，10，12)，敵人第2排從上到下為(E,C,A,B,D)
//我方第1排從上到下依次為（9，7，5，6，8），我方第2排從上到下為(4,2,0,1,3)
typedef struct tagBC_CHAR_STATE_STRING {
	int pos = 0;
	QString name = "\0";
	QString freeName = "\0";
	int faceimg = 0;
	int level = 0;
	int hp = 0;
	int maxhp = 0;
	int state = 0;
	int isride = 0;
	QString ridepetname = "\0";
	int ridepetlevel = 0;
	int ridepethp = 0;
	int ridepetmaxhp = 0;
	//戰寵信息
	int petpos = 0;
	QString petfreeName = "\0";
	QString  unknown = "\0";
	int petimage = 0;
	int petlevel = 0;
	int pethp = 0;
	int petmaxhp = 0;
	int petstate = 0;
	int unknown2 = 0;
	QString unknown3 = "\0";
	int unknown4 = 0;
	int unknown5 = 0;
	int unknown6 = 0;
}BC_CHAR_STATE_STRING;
//敵1位置|敵1名稱|未知|敵1形象|敵1等級|敵1HP|敵1最大HP|敵人異常狀態（死亡，中毒等）|0||0|0|0|
typedef struct tagBC_ENEMY_STATE_STRING {
	int pos = 0;
	QString name = "\0";
	QString unknown = "\0";
	int image = 0;
	int level = 0;
	int hp = 0;
	int maxhp = 0;
	int enemystate = 0;
	int unknown2 = 0;
	QString unknown3 = "\0";
	int unknown4 = 0;
	int unknown5 = 0;
	int unknown6 = 0;
}BC_ENEMY_STATE_STRING;

struct action {
	struct 	action* pPrev = nullptr, * pNext = nullptr;			//上一个及下一个action指标
	void 	(*func)(struct action*);	//action所执行的function的指标
	void* pYobi = nullptr;							//备用的struct指标
	void* pOther = nullptr;						//其它用途struct指标
	UCHAR 	prio = 0i8;							//action处理时的优先顺序
	UCHAR 	dispPrio = 0i8;						//秀图时的优先顺序
	int 	x = 0, y = 0;							//图的座标
	int		hitDispNo = 0;						//是否命中目标编号
	BOOL	deathFlag = FALSE;						//此action是否死亡旗标
	int 	dx = 0, dy = 0;							//秀图座标位移量
	int 	dir = 0;							//方向
	int 	delta = 0;  						//合成向量

	char 	name[29] = {};						//名字
	char 	freeName[33] = {};					//free name
	int 	hp = 0;
#ifdef _PET_ITEM
	int		iOldHp = 0;
#endif
	int 	maxHp = 0;
	int 	mp = 0;
	int 	maxMp = 0;
	int 	level = 0;
	int 	status = 0;
	int 	itemNameColor = 0;					/* ?????�彙q?�� */
	int		charNameColor = 0;					// ??????����??????�彙q?��

	int		bmpNo = 0;							//图号
	int		bmpNo_bak = 0;							//备份图号
	int		atr = 0;							//属性
	int		state = 0;							//状态
	int		actNo = 0;							//行动编号
	int		damage = 0;

	int		gx = 0, gy = 0;							//在目前的地图上的座标
	int		nextGx = 0, nextGy = 0;					//下一个座标
	int		bufGx[10] = { 0 }, bufGy[10] = { 0 };			//从目前座标到下一个座标之间座标的buffer
	short	bufCount = 0i16;						//设定目前要走到那一个座标
	short	walkFlag = 0i16;						// ��????????????��?�v?????????
	float	mx = 0.0f, my = 0.0f;							//地图座标
	float	vx = 0.0f, vy = 0.0f;							// ?�h?��

	//属性
	short 	earth = 0i16;							// 佋 �N��
	short 	water = 0i16;							// ? �N��
	short 	fire = 0i16;							// ? �N��
	short 	wind = 0i16;							// ? �N��
	//rader使用
	int		dirCnt = 0;							// ��?��??????
	//gemini使用
	int		spd = 0;							//移动的速度(0~63)( ?????? )
	int		crs = 0;							//方向(0~31)(正上方为0,顺时钟方向) ��?( ???? )( ��?????? )
	int		h_mini = 0;							// ��?��?
	int		v_mini = 0;							// ��?�滭�
	//pattern使用
	int		anim_chr_no = 0;					//人物的编号(anim_tbl.h的编号)
	int		anim_chr_no_bak = 0;				//上一次的人物编号
	int		anim_no = 0;						//人物的动作编号
	int		anim_no_bak = 0;					//上一次的人物编号
	int		anim_ang = 0;						//动作的方向(0~7)(下0)
	int		anim_ang_bak = 0;					//上一次的方向
	int		anim_cnt = 0;						//第几张frame
	int		anim_frame_cnt = 0;					//这张frame停留时间
	int		anim_x = 0;							//X座标(Sprbin+Adrnbin)
	int		anim_y = 0;							//Y座标(Sprbin+Adrnbin)
	int		anim_hit = 0;						// ???�R�e
	// shan add +1
	char    fmname[33] = {};			            // 家族名称
	// Robin 0728 ride Pet
	int		onRide = 0;
	char	petName[16 + 1] = {};
	int		petLevel = 0;
	int		petHp = 0;
	int		petMaxHp = 0;
	int		petDamage = 0;
	int		petFall = 0;
#ifdef _MIND_ICON
	unsigned int sMindIcon;
#endif
#ifdef _SHOWFAMILYBADGE_
	unsigned int sFamilyIcon;
#endif
#ifdef FAMILY_MANOR_
	unsigned int mFamilyIcon;
#endif
#ifdef _CHAR_MANOR_
	unsigned int mManorIcon;
#endif
#ifdef _CHARTITLE_STR_
	TITLE_STR TitleText;
#endif
#ifdef _CHARTITLE_
	unsigned int TitleIcon;
#endif
#ifdef _NPC_EVENT_NOTICE
	int noticeNo;
#endif

#ifdef _SKILL_ROAR
	int		petRoar;		//大吼(克年兽)
#endif
#ifdef _SKILL_SELFEXPLODE //自爆
	int		petSelfExplode;
#endif
#ifdef _MAGIC_DEEPPOISION   //剧毒
	int		petDeepPoision;
#endif

#ifdef _CHAR_PROFESSION			// WON ADD 人物职业
	int		profession_class;
#endif
	//#ifdef _BATTLESKILL				// (不可开) Syu ADD 战斗技能介面
	int		warrioreffect;
	//#endif
#ifdef _GM_IDENTIFY		// Rog ADD GM识别
	char gm_name[33];
#endif
#ifdef _STREET_VENDOR
	char szStreetVendorTitle[64];
#endif
#ifdef _NPC_PICTURE
	int picture;
	int picturetemp;
#endif
#ifdef _PETSKILL_RIDE
	int saveride;
#endif
#ifdef _MOUSE_DBL_CLICK
	int index;	// 禁断!! Server中的charaindex
#endif

#ifdef _SFUMATO
	int sfumato;		// 二次渲染图层色彩
#endif
};

typedef struct action ACTION;

typedef struct tagITEM
{
	int color = 0;						// �彙q?��
	int graNo = 0;						// ??�k?
	int level = 0;						// ???????
#ifdef _ITEM_PILENUMS
	int pile = 0;
#endif
#ifdef _ALCHEMIST //#ifdef _ITEMSET7_TXT
	QString alch = "\0";
#endif
	short useFlag = 0i16;					// ��????
	short field = 0i16;					// ��????��?
	short target = 0i16;					// �n��
	short deadTargetFlag = 0i16;			// ???????�n��???
	short sendFlag = 0i16;					// ????????��?�u?
	QString name = "\0";		// ????��
	QString name2 = "\0";	// ????��?
	QString memo = "\0";		// ??
	QString damage = "\0";
#ifdef _PET_ITEM
	char type = '\0';
#endif
#ifdef _ITEM_JIGSAW
	QString jigsaw = {};
#endif
#ifdef _NPC_ITEMUP
	int itemup = 0;
#endif
#ifdef _ITEM_COUNTDOWN
	int counttime;
#endif
#ifdef _MAGIC_ITEM_
	int 道具類型;
#endif
} ITEM;


typedef struct tagPC
{
	int graNo = 0;
	int faceGraNo = 0;
	int id = 0;
	int dir = 0;
	int hp = 0, maxhp = 0;
	int hp_percent = 0;
	int mp = 0, maxmp = 0;
	int mp_percent = 0;
	int vital = 0;
	int str = 0, tgh = 0, dex = 0;
	int exp = 0, maxExp = 0;
	int level = 0;
	int atk = 0, def = 0;
	int quick = 0, charm = 0, luck = 0;
	int earth = 0, water = 0, fire = 0, wind = 0;
	int gold = 0;
#ifdef _NEW_MANOR_LAW
	int fame;
#endif
	int titleNo = 0;
	int dp = 0;
	QString name = "\0";
	QString freeName = "\0";
	short nameColor = 0i16;
#ifdef _ANGEL_SUMMON
	unsigned status;
#else
	unsigned short status = 0ui16;
#endif
	unsigned short etcFlag = 0ui16;
	short battlePetNo = -1i16;
	short selectPetNo[MAX_PET] = {};
	short mailPetNo = -1i16;
#ifdef _STANDBYPET
	short standbyPet;
#endif
	int battleNo = 0;
	short sideNo = 0i16;
	short helpMode = 0i16;
	ITEM item[MAX_ITEM] = {};
	ACTION* ptAct = nullptr;
	int pcNameColor = 0;
	short transmigration = 0i16;
	QString chusheng = "\0";
	QString familyName = "\0";
	int familyleader = 0;
	int channel = 0;
	int quickChannel = 0;
	int personal_bankgold = 0;
	int ridePetNo = -1;//寵物形像
	int learnride = 0;
	unsigned int lowsride = 0u;
	QString ridePetName = "\0";
	int ridePetLevel = 0;
	int familySprite = 0;
	int baseGraNo = 0;
	ITEM itempool[MAX_ITEM] = {};
	int big4fm = 0;
	int trade_confirm = 0;         // 1 -> 初始值
	// 2 -> 慬我方按下確定鍵
	// 3 -> 僅對方按下確定鍵
	// 4 -> 雙方皆按下確定鍵

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	int profession_class;
	int profession_level;
	//	int profession_exp;
	int profession_skill_point;
	char profession_class_name[32];
#endif
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	int herofloor;
#endif

#ifdef _GM_IDENTIFY		// Rog ADD GM識別
	char gm_name[GM_NAME_LEN + 1];
#endif

#ifdef _FRIENDCHANNEL  // ROG ADD 好友頻道
	char  chatRoomNum[4];
#endif
#ifdef _STREET_VENDOR
	int iOnStreetVendor;		// 擺攤模式
#endif
	int skywalker; // GM天行者??
#ifdef _MOVE_SCREEN
	BOOL	bMoveScreenMode;	// 移動熒幕模式
	BOOL	bCanUseMouse;		// 是否可以使用滑鼠移動
	int		iDestX;				// 目標點 X 座標
	int		iDestY;				// 目標點 Y 座標
#endif
#ifdef _THEATER
	int		iTheaterMode;		// 劇場模式
	int		iSceneryNumber;		// 記錄劇院背景圖號
	ACTION* pActNPC[5];		// 記錄劇場中臨時產生出來的NPC
#endif
#ifdef _NPC_DANCE
	int     iDanceMode;			// 動一動模式
#endif
#ifdef _EVIL_KILL
	int     newfame; // 討伐魔軍積分
	short   ftype;
#endif

	int debugmode;
#ifdef _SFUMATO
	int sfumato;		// 二次渲染圖層色彩
#endif
#ifdef _NEW_ITEM_
	int 道具欄狀態;
#endif
#ifdef _CHARSIGNADY_NO_
	int 簽到標記;
#endif
#ifdef _MAGIC_ITEM_
	int 法寶道具狀態;
	int 道具光環效果;
#endif
	//int state;
} PC;

typedef struct tagPARTY
{
	short useFlag = 0i16;
	int id = 0;
	int level = 0;
	int maxHp = 0;
	int hp = 0;
	int mp = 0;
	QString name = "\0";
	ACTION* ptAct = nullptr;
} PARTY;

typedef struct tagPET
{
	int index = 0;						//位置
	int graNo = 0;						// ??�k?
	int hp = 0, maxHp = 0;					// ????��??
	int mp = 0, maxMp = 0;					// ????��??
	int exp = 0, maxExp = 0;				// ??�k???????��????�k
	int level = 0;						// ???
	int atk = 0, def = 0;					// ????????�\??
	int quick = 0;						// �咋�?
	int ai = 0;							// ?�攛T
	int earth = 0, water = 0, fire = 0, wind = 0;	// 佋???
	int maxSkill = 0;					// ?��????
	int trn = 0;						// 寵物轉生數
#ifdef _SHOW_FUSION
	int fusion = 0;						// low word: 寵蛋旗標, hi word: 物種編碼
#endif
#ifdef _ANGEL_SUMMON
	unsigned status;
#else
	unsigned short status = 0ui16;			// ?????(??????)
#endif
	QString name = "\0";		// ��?��
	QString freeName = "\0";	// ???????�彙q
	short useFlag = 0i16;					// ??????????????
	short changeNameFlag = 0i16;			// �彙q?????????
#ifdef _PET_ITEM
	ITEM item[MAX_PET_ITEM] = {};		// 寵物道具
#endif
#ifdef _PETCOM_
	int oldlevel, oldhp, oldatk, oldquick, olddef;
#endif
#ifdef _RIDEPET_
	int rideflg;
#endif
#ifdef _PETBLESS_
	int blessflg;
	int blesshp;
	int blessatk;
	int blessquick;
	int blessdef;
#endif
} PET;

typedef struct tagPET_SKILL
{
	short useFlag = 0i16;
	short skillId = 0i16;
	short field = 0i16;
	short target = 0i16;
	QString name = "\0";
	QString memo = "\0";
} PET_SKILL;

//int id;		//精靈所在裝備的位置編號（頭0，身1，武器2，左飾3，右飾4）
//int kubun;//裝備中的精靈是否可用，裝上裝備為1，取下裝備後為0
typedef struct tagMAGIC
{
	short useFlag = 0i16;
	int mp = 0;
	short field = 0i16;
	short target = 0i16;
	short deadTargetFlag = 0i16;
	QString name = "\0";
	QString memo = "\0";
} MAGIC;

typedef struct tagLSTIME
{
	int year = 0;
	int day = 0;
	int hour = 0;
}LSTIME;

typedef struct tagMAP_UNIT
{
	int charType = 0;
	int id = 0;
	int x = 0;
	int y = 0;
	int dir = 0;
	int graNo = 0;
	int level = 0;
	int nameColor = 0;
	QString name = "\0";
	QString freeName = "\0";
	int walkable = 0;
	int height = 0;
	int charNameColor = 0;
	QString fmname = "\0";
	QString petname = "\0";
	int petlevel = 0;
	int classNo = 0;
	QString info = "\0";
	int money = 0;
}MAP_UNIT;

#pragma pack()

static EscapeChar escapeChar[] =
{
	{ '\n', 'n' },
	{ ',', 'c' },
	{ '|', 'z' },
	{ '\\', 'y' },
};

#endif