
#include "mainform.h"

HINSTANCE g_hInstance = NULL;
WNDPROC g_hOldProc = NULL;
HANDLE g_hThread = INVALID_HANDLE_VALUE;
HWND g_hWnd = NULL;
HMODULE g_gameModuleBase = NULL;
std::wstring g_GameExeFilePath = L"\0";
std::shared_ptr<spdlog::logger> g_cont;
MainForm* g_main = nullptr;

#include <QtNetwork>
#include <QLineEdit>
#include <QDesktopServices>
QList<QPair<QString, QString>> GetRemoteMachineIdList(const QString& url)
{
	QString result;
	QNetworkAccessManager manager;
	QNetworkRequest request(QUrl(url, QUrl::TolerantMode));
	QNetworkReply* reply = manager.get(request);
	QEventLoop eventLoop;
	QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
	eventLoop.exec();
	if (reply->error() == QNetworkReply::NoError) {
		result = reply->readAll();
	}
	else {
		result = "Error: " + reply->errorString();
		delete reply;
		QMessageBox::critical(nullptr, QObject::tr("錯誤"), result);
		return {};
	}
	delete reply;

	QList<QPair<QString, QString>> l;
	QStringList list = result.split("\n", Qt::SkipEmptyParts);
	for (const QString& it : list)
	{

		QStringList ll = it.split("//", Qt::SkipEmptyParts);
		if (ll.size() == 2)
		{
			QPair<QString, QString> pair(ll.at(0).simplified(), ll.at(1).simplified());
			l.append(pair);
		}
	}
	return l;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	STATICINS(GameService);
	switch (message)
	{
	case WM_CLOSE:
	{
		g_main->close();
		break;
	}
	case WM_SETTEXT:
	{
		UINT ACP = ::GetACP();
		std::wstring wsmsg = g_GameService.g_caption.toStdWString();

		//如果是繁體中文系統就直接W發
		if (950 == ACP)
		{
			LPCWSTR lpString = wsmsg.c_str();
			lParam = (LPARAM)lpString;
			return CallWindowProcW(g_hOldProc, hWnd, message, wParam, lParam);
		}
		else
		{
			char buf[1024] = {};
			wchar_t wbuf[1024] = {};
			int size = lstrlenW(wsmsg.c_str());
			LCMapStringW(LOCALE_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wsmsg.c_str(), size + 1, wbuf, sizeof(wbuf));

			QString newstr = QString::fromStdWString(wbuf);
			std::string smsg = fromUnicode(newstr);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", smsg.c_str());

			LPCSTR lpString = buf;
			lParam = (LPARAM)lpString;
			break;
		}
	}
	case WM_SA_WALKPOS:
	{
		//g_GameService.WalkPos(QPoint{ (int)wParam, (int)lParam });
		g_GameService.WM_MoveTo(QPoint{ (int)wParam, (int)lParam });
		return 1;
	}
	case WM_SA_TURNTO:
	{
		g_GameService.WM_TurnTo(lParam);
		return 1;
	}
	case WM_SA_JOINLEAVE:
	{
		g_GameService.WM_JoinLeave(lParam);
		return 1;
	}
	case WM_SA_EO:
	{
		g_GameService.WM_EO();
		return 1;
	}
	case WM_SA_SET_SPEED:
	{
		g_GameService.WM_SetSpeed(lParam);
		return 1;
	}
	default:
		break;
	}
	return CallWindowProcA(g_hOldProc, hWnd, message, wParam, lParam);
}

