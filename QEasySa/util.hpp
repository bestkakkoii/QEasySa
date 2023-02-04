#pragma once
#ifndef UTIL_H
#define UTIL_H
#include <QObject>
#include <WinNls.h>
#include <sysinfoapi.h>

constexpr auto LSTIME_SECONDS_PER_DAY = 5400;
constexpr auto LSTIME_HOURS_PER_DAY = 1024;
constexpr auto LSTIME_DAYS_PER_YEAR = 100;

constexpr auto NIGHT_TO_MORNING = 700;
constexpr auto MORNING_TO_NOON = 930;
constexpr auto NOON_TO_EVENING = 200;
constexpr auto EVENING_TO_NIGHT = 300;

constexpr auto MAX_DIR = 8;

enum LSTIME_SECTION
{
	LS_NOON,
	LS_EVENING,
	LS_NIGHT,
	LS_MORNING,
};

#pragma pack(4)

typedef struct tagLSTIME
{
	int year = 0;
	int day = 0;
	int hour = 0;
}LSTIME;

#pragma pack()


class Util
{
	typedef struct tagEscapeChar
	{
		char escapechar = '\0';
		char escapedchar = '\0';
	} EscapeChar;

	//取靠近目標的最佳座標和方向
	typedef struct qdistance_s
	{
		int dir;
		qreal distance;//Euclidean
		QPoint p;
		QPointF pf;
	}qdistance_t;
private:
	EscapeChar escapeChar[4] =
	{
		{ '\n', 'n' },
		{ ',', 'c' },
		{ '|', 'z' },
		{ '\\', 'y' },
	};

	const QVector<QPoint> fix_point = {
		{0, -1},  //北3
		{1, -1},  //東北4
		{1, 0},	  //東5
		{1, 1},	  //東南6
		{0, 1},	  //南7
		{-1, 1},  //西南0
		{-1, 0},  //西1
		{-1, -1}, //西北2
	};

public:

	//int a62toi(char* a)
	//{
	//	int ret = 0;
	//	int fugo = 1;

	//	while (*a != NULL)
	//	{
	//		ret *= 62;
	//		if ('0' <= (*a) && (*a) <= '9')
	//			ret += (*a) - '0';
	//		else
	//			if ('a' <= (*a) && (*a) <= 'z')
	//				ret += (*a) - 'a' + 10;
	//			else
	//				if ('A' <= (*a) && (*a) <= 'Z')
	//					ret += (*a) - 'A' + 36;
	//				else
	//					if (*a == '-')
	//						fugo = -1;
	//					else
	//						return 0;
	//		a++;
	//	}
	//	return ret * fugo;
	//}

	int a62toi(const QString& a) const
	{
		int ret = 0;
		int sign = 1;
		for (int i = 0; i < a.length(); i++) {
			ret *= 62;
			if ('0' <= a[i] && a[i] <= '9')
				ret += a[i].unicode() - '0';
			else if ('a' <= a[i] && a[i] <= 'z')
				ret += a[i].unicode() - 'a' + 10;
			else if ('A' <= a[i] && a[i] <= 'Z')
				ret += a[i].unicode() - 'A' + 36;
			else if (a[i] == '-')
				sign = -1;
			else
				return 0;
		}
		return ret * sign;
	}

	//unsigned char* searchDelimPoint(unsigned char* src, unsigned char delim) const
	//{
	//	unsigned char* pt = src;

	//	while (1)
	//	{
	//		if (*pt == '\0')
	//			return (unsigned char*)0;

	//		if (*pt < 0x80)
	//		{
	//			if (*pt == delim)
	//			{
	//				pt++;
	//				return pt;
	//			}
	//			pt++;
	//		}
	//		else
	//		{
	//			pt++;
	//			if (*pt == '\0')
	//				return (unsigned char*)0;
	//			pt++;
	//		}
	//	}
	//}

	//int copyStringUntilDelim(unsigned char* src, char delim, int maxlen, unsigned char* out)
	//{
	//	int i;

	//	for (i = 0; i < maxlen; i++)
	//	{
	//		if (src[i] < 0x80)
	//		{
	//			if (src[i] == delim)
	//			{
	//				out[i] = '\0';
	//				return 0;
	//			}

