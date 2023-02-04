#pragma once
#include "gameservice.h"
#include <QMainWindow>
#include "ui_mainform.h"

class MainForm : public QMainWindow, public Ui::MainFormClass
{
	Q_OBJECT

public:
	MainForm(QWidget* parent = nullptr);
	~MainForm();

protected:
	virtual void closeEvent(QCloseEvent* e) override;
	virtual bool eventFilter(QObject* obj, QEvent* event) override;

private:
	QTimer m_timer;
	QTimer m_battletimer;
	QTranslator m_translator;
	QLabel* m_permanent = nullptr;
	QLabel* m_childstatus = nullptr;

	//記錄對話可以上下移動
	int m_lineEditListIndex = 0;
	QList<QString> m_lineEditList;

	QFuture<void> autojoin_future;

	const QString m_userFileName = QCoreApplication::applicationDirPath() + "/user.lua";
	void LoadUserFromLua();

	void UpdateStaticBar(const QString& data);
	void UpdateBar(const QString& data);


private slots:
	void on_checkBox_EnableFastBattle_stateChanged(int state);
	void on_checkBox_EnableAutoCombat_stateChanged(int state);
	void on_checkBox_EnableAutoBattle_stateChanged(int state);
	void on_checkBox_EnableAutoEatMeat_stateChanged(int state);
	void on_checkBox_EnableAutoLogin_stateChanged(int state);
	void on_checkBox_EnableBattleMagicHeal_stateChanged(int state);
	void on_checkBox_EnableDebugMode_stateChanged(int state);
	void on_checkBox_EnableAutoJoin_stateChanged(int state);
	void on_checkBox_EnableAutoReCombat_stateChanged(int state);

	void on_spinBox_BattleMagicHealValue_valueChanged(int value);
	void on_spinBox_speed_valueChanged(int value);

	void on_comboBox_userdata_currentIndexChanged(int index);
	void on_comboBox_userdata_editTextChanged(const QString& text);
	void on_comboBox_userdata_currentTextChanged(const QString& text);

	void on_comboBox_userdata_2_currentIndexChanged(int index);
	void on_comboBox_userdata_2_editTextChanged(const QString& text);
	void on_comboBox_userdata_2_currentTextChanged(const QString& text);

	void on_comboBox_userdata_3_currentIndexChanged(int index);

	void on_comboBox_autojoin_currentTextChanged(const QString& text);
	void on_comboBox_autojoin_editTextChanged(const QString& text);
	void on_comboBox_autojoin_clicked();

	void on_pushButton_clicked();
	void on_pushButton_logout_clicked();
	void on_pushButton_logback_clicked();
	void on_pushButton_sellall_clicked();
	void on_pushButton_joinleave_clicked();
	void on_pushButton_userdata_clicked();
	void on_pushButton_userdatarefresh_clicked();


	void OnTimer();
	void OnBattleTimer();
	void OnUpdateData();
};