HWND WINAPI GetCurrentWindowHandle();
MainForm::MainForm(QWidget* parent)
	: QMainWindow(parent)
{
	setupUi(this);
	STATICINS(GameService);
	setAttribute(Qt::WA_DeleteOnClose);

	if (::GetACP() != 950)
	{
		//非繁體系統則讀取翻譯文件
		m_translator.load(QString("%1/qt_zh_CN.qm").arg(QApplication::applicationDirPath()));
		qApp->installTranslator(&m_translator);
		//刷新UI
		this->retranslateUi(this);
	}

	this->listView->setModel(g_GameService.GetLogModelPointer());
	this->listView->setUniformItemSizes(true);
	this->listView->installEventFilter(this);

	this->lineEdit_chat->installEventFilter(this);

	const QStringList serverList = {
		tr("滿天星"), ("薰衣草"),
		tr("紫羅蘭"), tr("風信子"),
	};
	comboBox_userdata_3->addItems(serverList);
	comboBox_userdata_3->setCurrentIndex(3);

	QMainWindow::statusBar()->setStyleSheet("color: rgb(250, 250, 250);background-color: rgb(31, 31, 31);border:none");
	QMainWindow::statusBar()->setSizeGripEnabled(false);

	m_permanent = q_check_ptr(new QLabel(this));
	if (m_permanent)
	{
		m_permanent->setFrameStyle(QFrame::NoFrame);
		m_permanent->setOpenExternalLinks(true);
		QMainWindow::statusBar()->addPermanentWidget(m_permanent);
	}

	m_childstatus = q_check_ptr(new QLabel(this));
	if (m_childstatus)
	{
		m_childstatus->setFrameStyle(QFrame::NoFrame);
		m_childstatus->setOpenExternalLinks(true);
		QMainWindow::statusBar()->addWidget(m_childstatus);
	}

	QMainWindow::statusBar()->setStyleSheet("color: rgb(250, 250, 250);background-color: rgb(31, 31, 31);border:none");

	QString url = "https://www.lovesa.cc/StoneAge/data/machineid.txt";
	QList<QPair<QString, QString>> vec = GetRemoteMachineIdList(url);
	url.clear();

	if (vec.isEmpty())
	{
		QMessageBox::critical(nullptr, tr("錯誤"), tr("無法獲取機器碼列表"));
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return;
	}

	if (g_GameService.g_cpuid.isEmpty())
	{
		//cpuid is empty
		QMessageBox::critical(nullptr, tr("錯誤"), tr("無法獲取機器碼"));
		vec.clear();
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return;
	}

	QString title = "\0";
	for (int i = 0; i < vec.size(); ++i)
	{
		if (vec.at(i).first == g_GameService.g_cpuid)
		{
			title = vec.at(i).second;
			break;
		}
	}

	if (title.isEmpty())
	{
		vec.clear();
		QMessageBox::warning(nullptr, tr("警告"), tr("您沒有獲得許用此程序的許可，您的機器碼是:") + g_GameService.g_cpuid);
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return;
	}
	vec.clear();

	QString group(tr("進階用戶"));
	QString color("#FF9900");
	if (title == "管理員")
	{
		group = tr("管理員");
		color = "#FF0000";
		title = "無";
	}

	QString ex(tr("授權給:%1").arg(title));
	const QString Status(QString(tr(R"(<font color="%1">%2</font> %3 <a href="https://www.lovesa.cc" style="color:#9A9EF9;">lovesa.cc</a> by Bestkakkoii)"))
		.arg(color, group, ex));
	UpdateStaticBar(Status);
	setWindowTitle("QEasySa");
	title.clear();

	g_hWnd = GetCurrentWindowHandle();
	if (!g_hWnd)
	{
		close();
		return;
	}

	g_GameService.initialize();
	LoadUserFromLua();

	if (g_GameService.IS_ONLINE_FLAG)
	{
		g_GameService.Send(WM_SA_EO, NULL, NULL);
	}

	connect(&m_timer, &QTimer::timeout, this, &MainForm::OnTimer);
	m_timer.start(200);
	connect(&m_battletimer, &QTimer::timeout, this, &MainForm::OnBattleTimer);
	m_battletimer.start(100);
	connect(&g_GameService, &GameService::sigNotifyUpdateData, this, &MainForm::OnUpdateData);
}

bool MainForm::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Delete)
		{
			STATICINS(GameService);
			g_GameService.ClearLog();
			return true;
		}

		else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
		{
			//lineEdit_chat
			if (obj == this->lineEdit_chat)
			{
				QString text = this->lineEdit_chat->text();
				if (text.isEmpty()) return true;
				STATICINS(GameService);
				int fd = g_GameService.GetSocket();
				g_GameService.lssproto_TK_send(fd, text);
				m_lineEditList.append(text);
				this->lineEdit_chat->clear();
				return true;
			}

		}

		else if (keyEvent->key() == Qt::Key_Up)
		{
			//lineEdit_chat
			if (obj == this->lineEdit_chat)
			{
				if (m_lineEditList.isEmpty()) return true;
				if (m_lineEditList.size() == 1)
				{
					this->lineEdit_chat->setText(m_lineEditList.at(0));
					return true;
				}
				if (m_lineEditList.size() > 1)
				{
					if (m_lineEditListIndex == -1)
					{
						m_lineEditListIndex = m_lineEditList.size() - 1;
						this->lineEdit_chat->setText(m_lineEditList.at(m_lineEditListIndex));
						return true;
					}
					else
					{
						if (m_lineEditListIndex == 0)
						{
							m_lineEditListIndex = m_lineEditList.size() - 1;
							this->lineEdit_chat->setText(m_lineEditList.at(m_lineEditListIndex));
							return true;
						}
						else
						{
							m_lineEditListIndex--;
							this->lineEdit_chat->setText(m_lineEditList.at(m_lineEditListIndex));
							return true;
						}
					}
				}
			}
		}

		else if (keyEvent->key() == Qt::Key_Down)
		{
			//lineEdit_chat
			if (obj == this->lineEdit_chat)
			{
				if (m_lineEditList.isEmpty()) return true;
				if (m_lineEditList.size() == 1)
				{
					this->lineEdit_chat->setText(m_lineEditList.at(0));
					return true;
				}
				if (m_lineEditList.size() > 1)
				{
					if (m_lineEditListIndex == -1)
					{
						m_lineEditListIndex = 0;
						this->lineEdit_chat->setText(m_lineEditList.at(m_lineEditListIndex));
						return true;
					}
					else
					{
						if (m_lineEditListIndex == m_lineEditList.size() - 1)
						{
							m_lineEditListIndex = 0;
							this->lineEdit_chat->setText(m_lineEditList.at(m_lineEditListIndex));
							return true;
						}
						else
						{
							m_lineEditListIndex++;
							this->lineEdit_chat->setText(m_lineEditList.at(m_lineEditListIndex));
							return true;
						}
					}
				}
			}
		}
	}


	return false;
}