	//			out[i] = src[i];
	//			if (out[i] == '\0')
	//				return 1;
	//		}
	//		else
	//		{
	//			out[i] = src[i];

	//			i++;
	//			if (i >= maxlen)
	//				break;

	//			out[i] = src[i];
	//			if (out[i] == '\0')
	//				return 1;
	//		}
	//	}

	//	out[i] = '\0';

	//	return 1;
	//}

	//int getStringToken(char* src, char delim, int count, int maxlen, char* out)
	//{
	//	int c = 1;
	//	int i;
	//	unsigned char* pt;

	//	pt = (unsigned char*)src;
	//	for (i = 0; i < count - 1; i++)
	//	{
	//		if (pt == (unsigned char*)0)
	//			break;

	//		pt = searchDelimPoint(pt, delim);
	//	}

	//	if (pt == (unsigned char*)0)
	//	{
	//		out[0] = '\0';
	//		return 1;
	//	}

	//	return copyStringUntilDelim(pt, delim, maxlen, (unsigned char*)out);
	//}

	int getStringToken(const QString& src, const QString& delim, int count, QString& out) const
	{
		int c = 1;
		int i = 0;

		while (c < count)
		{
			i = src.indexOf(delim, i);
			if (i == -1)
			{
				out = "";
				return 1;
			}
			i += delim.length();
			c++;
		}

		int j = src.indexOf(delim, i);
		if (j == -1)
		{
			out = src.mid(i);
			return 0;
		}

		out = src.mid(i, j - i);
		return 0;
	}

	//int getIntegerToken(char* src, char delim, int count)
	//{
	//	char s[128];

	//	getStringToken(src, delim, count, sizeof(s) - 1, s);

	//	if (s[0] == '\0')
	//		return -1;

	//	return atoi(s);
	//}

	int getIntegerToken(const QString& src, const QString& delim, int count) const
	{
		QString s;
		if (getStringToken(src, delim, count, s) == 1)
			return -1;

		return s.toInt();

	}

	//int getInteger62Token(char* src, char delim, int count)
	//{
	//	char  s[128];

	//	getStringToken(src, delim, count, sizeof(s) - 1, s);
	//	if (s[0] == '\0')
	//		return -1;

	//	return a62toi(s);
	//}

	int getInteger62Token(const QString& src, const QString& delim, int count) const
	{
		QString s;
		getStringToken(src, delim, count, s);
		if (s.isEmpty())
			return -1;
		return a62toi(s);
	}

	//char* makeStringFromEscaped(char* src) const
	//{
	//	int		srclen = strlen(src);
	//	int		searchindex = 0;
	//	for (int i = 0; i < srclen; i++) {
	//		if (IsDBCSLeadByte(src[i])) {
	//			src[searchindex++] = src[i++];
	//			src[searchindex++] = src[i];
	//		}
	//		else {
	//			if (src[i] == '\\') {
	//				int j;
	//				i++;
	//				for (j = 0; j < sizeof(escapeChar) / sizeof(escapeChar[0]); j++)
	//					if (escapeChar[j].escapedchar == src[i]) {
	//						src[searchindex++] = escapeChar[j].escapechar;
	//						goto NEXT;
	//					}
	//				src[searchindex++] = src[i];
	//			}
	//			else
	//				src[searchindex++] = src[i];
	//		}
	//	NEXT:
	//		;
	//	}
	//	src[searchindex] = '\0';
	//	return src;
	//}

	QString makeStringFromEscaped(QString& src) const
	{
		int srclen = src.length();
		int searchIndex = 0;
		do {
			try
			{
				for (int i = 0; i < srclen; i++) {
					if (src.at(i).isHighSurrogate()) {
						src[searchIndex++] = src.at(i++);
						src[searchIndex++] = src.at(i);
					}
					else {
						if (src.at(i) == '\\') {
							int j;
							i++;
							for (j = 0; j < sizeof(escapeChar) / sizeof(escapeChar[0]); j++) {
								if (escapeChar[j].escapedchar == src.at(i).toLatin1()) {
									src[searchIndex++] = escapeChar[j].escapechar;
									break;
								}
							}
							if (j == sizeof(escapeChar) / sizeof(escapeChar[0])) {
								src[searchIndex++] = src.at(i);
							}
						}
						else {
							src[searchIndex++] = src.at(i);
						}
					}
				}
				src.truncate(searchIndex);
			}
			catch (...)
			{
				break;
			}
		} while (false);

		return src;
	}

