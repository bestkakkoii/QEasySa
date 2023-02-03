
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

constexpr UINT SYNC_MSG = WM_USER + 100;
QString g_caption = "";
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
		std::wstring wsmsg = g_caption.toStdWString();

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
		g_GameService.WalkPos(QPoint{ (int)wParam, (int)lParam });
		g_GameService.Send(WM_SETTEXT, NULL, NULL);
		return 1;
	}
	case SYNC_MSG:
	{
		break;
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
		m_translator.load(QString("%1/qt_zh_CN.qm").arg(QApplication::applicationDirPath()));
		qApp->installTranslator(&m_translator);
		//刷新UI
		this->retranslateUi(this);
	}

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

	g_hWnd = GetCurrentWindowHandle();

	if (!g_hWnd)
	{
		close();
		return;
	}

	g_hOldProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_hWnd, GWL_WNDPROC));
	reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWnd, GWL_WNDPROC,
		reinterpret_cast<LONG_PTR>(WndProc)));

	if (!g_hOldProc)
	{
		close();
		return;
	}

	WCHAR path[MAX_PATH] = {};
	GetModuleFileName(NULL, path, MAX_PATH);
	g_GameExeFilePath = path;


	g_GameService.initialize();

	this->listView->setModel(&g_GameService.g_log_model);
	this->listView->setUniformItemSizes(true);
	this->listView->installEventFilter(this);

	this->lineEdit_chat->installEventFilter(this);

	connect(&m_timer, &QTimer::timeout, this, &MainForm::on_timer);
	m_timer.start(100);

	const QStringList serverList = {
		tr("滿天星"), ("薰衣草"),
		tr("紫羅蘭"), tr("風信子"),
	};
	comboBox_userdata_3->addItems(serverList);
	comboBox_userdata_3->setCurrentIndex(3);

	LoadUserFromLua();


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

	if (g_GameService.IS_ONLINE_FLAG)
	{
		int fd = *g_GameService.g_net_socket;
		g_GameService.lssproto_EO_send(fd, 0);
		g_GameService.lssproto_Echo_send(fd, (char*)"????");
		g_GameService.lssproto_Echo_send(fd, (char*)"!!!!");
	}
}