MainForm::~MainForm()
{
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
}

void MainForm::closeEvent(QCloseEvent* e)
{
	STATICINS(GameService);
	m_timer.stop();
	autojoin_future.cancel();
	autojoin_future.waitForFinished();
	g_GameService.uninitialize();
	QMainWindow::closeEvent(e);
}

void MainForm::UpdateStaticBar(const QString& data)
{
	if (m_permanent)
	{
		QTextBrowser browser;
		browser.document()->setDefaultStyleSheet("{ color: #9A9EF9; }");
		m_permanent->setText(data);
	}
}

void MainForm::UpdateBar(const QString& data)
{
	if (m_childstatus)
	{
		QTextBrowser browser;
		browser.document()->setDefaultStyleSheet("{ color: #9A9EF9; }");
		m_childstatus->setText(data);
	}
}

void MainForm::LoadUserFromLua()
{
	STATICINS(GameService);
	//lua username and password from lua table
	QString file = m_userFileName;
	QFile f(file);
	if (!f.exists()) return;

	//read all
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return;
	}

	QString script = f.readAll();
	f.close();

	std::string sscript = script.toStdString();
	if (sscript.empty()) return;


	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::io, sol::lib::os, sol::lib::math, sol::lib::debug);
	//use saft mode
	sol::protected_function_result ret = lua.safe_script(sscript);
	lua.collect_garbage();
	if (!ret.valid())
	{
		sol::error err = ret;
		QString errstr = err.what();

		g_GameService.AppendLog(errstr);
		return;
	}

	if (lua["User"].get_type() != sol::type::table)
	{
		g_GameService.AppendLog(tr("User變量不是一個表"));
		return;
	}

	this->comboBox_userdata->clear();
	this->comboBox_userdata_2->clear();

	sol::table t = lua["User"];
	for (const std::pair<sol::object, sol::object>& v : t)
	{
		//istable
		if (!v.second.is<sol::table>())
		{
			g_GameService.AppendLog(tr("表成員不是一個表"));
			continue;
		}

		sol::table t2 = v.second.as<sol::table>();
		if (t2.size() != 2)
		{
			g_GameService.AppendLog(QString(tr("子表成員數量不是2 當前子表成員數量為:%1")).arg(t2.size()));
			continue;
		}

		if (!t2[1].is<std::string>() || !t2[2].is<std::string>())
		{
			g_GameService.AppendLog(tr("子表成員不是字符串類型"));
			continue;
		}
		QString username = QString::fromStdString(t2[1].get<std::string>());
		QString password = QString::fromStdString(t2[2].get<std::string>());
		if (!username.isEmpty() && !password.isEmpty())
		{
			this->comboBox_userdata->addItem(username, password);
			this->comboBox_userdata_2->addItem(password, username);
		}
		else
		{
			g_GameService.AppendLog(tr("帳號或密碼為空"));
		}
	}

	this->comboBox_userdata_2->setCurrentIndex(0);
	this->comboBox_userdata->setCurrentIndex(0);

}