	QString Tokenize(QString& data, QString del, Qt::CaseSensitivity s = Qt::CaseSensitive) const
	{
		int pos = data.indexOf(del, 0, s);
		if (pos != -1)
		{
			QString retstring = data.mid(0, pos);
			data = data.mid(pos + del.size());
			return retstring;
		}
		else
		{
			QString retstring = data;
			data.clear();
			return retstring;
		}
	}

	int safe_atoi(const QString& str) const
	{
		QByteArray ba = str.toLatin1();
		long result = 0ul;
		try
		{
			const char* c_str = ba.data();
			if (!c_str) {
				return 0;
			}
			char* end;
			result = strtol(c_str, &end, 10);

			if (end == c_str || *end != '\0') {
				// invalid conversion
				return 0;
			}
		}
		catch (...)
		{
			return 0;
		}

		if (result > INT_MAX || result < INT_MIN) {
			// overflow or underflow
			return 0;
		}

		return static_cast<int>(result);
	}

	int safe_atoi(const char* str) const
	{
		if (!str) {
			return 0;
		}
		long result = 0ul;
		try
		{
			char* end;
			result = strtol(str, &end, 10);

			if (end == str || *end != '\0') {
				// invalid conversion
				return 0;
			}
		}
		catch (...)
		{
			return 0;
		}

		if (result > INT_MAX || result < INT_MIN) {
			// overflow or underflow
			return 0;
		}

		return static_cast<int>(result);
	}

