#include "settingchild_char.h"
#include "ui_settingchild_char.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include "ElaContentDialog.h"
#include "ElaMessageBar.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QLabel>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <qdebug.h>
#include <qlogging.h>

SettingChild_Char::SettingChild_Char(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_Char) {
  ui->setupUi(this);
  RefreshCharList();
  ui->BreadcrumbBar->appendBreadcrumb("角色设置");
  ui->BreadcrumbBar->setTextPixelSize(25);

  /*读取配置项*/
  // 角色选择
  QSettings *settings =
      new QSettings(IniSettingPath, QSettings::IniFormat, this);
  QString defaultChar =
      settings->value("character/CharSelect", "未选择").toString();
  ui->comboBox_CharList->setCurrentText(defaultChar);
  LoadCurrentCharConfig();

  isAlreadyLoading = true;
}

SettingChild_Char::~SettingChild_Char() { delete ui; }

/*加载角色配置*/
void SettingChild_Char::LoadCurrentCharConfig() {
  QString charName = ui->comboBox_CharList->currentText();
  if (charName.isEmpty() ||
      charName == "未选择") { // 如果没有选择角色，清空配置项显示
    ui->plainTextEdit_CharPrompt->clear();
    ui->spinBox_TachieSize->setValue(0);
    ui->comboBox_ModelSelect->clear();
    ui->ToggleSwitch_VitsEnable->setIsToggled(false);
    ui->comboBox_Vits_MASSelect->clear();
    ui->comboBox_Vits_MASSelect->setEnabled(false);
    ui->comboBox_Vits_ServerSelect->setEnabled(false);
    return;
  }

  // 角色提示词
  ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
  QString charPrompt = charConfig.value("prompt").toString();
  ui->plainTextEdit_CharPrompt->setPlainText(charPrompt);
  // 立绘大小
  ZcJsonLib charUserConfig(CharacterUserConfigPath + "/" + charName +
                           "/config.json");
  QString tachieSize = charUserConfig.value("tachieSize").toString();
  ui->spinBox_TachieSize->setValue(tachieSize.toInt());
  // 服务商和模型选择
  QString serverSelect = charUserConfig.value("serverSelect").toString();
  ui->comboBox_ServerSelect->setCurrentText(serverSelect);
  RefreshModelList();
  // 模型选择
  QString modelSelect = charUserConfig.value("modelSelect").toString();
  ui->comboBox_ModelSelect->setCurrentText(modelSelect);
  // Vits语音合成选择
  bool vitsEnable = charUserConfig.value("vitsEnable").toBool();
  ui->ToggleSwitch_VitsEnable->setIsToggled(vitsEnable);
  ui->comboBox_Vits_MASSelect->setEnabled(vitsEnable);
  ui->comboBox_Vits_ServerSelect->setEnabled(vitsEnable);
  // Vits模型选择
  ZcJsonLib config(JsonSettingPath);
  QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
  QStringList vitsMasList;
  for (const QJsonValue &val : arr)
    vitsMasList.append(val.toString());
  ui->comboBox_Vits_MASSelect->clear();
  ui->comboBox_Vits_MASSelect->addItems(vitsMasList);
  // 读取当前选择的Vits模型
  QString vitsMasSelect = charUserConfig.value("vitsMasSelect").toString();
  ui->comboBox_Vits_MASSelect->setCurrentText(vitsMasSelect);
}

/*刷新角色列表按钮*/
void SettingChild_Char::on_pushButton_RefreshCharList_clicked() {
  RefreshCharList();
}

/*设置立绘大小*/
void SettingChild_Char::RefreshCharList() {
  // 获取所有角色文件夹并添加到combox
  QDir charDir(CharacterAssestPath);
  QStringList charFolders =
      charDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  ui->comboBox_CharList->clear();
  ui->comboBox_CharList->addItems(charFolders);
}

/*修改选中角色*/
void SettingChild_Char::on_comboBox_CharList_currentTextChanged(
    const QString &arg1) {
  if (!isAlreadyLoading)
    return;
  // 保存到配置文件
  QSettings *settings =
      new QSettings(IniSettingPath, QSettings::IniFormat, this);
  settings->setValue("character/CharSelect", arg1);

  isAlreadyLoading = false;
  LoadCurrentCharConfig();
  isAlreadyLoading = true;

  emit requestReloadCharSelect("default");
  emit requestReloadAIConfig();
}

/*修改角色提示词*/
void SettingChild_Char::on_plainTextEdit_CharPrompt_textChanged() {
  if (!isAlreadyLoading)
    return;
  // 保存到角色文件夹下的config.json
  QString charName = ui->comboBox_CharList->currentText();
  QString charPrompt = ui->plainTextEdit_CharPrompt->toPlainText();
  ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
  charConfig.setValue("prompt", charPrompt);
}