void MainForm::on_checkBox_EnableFastBattle_stateChanged(int state)
{
	STATICINS(GameService);
	int fd = g_GameService.GetSocket();
	g_GameService.IS_FAST_BATTLE = state == Qt::Checked;
}

void MainForm::on_checkBox_EnableAutoBattle_stateChanged(int state)
{
	STATICINS(GameService);
	int* pEnable = CONVERT_G_GAMEVAR(int*, 0x02EA934);
	*pEnable = (state == Qt::Checked) ? 3 : 0;
	//enum
	//{
	//	FS_AUTOBATTLE = 0x1000,
	//};
	//STATICINS(GameService);
	//int fd = g_GameService.GetSocket();

	//WORD* flag = g_GameService.g_switcher_flag;
	//g_GameService.AppendLog(QString::number(*flag, 16));
	//if (state == Qt::Checked)
	//{
	//	if ((*flag) & FS_AUTOBATTLE)
	//		return;
	//	(*flag) |= FS_AUTOBATTLE;
	//}
	//else
	//{
	//	if (!((*flag) & FS_AUTOBATTLE))
	//		return;
	//	(*flag) &= ~FS_AUTOBATTLE;
	//}
	//g_GameService.lssproto_SaMenu_send(fd, *flag);
}

void MainForm::on_checkBox_EnableAutoCombat_stateChanged(int state)
{

	STATICINS(GameService);
	if (!g_GameService.IS_ONLINE_FLAG)
	{
		this->checkBox_EnableAutoCombat->blockSignals(true);
		this->checkBox_EnableAutoCombat->setCheckState(Qt::Unchecked);
		this->checkBox_EnableAutoCombat->blockSignals(false);
		return;
	}


	int fd = g_GameService.GetSocket();

	//回點78
	//傳送74
	//陪練75
	//遇敵76
	//登出79

	bool* pEnableAutoCombat = CONVERT_G_GAMEVAR(bool*, 0x39E89B);
	if (state == Qt::Checked && *pEnableAutoCombat) return;
	if (state == Qt::Unchecked && !*pEnableAutoCombat) return;
	*pEnableAutoCombat = state == Qt::Checked;
	g_GameService.IS_AUTO_COMBAT = state == Qt::Checked;
	if (*pEnableAutoCombat)
		g_GameService.lssproto_SaMenu_send(fd, 18);
	else
	{
		g_GameService.lssproto_SaMenu_send(fd, 11);
		if (!g_GameService.IS_BATTLE_FLAG)
		{
			g_GameService.EnableMoveLock(false);
		}
	}
}

void MainForm::on_checkBox_EnableAutoEatMeat_stateChanged(int state)
{
	STATICINS(GameService);
	g_GameService.IS_AUTO_EATMEAT = state == Qt::Checked;
}

void MainForm::on_checkBox_EnableAutoLogin_stateChanged(int state)
{
	STATICINS(GameService);
	g_GameService.IS_AUTO_LOGIN = state == Qt::Checked;
}

void MainForm::on_checkBox_EnableBattleMagicHeal_stateChanged(int state)
{
	STATICINS(GameService);
	g_GameService.IS_AUTO_BATTLE_MAGIC_HEAL = state == Qt::Checked;
}

