#include "settingchild_speech.h"
#include "ui_settingchild_speech.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include "ElaKeyBinder.h"

#include <QLayout>
#include <QSignalBlocker>
#include <QTimer>

SettingChild_Speech::SettingChild_Speech(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_Speech)
{
    ui->setupUi(this);

    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("语音输入设置");

    ZcJsonLib config(JsonSettingPath);
    ui->ToggleSwitch_SpeechInputEnable->setIsToggled(
        config.value("speechInput/Enable", false).toBool());
    const bool globalHotkeyEnable =
        config.value("speechInput/GlobalHotkey/Enable", false).toBool();
    ui->ToggleSwitch_GlobalHotkeyEnable->setIsToggled(globalHotkeyEnable);
    ui->keyBinder_GlobalHotkey->setBinderKeyText(
        config.value("speechInput/GlobalHotkey/BinderText").toString());
    ui->keyBinder_GlobalHotkey->setNativeVirtualBinderKey(
        static_cast<quint32>(
            config.value("speechInput/GlobalHotkey/NativeKey", 0).toInteger()));
    refreshBaiduStatus();
    refreshGlobalHotkeyBinderState();
    //ElaKeyBinder构造时会用window()创建原生弹窗，延迟重建可确保父窗口已稳定。
    QTimer::singleShot(0, this,
                       &SettingChild_Speech::initializeNativeGlobalHotkeyBinder);
}

SettingChild_Speech::~SettingChild_Speech()
{
    delete ui;
}

/*面包屑导航*/
void SettingChild_Speech::on_BreadcrumbBar_breadcrumbClicked(
    QString breadcrumb, QStringList lastBreadcrumbList)
{
    Q_UNUSED(breadcrumb)
    Q_UNUSED(lastBreadcrumbList)
    ui->stackedWidget->setCurrentIndex(0);
    refreshBaiduStatus();
}

/*进入下一级*/
void SettingChild_Speech::on_pushButton_Baidu_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("百度语音识别");

    ZcJsonLib config(JsonSettingPath);
    ui->lineEdit_BaiduApiKey->setText(
        config.value("speechInput/Baidu/ApiKey").toString());
    ui->lineEdit_BaiduSecretKey->setText(
        config.value("speechInput/Baidu/SecretKey").toString());
}

/*启用输入*/
void SettingChild_Speech::on_ToggleSwitch_SpeechInputEnable_toggled(bool checked)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/Enable", checked);
    emit speechConfigChanged();
}

void SettingChild_Speech::on_ToggleSwitch_GlobalHotkeyEnable_toggled(
    bool checked)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/GlobalHotkey/Enable", checked);
    refreshGlobalHotkeyBinderState();
    emit speechConfigChanged();
}

void SettingChild_Speech::on_lineEdit_BaiduApiKey_textChanged(const QString &arg1)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/Baidu/ApiKey", arg1);
    refreshBaiduStatus();
    emit speechConfigChanged();
}

void SettingChild_Speech::on_lineEdit_BaiduSecretKey_textChanged(
    const QString &arg1)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/Baidu/SecretKey", arg1);
    refreshBaiduStatus();
    emit speechConfigChanged();
}

/*刷新状态*/
void SettingChild_Speech::refreshBaiduStatus()
{
    ZcJsonLib config(JsonSettingPath);
    const QString apiKey =
        config.value("speechInput/Baidu/ApiKey").toString().trimmed();
    const QString secretKey =
        config.value("speechInput/Baidu/SecretKey").toString().trimmed();

    ui->label_Baidu_Status->setVisible(!apiKey.isEmpty() && !secretKey.isEmpty());
}

void SettingChild_Speech::on_keyBinder_GlobalHotkey_binderKeyTextChanged(
    QString binderKeyText)
{
    Q_UNUSED(binderKeyText)
    saveGlobalHotkeyConfig();
}

void SettingChild_Speech::on_keyBinder_GlobalHotkey_nativeVirtualBinderKeyChanged(
    quint32 binderKey)
{
    Q_UNUSED(binderKey)
    saveGlobalHotkeyConfig();
}

void SettingChild_Speech::initializeNativeGlobalHotkeyBinder()
{
    /*重建Ela原生按键绑定框*/
    auto *layout = ui->keyBinder_GlobalHotkey->parentWidget()->layout();
    if (!layout)
        return;

    //保留ui中临时控件已读取到的配置，再交给新的ElaKeyBinder显示。
    const QString binderText = ui->keyBinder_GlobalHotkey->getBinderKeyText();
    const quint32 nativeKey = ui->keyBinder_GlobalHotkey->getNativeVirtualBinderKey();
    const int index = layout->indexOf(ui->keyBinder_GlobalHotkey);
    if (index < 0)
        return;

    auto *oldBinder = ui->keyBinder_GlobalHotkey;
    auto *nativeBinder = new ElaKeyBinder(oldBinder->parentWidget());
    nativeBinder->setObjectName(oldBinder->objectName());
    nativeBinder->setSizePolicy(oldBinder->sizePolicy());
    nativeBinder->setToolTip(oldBinder->toolTip());

    layout->replaceWidget(oldBinder, nativeBinder);
    oldBinder->deleteLater();
    ui->keyBinder_GlobalHotkey = nativeBinder;

    //恢复配置时屏蔽信号，避免初始化过程误触发保存。
    {
        const QSignalBlocker blocker(ui->keyBinder_GlobalHotkey);
        ui->keyBinder_GlobalHotkey->setBinderKeyText(binderText);
        ui->keyBinder_GlobalHotkey->setNativeVirtualBinderKey(nativeKey);
    }

    connect(ui->keyBinder_GlobalHotkey, &ElaKeyBinder::binderKeyTextChanged,
            this,
            &SettingChild_Speech::on_keyBinder_GlobalHotkey_binderKeyTextChanged);
    connect(ui->keyBinder_GlobalHotkey,
            &ElaKeyBinder::nativeVirtualBinderKeyChanged, this,
            &SettingChild_Speech::
                on_keyBinder_GlobalHotkey_nativeVirtualBinderKeyChanged);
    refreshGlobalHotkeyBinderState();
}

void SettingChild_Speech::saveGlobalHotkeyConfig()
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/GlobalHotkey/BinderText",
                    ui->keyBinder_GlobalHotkey->getBinderKeyText());
    config.setValue("speechInput/GlobalHotkey/NativeKey",
                    static_cast<qint64>(
                        ui->keyBinder_GlobalHotkey->getNativeVirtualBinderKey()));
    refreshGlobalHotkeyBinderState();
    emit speechConfigChanged();
}

void SettingChild_Speech::refreshGlobalHotkeyBinderState()
{
    const bool isEnabled = ui->ToggleSwitch_GlobalHotkeyEnable->getIsToggled();
    ui->keyBinder_GlobalHotkey->setEnabled(true);

    if (ui->keyBinder_GlobalHotkey->getBinderKeyText().trimmed().isEmpty())
        ui->keyBinder_GlobalHotkey->setBinderKeyText("未绑定");

    ui->keyBinder_GlobalHotkey->setToolTip(
        isEnabled ? "点击后会弹出绑定窗口，按下按键并确认即可保存"
                  : "可以先绑定录音按键，启用全局热键后生效");
}