/*修改立绘大小*/
void SettingChild_Char::on_spinBox_TachieSize_textChanged(const QString &arg1) {
  if (!isAlreadyLoading)
    return;
  // 保存到角色配置位置下的config.json
  QString charName = ui->comboBox_CharList->currentText();
  QString tachieSize = ui->spinBox_TachieSize->text();
  ZcJsonLib charConfig(ReadCharacterUserConfigPath());
  charConfig.setValue("tachieSize", tachieSize);
  emit requestSetTachieSize(arg1.toInt());
}

/*切换服务商选择*/
void SettingChild_Char::on_comboBox_ServerSelect_currentTextChanged(
    const QString &arg1) {
  if (!isAlreadyLoading)
    return;
  // 保存到角色配置位置下的config.json
  QString charName = ui->comboBox_CharList->currentText();
  QString serverSelect = ui->comboBox_ServerSelect->currentText();
  ZcJsonLib charConfig(ReadCharacterUserConfigPath());
  charConfig.setValue("serverSelect", serverSelect);
  RefreshModelList();
  emit requestReloadAIConfig();
}

/*刷新模型列表*/
void SettingChild_Char::RefreshModelList() {
  QString serverSelect = ui->comboBox_ServerSelect->currentText();
  ZcJsonLib config(JsonSettingPath);
  QStringList modelList;
  QJsonArray modelArray =
      config.value("llm/" + serverSelect + "/ModelList").toArray();
  for (const auto &model : modelArray) {
    modelList.append(model.toString());
  }
  ui->comboBox_ModelSelect->clear();
  ui->comboBox_ModelSelect->addItems(modelList);
}

/*切换模型选择*/
void SettingChild_Char::on_comboBox_ModelSelect_currentTextChanged(
    const QString &arg1) {
  if (!isAlreadyLoading)
    return;
  // 保存到角色配置位置下的config.json
  QString charName = ui->comboBox_CharList->currentText();
  QString modelSelect = ui->comboBox_ModelSelect->currentText();
  ZcJsonLib charConfig(ReadCharacterUserConfigPath());
  charConfig.setValue("modelSelect", modelSelect);
  emit requestReloadAIConfig();
}

/*重置立绘位置*/
void SettingChild_Char::on_pushButton_ResetTachieLoc_clicked() {
  emit requestResetTachieLoc();
}

/*切换语音合成模型选择*/
void SettingChild_Char::on_comboBox_Vits_MASSelect_currentTextChanged(
    const QString &arg1) {
  if (!isAlreadyLoading)
    return;
  // 保存到角色配置位置下的config.json
  QString charName = ui->comboBox_CharList->currentText();
  QString vitsMasSelect = ui->comboBox_Vits_MASSelect->currentText();
  ZcJsonLib charConfig(ReadCharacterUserConfigPath());
  charConfig.setValue("vitsMasSelect", vitsMasSelect);
}

/*切换是否启用Vits*/
void SettingChild_Char::on_ToggleSwitch_VitsEnable_toggled(bool checked) {
  if (!isAlreadyLoading)
    return;
  // 保存到角色配置位置下的config.json
  QString charName = ui->comboBox_CharList->currentText();
  ZcJsonLib charConfig(ReadCharacterUserConfigPath());
  charConfig.setValue("vitsEnable", checked);
  ui->comboBox_Vits_MASSelect->setEnabled(checked);
  ui->comboBox_Vits_ServerSelect->setEnabled(checked);
}