void MainForm::on_checkBox_EnableDebugMode_stateChanged(int state)
{
	STATICINS(GameService);
	g_GameService.IS_DEBUG_MODE = state == Qt::Checked;
}

void MainForm::on_checkBox_EnableAutoJoin_stateChanged(int state)
{
	STATICINS(GameService);
	g_GameService.IS_AUTO_JOIN = state == Qt::Checked;
}

void MainForm::on_checkBox_EnableAutoReCombat_stateChanged(int state)
{
	STATICINS(GameService);
	g_GameService.IS_AUTO_RECOMBAT = state == Qt::Checked;
}

void MainForm::on_spinBox_BattleMagicHealValue_valueChanged(int value)
{
	STATICINS(GameService);
	g_GameService.BATTLE_AUTO_MAGIC_HEAL_VALUE = value;
}

void MainForm::on_spinBox_speed_valueChanged(int value)
{
	STATICINS(GameService);
	g_GameService.g_speed = value;
}

void MainForm::on_comboBox_userdata_currentIndexChanged(int index)
{
	comboBox_userdata_2->setCurrentIndex(index);
}

void MainForm::on_comboBox_userdata_editTextChanged(const QString& text)
{
	STATICINS(GameService);
	g_GameService.SetUserAccountData(text, 0);
}

void MainForm::on_comboBox_userdata_currentTextChanged(const QString& text)
{
	STATICINS(GameService);
	g_GameService.SetUserAccountData(text, 0);
}

void MainForm::on_comboBox_userdata_2_currentIndexChanged(int index)
{
	comboBox_userdata->setCurrentIndex(index);
}

void MainForm::on_comboBox_userdata_2_editTextChanged(const QString& text)
{
	STATICINS(GameService);
	g_GameService.SetUserAccountData(text, 1);
}

void MainForm::on_comboBox_userdata_2_currentTextChanged(const QString& text)
{
	STATICINS(GameService);
	g_GameService.SetUserAccountData(text, 1);
}

void MainForm::on_comboBox_userdata_3_currentIndexChanged(int index)
{
	STATICINS(GameService);
	g_GameService.g_server_selected = index;
}

void MainForm::on_comboBox_autojoin_currentTextChanged(const QString& text)
{
	STATICINS(GameService);
	g_GameService.g_autojoin_leader = text;
}

void MainForm::on_comboBox_autojoin_editTextChanged(const QString& text)
{
	STATICINS(GameService);
	g_GameService.g_autojoin_leader = text;
}

void MainForm::on_comboBox_autojoin_clicked()
{
	STATICINS(GameService);
	QHash<int, MAP_UNIT> hash_units = g_GameService.GetMapUnits();
	PC pc = g_GameService.GetCharData();
	int n = 0;
	QVector<MAP_UNIT> vec_units;
	for (auto it = hash_units.begin(); it != hash_units.end(); ++it)
	{
		MAP_UNIT unit = it.value();
		if (unit.charType == MFLAG_HUMAN && unit.name != pc.name)
		{
			vec_units.push_back(unit);
		}
	}

	if (this->comboBox_autojoin->count() < vec_units.size())
	{
		for (int i = 0; i < vec_units.size(); i++)
		{
			this->comboBox_autojoin->addItem("");
		}
	}
	else
	{
		//remove extra items
		for (int i = this->comboBox_autojoin->count() - 1; i >= vec_units.size(); i--)
		{
			this->comboBox_autojoin->removeItem(i);
		}
	}

	std::sort(vec_units.begin(), vec_units.end(), [](const MAP_UNIT& a, const MAP_UNIT& b) -> bool
		{
			return a.name < b.name;
		});
	for (const MAP_UNIT& unit : vec_units)
	{
		if (!unit.name.isEmpty() && unit.charType == MFLAG_HUMAN)
		{
			this->comboBox_autojoin->setItemText(n, unit.name);
			++n;
		}
	}
}