	int safe_memcmp(const void* s1, size_t len1, const void* s2, size_t len2) const
	{
		if (!s1 || !s2) {
			return INT_MIN;
		}

		size_t min_len = (len1 < len2) ? len1 : len2;

		const unsigned char* p1 = (const unsigned char*)s1;
		const unsigned char* p2 = (const unsigned char*)s2;

		int result = 0;
		for (size_t i = 0; i < min_len && result == 0; i++) {
			result = p1[i] - p2[i];
		}

		if (result == 0) {
			if (len1 < len2) {
				return -1;
			}
			else if (len1 > len2) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else {
			return result;
		}
	}

	char* safe_strstr(char* src, size_t srclen, const char* target, size_t targetlen) const
	{
		if (srclen < targetlen || !src || !target) {
			return nullptr;
		}

		for (size_t i = 0; i <= srclen - targetlen; ++i) {
			if (safe_memcmp(src + i, targetlen, target, targetlen) == 0) {
				return src + i;
			}
		}

		return nullptr;
	}

	LSTIME_SECTION getLSTime(LSTIME* lstime, int m_FirstTime, int m_serverTime)
	{
		if (!lstime)
			return LS_NOON;
		RealTimeToSATime(lstime, m_FirstTime, m_serverTime);
		if (NIGHT_TO_MORNING < lstime->hour
			&& lstime->hour <= MORNING_TO_NOON)
			return LS_MORNING;
		else if (NOON_TO_EVENING < lstime->hour
			&& lstime->hour <= EVENING_TO_NIGHT)
			return LS_EVENING;
		else if (EVENING_TO_NIGHT < lstime->hour
			&& lstime->hour <= NIGHT_TO_MORNING)
			return LS_NIGHT;
		else
			return LS_NOON;
	}

	int CalcBestFollowPointByDstPoint(int floor, const QPoint& src, const QPoint& dst, QPoint* ret, bool enableExt, int npcdir)
	{

		QVector<qdistance_t> disV;// <distance, point>

		int d = 0;
		int invalidcount = 0;
		for (const QPoint& it : fix_point)
		{
			qdistance_t c = {};
			c.dir = d;
			c.pf = dst + it;
			c.p = dst + it;
			if (src == c.p)//如果已經在目標點
			{
				if (ret)
					*ret = c.p;
				int n = c.dir + 4;
				return ((n) <= (7)) ? (n) : ((n)-(MAX_DIR));
			}
			//if (_MAP_IsPassable(floor, src, dst + it))//確定是否可走
			//{
				//計算src 到 c.p直線距離
			c.distance = std::sqrt(std::pow((qreal)src.x() - c.pf.x(), 2) + std::pow((qreal)src.y() - c.pf.y(), 2));
			//}
			//else//不可走就隨便加個超長距離
			//{
				//c.distance = std::numeric_limits<double>::max();
				//++invalidcount;
			//}
			++d;
			disV.append(c);
		}

		//if (invalidcount >= MAX_DIR && enableExt && npcdir != -1)//如果周圍8格都不能走搜尋NPC面相方向兩格(中間隔著櫃檯)
		//{
		//	for (int i = 0; i < 7; ++i)
		//	{
		//		QPoint newP;
		//		switch (i)//找出NPC面相方向的兩格
		//		{
		//		case 0://NPC面相北找往北兩格
		//			newP = dst + QPoint(0, -2);
		//			break;
		//		case 2://NPC面相東
		//			newP = dst + QPoint(2, 0);
		//			break;
		//		case 4://NPC面相南
		//			newP = dst + QPoint(0, 2);
		//			break;
		//		case 6://NPC面相西
		//			newP = dst + QPoint(-2, 0);
		//			break;
		//		}
		//		if (_MAP_IsPassable(floor, src, newP) || src == newP)//確定是否可走
		//		{
		//			//qdistance_t c = {};
		//			//要面相npc的方向  (當前人物要面向newP的方向)
		//			if (ret)
		//				*ret = newP;
		//			int n = npcdir + 4;
		//			return ((n) <= (7)) ? (n) : ((n)-(MAX_DIR));
		//		}
		//	}
		//	return -1;
		//}
		//else if (invalidcount >= 8)// 如果周圍8格都不能走
		//{
		//	return -1;
		//}

#if _MSVC_LANG > 201703L
		std::ranges::sort(disV, compareDistance);
#else
		std::sort(disV.begin(), disV.end(), [](qdistance_t& a, qdistance_t& b) { return (a.distance < b.distance); });
#endif
		if (!disV.size()) return -1;
		if (ret)
			*ret = disV.at(0).p;
		//計算方向
		int n = disV.at(0).dir + 4;
		return ((n) <= (7)) ? (n) : ((n)-(MAX_DIR));//返回方向
	}

private:
	const long era = (long)912766409 + 5400;
	void RealTimeToSATime(LSTIME* lstime, int m_FirstTime, int m_serverTime)
	{
		if (!lstime)
			return;
		Util util;
		long lsseconds; /* LS????? */
		long lsdays; /* LS????? */

		//cary 十五
		lsseconds = (GetTickCount64() - m_FirstTime) / 1000 + m_serverTime - era;

		lstime->year = (int)(lsseconds / (LSTIME_SECONDS_PER_DAY * LSTIME_DAYS_PER_YEAR));

		lsdays = lsseconds / LSTIME_SECONDS_PER_DAY;
		lstime->day = lsdays % LSTIME_DAYS_PER_YEAR;

		//(750*12)
		lstime->hour = (int)(lsseconds % LSTIME_SECONDS_PER_DAY)

			* LSTIME_HOURS_PER_DAY / LSTIME_SECONDS_PER_DAY;

		return;
	}



	////返回從nstart開始到token在source中出現位置之間的字符串
	//static void __stdcall Tokenize(char* source, char* dest, const char* token, int& nstart)
	//{
	//	char* p, * p1 = NULL;
	//	//要查找字符串的長度
	//	int len = strlen(token);
	//	//總字符串長度
	//	int totallen = strlen(source);
	//	//如果起始查找位置大於等於原始字符串長度，則返回
	//	if (nstart >= totallen) {
	//		dest[0] = 0;
	//		return;
	//	}
	//	//定義開始查找的位置
	//	p = source + nstart;
	//	//找出token在字符串的位置
	//	p1 = strstr(p, token);
	//	//如果找到
	//	if (p1) {
	//		//下一個啟始位置
	//		nstart = nstart + (p1 - p) + len;
	//		//覆制子符串到dest中
	//		strncpy(dest, p, p1 - p);
	//		dest[p1 - p] = 0;
	//		return;
	//	}
	//	//覆制最後一部分
	//	strncpy(dest, p, strlen(p));
	//	dest[strlen(p)] = 0;
	//	nstart += strlen(p);
	//}
};

#endif