/*导入角色压缩包*/
void SettingChild_Char::on_pushButton_InputChar_clicked() {
  // 选择角色压缩包
  QString zipFilePath = QFileDialog::getOpenFileName(
      this, "选择角色压缩包",
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
      "Zip Files (*.zip)");

  if (zipFilePath.isEmpty())
    return;

  QFileInfo zipInfo(zipFilePath);
  QString charName = zipInfo.completeBaseName();
  QString targetCharPath = QDir(CharacterAssestPath).filePath(charName);

  // 如果目标角色文件夹已存在，询问用户是否覆盖
  if (QDir(targetCharPath).exists()) {
    ElaContentDialog overwriteDialog(this);
    overwriteDialog.setLeftButtonText("取消");
    overwriteDialog.setRightButtonText("覆盖");

    QLabel *messageLabel = new QLabel(
        QString("角色 %1 已存在，是否覆盖？").arg(charName), &overwriteDialog);
    messageLabel->setWordWrap(true);
    overwriteDialog.setCentralWidget(messageLabel);

    QObject::connect(&overwriteDialog, &ElaContentDialog::leftButtonClicked,
                     &overwriteDialog, &QDialog::reject);
    QObject::connect(&overwriteDialog, &ElaContentDialog::rightButtonClicked,
                     &overwriteDialog, &QDialog::accept);

    if (overwriteDialog.exec() != QDialog::Accepted) {
      return;
    }
  }
  QProcess process;
// 使用系统命令解压（跨平台支持）
#ifdef Q_OS_WIN
  QString command =
      QString("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
          .arg(zipFilePath, CharacterAssestPath);
  process.setProgram("powershell");
  process.setArguments(QStringList() << "-NoProfile" << "-Command" << command);
#else
  process.setProgram("unzip");
  process.setArguments(QStringList()
                       << "-o" << zipFilePath << "-d" << CharacterAssestPath);
#endif

  process.start();
  if (!process.waitForFinished(60000)) {
    process.kill();
    ElaMessageBar::error(ElaMessageBarType::TopRight, "导入失败",
                         "解压过程超时", 5000, this);
    return;
  }

  if (process.exitCode() != 0) {
    QString error = process.readAllStandardError();
    ElaMessageBar::error(ElaMessageBarType::TopRight, "导入失败",
                         QString("解压失败: %1").arg(error), 5000, this);
    return;
  }

  RefreshCharList();
  if (ui->comboBox_CharList->findText(charName) >= 0) {
    ui->comboBox_CharList->setCurrentText(charName);
  }

  ElaMessageBar::success(ElaMessageBarType::TopRight, "导入成功",
                         QString("角色 %1 已导入").arg(charName), 4000, this);
}

/*导出选中的角色*/
void SettingChild_Char::on_pushButton_OutputChar_clicked() {
  // 获取选中的角色名称
  QString charName = ui->comboBox_CharList->currentText();
  if (charName.isEmpty() || charName == "未选择") {
    ElaMessageBar::warning(ElaMessageBarType::TopRight, "导出失败",
                           "请先选择一个角色", 3000, this);
    return;
  }

  // 检查角色文件夹是否存在
  QString charPath = QDir(CharacterAssestPath).filePath(charName);
  if (!QDir(charPath).exists()) {
    ElaMessageBar::warning(ElaMessageBarType::TopRight, "导出失败",
                           "角色文件夹不存在", 3000, this);
    return;
  }

  // 询问用户导出位置
  QString exportDir = QFileDialog::getExistingDirectory(
      this, "选择导出位置",
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (exportDir.isEmpty()) {
    return; // 用户取消了
  }

  // 构建目标压缩包路径
  QString zipFileName = charName + ".zip";
  QString zipFilePath = QDir(exportDir).filePath(zipFileName);

  // 检查文件是否已存在
  if (QFile::exists(zipFilePath)) {
    ElaContentDialog overwriteDialog(this);
    overwriteDialog.setLeftButtonText("取消");
    overwriteDialog.setRightButtonText("覆盖");

    QLabel *messageLabel =
        new QLabel(QString("文件 %1 已存在，是否覆盖？").arg(zipFileName),
                   &overwriteDialog);
    messageLabel->setWordWrap(true);
    overwriteDialog.setCentralWidget(messageLabel);

    QObject::connect(&overwriteDialog, &ElaContentDialog::leftButtonClicked,
                     &overwriteDialog, &QDialog::reject);
    QObject::connect(&overwriteDialog, &ElaContentDialog::rightButtonClicked,
                     &overwriteDialog, &QDialog::accept);

    if (overwriteDialog.exec() != QDialog::Accepted) {
      return;
    }
    QFile::remove(zipFilePath);
  }

  // 使用系统命令压缩（跨平台支持）
  QProcess process;

#ifdef Q_OS_WIN
  // Windows: 使用PowerShell
  QString command =
      QString("Compress-Archive -Path '%1' -DestinationPath '%2' -Force")
          .arg(charPath, zipFilePath);
  process.setProgram("powershell");
  process.setArguments(QStringList() << "-NoProfile" << "-Command" << command);
#else
  // Linux/Mac: 使用zip命令
  // 需要先cd到Character/Assets目录，然后压缩相对路径
  QString basePath = QDir(CharacterAssestPath).absolutePath();

  process.setWorkingDirectory(basePath);
  process.setProgram("zip");
  process.setArguments(QStringList() << "-r" << zipFilePath << charName);
#endif

  process.start();
  if (!process.waitForFinished(60000)) // 等待最多60秒
  {
    ElaMessageBar::error(ElaMessageBarType::TopRight, "导出失败",
                         "压缩过程超时", 5000, this);
    process.kill();
    return;
  }

  if (process.exitCode() != 0) {
    QString error = process.readAllStandardError();
    ElaMessageBar::error(ElaMessageBarType::TopRight, "导出失败",
                         QString("压缩失败: %1").arg(error), 5000, this);
    return;
  }

  ElaMessageBar::success(
      ElaMessageBarType::TopRight, "导出成功",
      QString("角色 %1 已成功导出到:\n%2").arg(charName, zipFilePath), 4000,
      this);
}