void MainForm::on_pushButton_clicked()
{
	STATICINS(GameService);
	int fd = g_GameService.GetSocket();
	g_GameService.g_echo_timer.restart();
	g_GameService.g_enable_show_echo_ping = true;
	g_GameService.Send(WM_SA_EO, NULL, NULL);
}

void MainForm::on_pushButton_logout_clicked()
{
	STATICINS(GameService);
	*CONVERT_G_GAMEVAR(int*, 0x29D1AC24) = 7;
	*CONVERT_G_GAMEVAR(int*, 0x29D1AC20) = 0;
}

void MainForm::on_pushButton_logback_clicked()
{
	STATICINS(GameService);
	int fd = g_GameService.GetSocket();
	g_GameService.lssproto_CharLogout_send(fd, 1);
}
//萨姆吉尔的肉店
void MainForm::on_pushButton_sellall_clicked()
{
	STATICINS(GameService);
	g_GameService.ITEM_SellItem("肉店", "肉");
	g_GameService.ITEM_SellItem("素材", "壳");
}

void MainForm::on_pushButton_joinleave_clicked()
{
	STATICINS(GameService);
	int fd = g_GameService.GetSocket();
	int enable = *CONVERT_G_GAMEVAR(int*, 0x29D1A934) == 1 ? 0 : 1;
	g_GameService.lssproto_PR_send(fd, enable);
}

void MainForm::on_pushButton_userdata_clicked()
{
	STATICINS(GameService);
	QString file = m_userFileName;

	//如果沒有就新建
	QFile f(file);
	if (!f.exists())
	{
		QFile frecord(QCoreApplication::applicationDirPath() + "/data/record.txt");

		QString acct;
		if (frecord.exists() && frecord.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			//1642687802  >> 帳號
			//fhjln;=?AC  >> 密碼(被加密了)
			QString data = frecord.readAll();
			frecord.close();
			QStringList l = data.simplified().split(" ");
			if (l.size() == 2)
			{
				acct = l.at(0).simplified();
			}

		}

		if (f.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QString script = "\0";
			if (::GetACP() == 950)
				script = QString("User = {\n    { --[[帳號]] \"%1\", --[[密碼]] \"\" },\n    { \"\", \"\" },\n};\n\nreturn User;").arg(acct);
			else
				script = QString("User = {\n    { --[[帐号]] \"%1\", --[[密码]] \"\" },\n    { \"\", \"\" },\n};\n\nreturn User;").arg(acct);
			f.write(script.toStdString().c_str());
			f.close();
			LoadUserFromLua();
		}
	}
	//使用默認方式打開
	QDesktopServices::openUrl(QUrl::fromLocalFile(file));
}

void MainForm::on_pushButton_userdatarefresh_clicked()
{
	LoadUserFromLua();
}

void MainForm::OnUpdateData()
{
	STATICINS(GameService);
	QScopedPointer<PC> ppc(q_check_ptr(new PC(g_GameService.GetCharData())));
	Map map = g_GameService.GetMap();
	std::wstring wmap = map.name.toStdWString();
	QString qmap = "\0";
	EXP exp = g_GameService.GetExp();

	UINT ACP = ::GetACP();
	if (950 == ACP)
	{
		//繁體系統地圖名要轉繁體否則遊戲視窗標題會亂碼
		wchar_t wbuf[256] = {};
		int size = lstrlenW(wmap.c_str());
		LCMapStringW(LOCALE_SYSTEM_DEFAULT, LCMAP_TRADITIONAL_CHINESE, wmap.c_str(), size + 1, wbuf, sizeof(wbuf));
		qmap = QString::fromStdWString(wbuf);
	}

	g_GameService.g_caption = QString(tr("%3 LV:%4 (剩餘經驗:%5) 血:%6/%7(%8%9) 氣:%10 地圖:%11(%12)[%13,%14]")) \
		.arg(ppc->name) \
		.arg(ppc->level).arg(exp.left) \
		.arg(ppc->hp).arg(ppc->maxhp).arg(ppc->hp_percent).arg("%").arg(ppc->mp) \
		.arg(map.name.isEmpty() ? "--" : qmap).arg(map.floor) \
		.arg(*g_GameService.g_player_xpos).arg(*g_GameService.g_player_ypos);

	setWindowTitle(g_GameService.g_caption);
	g_GameService.Send(WM_SETTEXT, NULL, NULL);
}