//event
bool MainForm::eventFilter(QObject* obj, QEvent* event)
{

	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Delete)
		{
			STATICINS(GameService);
			g_GameService.g_log_model.clear();
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
				int fd = *g_GameService.g_net_socket;
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
	int fd = *g_GameService.g_net_socket;
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
	//int fd = *g_GameService.g_net_socket;

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


	int fd = *g_GameService.g_net_socket;

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

void MainForm::on_spinBox_BattleMagicHealValue_valueChanged(int value)
{
	STATICINS(GameService);
	g_GameService.BATTLE_AUTO_MAGIC_HEAL_VALUE = value;
}

void SendString(QString list, int n)
{
	STATICINS(GameService);
	int* focus_index = CONVERT_G_GAMEVAR(int*, 0x2B4E705C);//xgsa.exe+2B4E705C
	*focus_index = n;
	if (0 == n)
	{
		g_GameService.g_account = list;
		char* account = CONVERT_G_GAMEVAR(char*, 0x29C37AC8);//MAX is 18//xgsa.exe+29C37AC8
		int* acct_size_256 = CONVERT_G_GAMEVAR(int*, 0x29C37BD0);//xgsa.exe+29C37BD0
		int* unk_acct_base = CONVERT_G_GAMEVAR(int*, 0x29C37BDC);//xgsa.exe+29C37BDC//0 size = 464, 1 size = 10
		int* hex_unk_acct_base = CONVERT_G_GAMEVAR(int*, 0x29C37BCC);//xgsa.exe+29C37BCC//base 0x12
		_snprintf_s(account, 18, 18, "%s", list.toStdString().c_str());
		*acct_size_256 = list.size() * 256;
		*unk_acct_base = 464 + (10 * list.size());
		*hex_unk_acct_base = 0x12 | (list.size() * 0x1000000);
	}
	else
	{
		g_GameService.g_password = list;
		char* password = CONVERT_G_GAMEVAR(char*, 0x29C37990);//MAX is 18//xgsa.exe+29C37990
		int* psw_size_256 = CONVERT_G_GAMEVAR(int*, 0x29C37A98);//xgsa.exe+29C37A98)
		int* unk_psw_base = CONVERT_G_GAMEVAR(int*, 0x29C37AA4);//xgsa.exe+29C37AA4//0 size = 464, 1 size = 10
		int* hex_unk_psw_base = CONVERT_G_GAMEVAR(int*, 0x29C37A94);//xgsa.exe+29C37A94//base 0x12
		_snprintf_s(password, 18, 18, "%s", list.toStdString().c_str());
		*psw_size_256 = list.size() * 256;
		*unk_psw_base = 464 + (10 * list.size());
		*hex_unk_psw_base = 0x12 | (list.size() * 0x1000000);
	}
}

void MainForm::on_comboBox_userdata_currentIndexChanged(int index)
{
	comboBox_userdata_2->setCurrentIndex(index);
}

void MainForm::on_comboBox_userdata_editTextChanged(const QString& text)
{
	SendString(text, 0);
}

void MainForm::on_comboBox_userdata_currentTextChanged(const QString& text)
{
	SendString(text, 0);
}

void MainForm::on_comboBox_userdata_2_currentIndexChanged(int index)
{
	comboBox_userdata->setCurrentIndex(index);
}

void MainForm::on_comboBox_userdata_2_editTextChanged(const QString& text)
{
	SendString(text, 1);
}

void MainForm::on_comboBox_userdata_2_currentTextChanged(const QString& text)
{
	SendString(text, 1);
}

void MainForm::on_comboBox_userdata_3_currentIndexChanged(int index)
{
	STATICINS(GameService);
	g_GameService.g_server_selected = index;
}




void MainForm::on_pushButton_clicked()
{
	STATICINS(GameService);
	int fd = *g_GameService.g_net_socket;
	g_GameService.g_echo_timer.restart();
	g_GameService.g_enable_show_echo_ping = true;
	g_GameService.lssproto_EO_send(fd, 0);
	g_GameService.lssproto_Echo_send(fd, (char*)"????");
	g_GameService.lssproto_Echo_send(fd, (char*)"!!!!");

	//g_GameService.lssproto_JOINTEAM_send(fd);

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
	int fd = *g_GameService.g_net_socket;
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
	int fd = *g_GameService.g_net_socket;
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

void MainForm::on_timer()
{
	STATICINS(GameService);

	QScopedPointer<PC> ppc(q_check_ptr(new PC(g_GameService.GetCharData())));
	qbattle_data_t bt = g_GameService.GetBattleData();
	Map map = g_GameService.GetMap();
	std::wstring wmap = map.name.toStdWString();
	QString qmap = "\0";

	UINT ACP = ::GetACP();
	if (950 == ACP)
	{
		//地圖要轉繁體
		wchar_t wbuf[256] = {};
		int size = lstrlenW(wmap.c_str());
		LCMapStringW(LOCALE_SYSTEM_DEFAULT, LCMAP_TRADITIONAL_CHINESE, wmap.c_str(), size + 1, wbuf, sizeof(wbuf));
		qmap = QString::fromStdWString(wbuf);
	}

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


	g_caption = QString(tr("%3 LV:%4 (剩餘經驗:%5) 血:%6/%7(%8%9) 氣:%10 地圖:%11(%12)[%13,%14]")) \
		.arg(ppc->name) \
		.arg(ppc->level).arg(exp.left) \
		.arg(ppc->hp).arg(ppc->maxhp).arg(ppc->hp_percent).arg("%").arg(ppc->mp) \
		.arg(map.name.isEmpty() ? "--" : qmap).arg(map.floor) \
		.arg(*g_GameService.g_player_xpos).arg(*g_GameService.g_player_ypos);

	QString msg = QString("[%1] %2") \
		.arg(!g_GameService.IS_ONLINE_FLAG ? tr(R"(<font color="#9E9E9E">離線</font>)") : \
			g_GameService.IS_BATTLE_FLAG ? QString(tr(R"(<font color="#FF5050">戰鬥中</font>)")) : \
			tr(R"(<font color="#BEFF96">平時</font>)")) \
		.arg(QString(tr("戰鬥總場數 %1 場 歷時: %2 秒(總歷時: %3 秒) 總經驗:%4 平均:%5 預計:%6 時間:%7")).arg(bt.total_battle_count) \
			.arg(QString::number(bt.tickcount / 1000.0, 'f', 2)) \
			.arg(QString::number(bt.total_duration / 1000.0, 'f', 2)).arg(exp.total).arg(QString::number(avgExp, 'f', 2)).arg(szlefttime).arg(szonline_time));
	UpdateBar(msg);
	//QMainWindow::statusBar()->showMessage(msg, NULL);





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

		}


	}
	else
	{
		if (!g_GameService.IS_AUTO_LOGIN) return;
		int* WORLD = g_GameService.g_world_state;
		int* GAME = g_GameService.g_game_state;
		//login w1 g2
		//server w2 g3
		//wait w2 g5
		//char w3 g11
		//wait_log w6 g1
		if (*WORLD == 1 && *GAME == 2)
		{
			SendString(g_GameService.g_account, 0);
			SendString(g_GameService.g_password, 1);
			*GAME = 3;
		}
		else if (*WORLD == 2 && *GAME == 3) //server
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
			{
				//770, 250
				SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(770, 250));
				//double
				SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(770, 250));
				break;
			}
			default:
				return;
			}


		}
		else if (*WORLD == 2 && *GAME == 5)
		{

		}
		else if (*WORLD == 3 && *GAME == 11)
		{
			// 790,490
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(790, 490));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(790, 490));
		}
		else if (*WORLD == 6 && *GAME == 1)
		{
		}
		else if (*WORLD == 2 && *GAME == 101)//連線失敗
		{
			// 510, 325
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(510, 325));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(510, 325));
		}
		else if (*WORLD == 11 && *GAME == 2)//斷線
		{
			// 510, 355
			SendMessage(g_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(510, 355));
			//double
			SendMessage(g_hWnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(510, 355));
		}
	}

	setWindowTitle(g_caption);
	g_GameService.Send(WM_SETTEXT, NULL, NULL);
}