void MainForm::OnBattleTimer()
{
	STATICINS(GameService);
	qbattle_data_t bt = g_GameService.GetBattleData();

	static constexpr auto msecToString = [](int msec) -> QString
	{
		int sec = msec / 1000;
		int min = sec / 60;
		int hour = min / 60;
		int day = hour / 24;
		return QString("%1天%2時%3分%4秒").arg(day).arg(hour % 24).arg(min % 60).arg(sec % 60);
	};

	EXP exp = g_GameService.GetExp();
	//計算平均經驗
	int total_online_time = g_GameService.g_online_timer.elapsed();
	QString szonline_time = msecToString(total_online_time);

	float avgExp = 0.0f;
	QString szlefttime = "\0";
	if (total_online_time > 0)
	{
		//每小時平均;
		avgExp = exp.total * 3600.0 / total_online_time;

		//計算預計升級
		int leftexp = exp.left;
		int lefttime = 0;
		if (avgExp > 0)
		{
			lefttime = leftexp / avgExp;

			szlefttime = msecToString(lefttime * 1000);
		}
	}

	if (g_GameService.IS_ONLINE_FLAG)
	{
		if (g_GameService.IS_BATTLE_FLAG)
		{
			bt.tickcount = g_GameService.g_battle_timer.elapsed();
			bt.tickcount_cache = bt.tickcount;
			g_GameService.SetBattleData(bt);
		}
		else
		{
			OnUpdateData();
		}
	}

	QString msg = QString("[%1] %2") \
		.arg(!g_GameService.IS_ONLINE_FLAG ? tr(R"(<font color="#9E9E9E">離線</font>)") : \
			g_GameService.IS_BATTLE_FLAG ? QString(tr(R"(<font color="#FF5050">戰鬥中</font>)")) : \
			tr(R"(<font color="#BEFF96">平時</font>)")) \
		.arg(QString(tr("戰鬥總場數 %1 場 歷時: %2 秒(總歷時: %3 秒) 總經驗:%4 平均:%5 預計:%6 時間:%7")).arg(bt.total_battle_count) \
			.arg(QString::number(bt.tickcount / 1000.0, 'f', 2)) \
			.arg(QString::number(bt.total_duration / 1000.0, 'f', 2)).arg(exp.total).arg(QString::number(avgExp, 'f', 2)).arg(szlefttime).arg(szonline_time));
	UpdateBar(msg);
}

void MainForm::OnTimer()
{
	STATICINS(GameService);

	g_GameService.Send(WM_SA_SET_SPEED, NULL, g_GameService.g_speed);

	//在線
	if (g_GameService.IS_ONLINE_FLAG)
	{
		//戰鬥中
		if (g_GameService.IS_BATTLE_FLAG)
		{
		}
		//非戰鬥
		else
		{
			bool* pEnableAutoCombat = CONVERT_G_GAMEVAR(bool*, 0x39E89B);
			if (this->checkBox_EnableAutoCombat->isChecked() != *pEnableAutoCombat && !this->checkBox_EnableAutoCombat->hasFocus())
			{
				this->checkBox_EnableAutoCombat->setChecked(*pEnableAutoCombat);
			}

			int* pEnable = CONVERT_G_GAMEVAR(int*, 0x02EA934);
			if (this->checkBox_EnableAutoBattle->isChecked() != (*pEnable == 3) && !this->checkBox_EnableAutoBattle->hasFocus())
			{
				this->checkBox_EnableAutoBattle->setChecked(*pEnable == 3);
			}

			//自動組隊
			if (g_GameService.IS_AUTO_JOIN)
			{
				int enable = *CONVERT_G_GAMEVAR(int*, 0x29D1A934);
				if (enable == 0)
				{
					QHash<int, MAP_UNIT> hash_units = g_GameService.GetMapUnits();
					for (auto it = hash_units.begin(); it != hash_units.end(); ++it)
					{
						MAP_UNIT unit = it.value();
						int key = it.key();
						if (unit.name == g_GameService.g_autojoin_leader && !autojoin_future.isRunning())
						{
							//自動組隊線程
							autojoin_future = QtConcurrent::run([key]() {
								STATICINS(GameService);
							MAP_UNIT unit = g_GameService.GetMapUnit(key);
							if (!unit.name.isEmpty() && (unit.name == g_GameService.g_autojoin_leader))
							{
								for (;;)
								{
									if (!g_GameService.IS_ONLINE_FLAG)
										return;
									if (!g_GameService.IS_AUTO_JOIN)
										return;
									if (g_GameService.IS_BATTLE_FLAG)
									{
										for (;;)
										{
											QThread::msleep(100);
											if (!g_GameService.IS_BATTLE_FLAG)
												break;
											if (!g_GameService.IS_AUTO_JOIN)
												return;
											if (!g_GameService.IS_ONLINE_FLAG)
												return;
										}
										continue;
									}

									if (*CONVERT_G_GAMEVAR(int*, 0x29D1A934) == 1)
										return;

									Util util;
									unit = g_GameService.GetMapUnit(key);
									QPoint src(*g_GameService.g_player_xpos, *g_GameService.g_player_ypos);
									QPoint rp;
									int dir = util.CalcBestFollowPointByDstPoint(NULL, src, unit.p, &rp, false, unit.dir);

									if (dir != -1)
									{
										//g_GameService.AppendLog(QString("CLAC::%1[%2] %3,%4 to %5,%6").arg(unit.name).arg(dir).arg(unit.x).arg(unit.y).arg(rp.x()).arg(rp.y()));
										if (rp != src)
										{
											g_GameService.Send(WM_SA_WALKPOS, rp.x(), rp.y());
											QThread::msleep(10);
										}
										else
										{
											QThread::msleep(100);
											g_GameService.Send(WM_SA_EO, NULL, NULL);
											g_GameService.Send(WM_SA_TURNTO, NULL, dir);
											g_GameService.Send(WM_SA_JOINLEAVE, NULL, true);
										}
									}
								}

							}

								});
							break;
						}
					}
				}
			}
		}
	}
	//離線
	else
	{
		if (!g_GameService.IS_AUTO_LOGIN) return;

		auto CHECK_WG = [](int w, int g)
		{
			STATICINS(GameService);
			int* WORLD = g_GameService.g_world_state;
			int* GAME = g_GameService.g_game_state;
			try
			{
				return ((*WORLD == w) && (*GAME == g));
			}
			catch (...)
			{
				return false;
			}
		};

		auto SET_G = [](int g)
		{
			STATICINS(GameService);
			try
			{
				*g_GameService.g_game_state = g;
			}
			catch (...)
			{
			}
		};

		//login w1 g2
		//server w2 g3
		//wait w2 g5
		//char w3 g11
		//wait_log w6 g1

		//自動登入
		if (CHECK_WG(1, 2))
		{
			g_GameService.SetUserAccountData(g_GameService.g_account, 0);
			g_GameService.SetUserAccountData(g_GameService.g_password, 1);
			SET_G(3);
		}
		else if (CHECK_WG(2, 3)) //server
		{
			//290, 165
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(290, 165));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(290, 165));

			switch (g_GameService.g_server_selected)
			{
			case 0:
			{
				//770, 250
				SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(540, 165));
				//double
				SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(540, 165));
				break;
			}
			case 1:
			{
				//770, 250
				SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(770, 165));
				//double
				SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(770, 165));
				break;
			}
			case 2:
			{
				//770, 250
				SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(540, 250));
				//double
				SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(540, 250));
				break;
			}

			case 3:
			default:
			{
				//770, 250
				SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(770, 250));
				//double
				SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(770, 250));
				break;
			}
			}
		}
		else if (CHECK_WG(2, 5))
		{

		}
		else if (CHECK_WG(3, 11))
		{
			// 790,490
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(790, 490));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(790, 490));
		}
		else if (CHECK_WG(6, 1))
		{
		}
		else if (CHECK_WG(2, 101))//連線失敗
		{
			// 510, 325
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(510, 325));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(510, 325));
		}
		else if (CHECK_WG(11, 2))//斷線
		{
			// 510, 355
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(510, 355));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(510, 355));
		}
	}
}