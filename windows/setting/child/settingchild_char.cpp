#include "settingchild_char.h"
#include "ui_settingchild_char.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include "ElaComboBox.h"
#include "ElaContentDialog.h"
#include "ElaMessageBar.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace //用于跨平台解压zip，Linux/Mac优先用python，失败则回退到系统解压工具
{
bool runProcess(const QString &program, const QStringList &arguments,
                int timeoutMs, QString *errorMessage)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();
    if (!process.waitForFinished(timeoutMs))
    {
        process.kill();
        if (errorMessage)
            *errorMessage = QString("%1 运行超时").arg(program);
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        if (errorMessage)
        {
            QString error = process.readAllStandardError().trimmed();
            if (error.isEmpty())
                error = process.readAllStandardOutput().trimmed();
            *errorMessage = error.isEmpty() ? QString("%1 解压失败").arg(program)
                                             : error;
        }
        return false;
    }
    return true;
}

bool normalizeBackslashEntries(const QString &targetDir, QString *errorMessage)
{
    QStringList pending;
    QDirIterator iterator(targetDir,
                          QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        iterator.next();
        if (iterator.fileName().contains('\\'))
            pending.append(iterator.filePath());
    }

    if (pending.isEmpty())
        return true;

    std::sort(pending.begin(), pending.end(),
              [](const QString &a, const QString &b)
              { return a.size() > b.size(); });

    QDir rootDir(targetDir);
    QStringList failed;
    for (const QString &sourcePath : pending)
    {
        const QString relativePath = rootDir.relativeFilePath(sourcePath);
        QString normalized = relativePath;
        normalized.replace('\\', '/');
        const QString targetPath = rootDir.filePath(normalized);
        const QFileInfo sourceInfo(sourcePath);
        const QFileInfo targetInfo(targetPath);

        QDir().mkpath(targetInfo.path());

        if (sourceInfo.isDir())
        {
            if (QDir(targetPath).exists())
                continue;
            if (!QDir().rename(sourcePath, targetPath))
                failed.append(relativePath);
            continue;
        }

        if (QFile::exists(targetPath))
            QFile::remove(targetPath);
        if (!QFile::rename(sourcePath, targetPath))
            failed.append(relativePath);
    }

    if (!failed.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QString("部分文件名包含反斜杠，重命名失败: %1")
                                .arg(failed.join(", "));
        return false;
    }

    return true;
}

bool extractZipArchive(const QString &zipFilePath, const QString &targetDir,
                       QString *errorMessage)
{
    QString pythonProgram = QStandardPaths::findExecutable("python3");
    if (pythonProgram.isEmpty())
        pythonProgram = QStandardPaths::findExecutable("python");

    QStringList errors;

    if (!pythonProgram.isEmpty())
    {
        QProcess process;
        process.setProgram(pythonProgram);
        process.setArguments(QStringList()
                             << "-c"
                             << QStringLiteral(R"PY(
        import os
        import sys
        import zipfile
        import shutil

        zip_path = sys.argv[1]
        target_dir = sys.argv[2]

        os.makedirs(target_dir, exist_ok=True)
        target_abs = os.path.abspath(target_dir)

        with zipfile.ZipFile(zip_path) as archive:
            for member in archive.infolist():
                # 兼容 Windows zip 中使用的反斜杠作为分隔符
                member_name = member.filename.replace('\\', '/')
                dest_path = os.path.abspath(os.path.join(target_dir, member_name))
                if os.path.commonpath([target_abs, dest_path]) != target_abs:
                    raise RuntimeError(f"非法压缩包条目: {member.filename}")
                # 如果是目录，创建目录
                if member_name.endswith('/'):
                    os.makedirs(dest_path, exist_ok=True)
                    continue
                parent = os.path.dirname(dest_path)
                if parent:
                    os.makedirs(parent, exist_ok=True)
                with archive.open(member) as source, open(dest_path, 'wb') as target:
                    shutil.copyfileobj(source, target)
        )PY")
                             << zipFilePath << targetDir);

        process.start();
        if (process.waitForFinished(60000) && process.exitStatus() == QProcess::NormalExit &&
            process.exitCode() == 0)
        {
            QString normalizeError;
            if (!normalizeBackslashEntries(targetDir, &normalizeError))
            {
                if (errorMessage)
                    *errorMessage = normalizeError;
                return false;
            }
            return true;
        }

        QString pythonError = process.readAllStandardError().trimmed();
        if (pythonError.isEmpty())
            pythonError = process.readAllStandardOutput().trimmed();
        errors << (pythonError.isEmpty() ? "python 解压失败" : pythonError);
    }
    else
    {
        errors << "未找到可用的 Python 解释器";
    }

    const QString dittoProgram = QStandardPaths::findExecutable("ditto");
    if (!dittoProgram.isEmpty())
    {
        QString dittoError;
        if (runProcess(dittoProgram, QStringList() << "-x" << "-k" << zipFilePath
                                                   << targetDir,
                       60000, &dittoError))
        {
            QString normalizeError;
            if (!normalizeBackslashEntries(targetDir, &normalizeError))
            {
                if (errorMessage)
                    *errorMessage = normalizeError;
                return false;
            }
            return true;
        }
        errors << (dittoError.isEmpty() ? "ditto 解压失败" : dittoError);
    }

    const QString unzipProgram = QStandardPaths::findExecutable("unzip");
    if (!unzipProgram.isEmpty())
    {
        QString unzipError;
        if (runProcess(unzipProgram,
                       QStringList() << "-q" << zipFilePath << "-d" << targetDir,
                       60000, &unzipError))
        {
            QString normalizeError;
            if (!normalizeBackslashEntries(targetDir, &normalizeError))
            {
                if (errorMessage)
                    *errorMessage = normalizeError;
                return false;
            }
            return true;
        }
        errors << (unzipError.isEmpty() ? "unzip 解压失败" : unzipError);
    }

    if (errorMessage)
        *errorMessage = errors.join(" | ");
    return false;
}
} // namespace

/*初始化窗口*/
SettingChild_Char::SettingChild_Char(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_Char)
{
    ui->setupUi(this);
    //初始化动画管理器
    m_pluginManager.Reload();

    RefreshCharList();
    ui->BreadcrumbBar->appendBreadcrumb("角色设置");
    ui->BreadcrumbBar->setTextPixelSize(25);

    /*读取配置项*/
    //角色选择
    QSettings *settings =
        new QSettings(IniSettingPath, QSettings::IniFormat, this);
    QString defaultChar =
        settings->value("character/CharSelect", "未选择").toString();
    ui->comboBox_CharList->setCurrentText(defaultChar);
    LoadCurrentCharConfig();

    isAlreadyLoading = true;
}

SettingChild_Char::~SettingChild_Char()
{
    delete ui;
}

/*加载角色配置*/
void SettingChild_Char::LoadCurrentCharConfig()
{
    QString charName = ui->comboBox_CharList->currentText();
    if (charName.isEmpty() || charName == "未选择")
    {
        //如果没有选择角色，清空配置项显示
        ui->plainTextEdit_CharPrompt->clear();
        ui->spinBox_TachieSize->setValue(0);
        ClearTachieBindingRows();
        ui->comboBox_ModelSelect->clear();
        ui->ToggleSwitch_VitsEnable->setIsToggled(false);
        ui->comboBox_Vits_MASSelect->clear();
        ui->comboBox_Vits_MASSelect->setEnabled(false);
        ui->comboBox_Vits_ServerSelect->setEnabled(false);
        return;
    }

    //角色提示词
    ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
    QString charPrompt = charConfig.value("prompt").toString();
    ui->plainTextEdit_CharPrompt->setPlainText(charPrompt);
    //立绘大小
    ZcJsonLib charUserConfig(CharacterUserConfigPath + "/" + charName +
                             "/config.json");
    QString tachieSize = charUserConfig.value("tachieSize").toString();
    ui->spinBox_TachieSize->setValue(tachieSize.toInt());
    //立绘动作和动画绑定
    RefreshTachieActionList();
    //服务商和模型选择
    QString serverSelect = charUserConfig.value("serverSelect").toString();
    ui->comboBox_ServerSelect->setCurrentText(serverSelect);
    RefreshModelList();
    //模型选择
    QString modelSelect = charUserConfig.value("modelSelect").toString();
    ui->comboBox_ModelSelect->setCurrentText(modelSelect);
    //Vits语音合成选择
    bool vitsEnable = charUserConfig.value("vitsEnable").toBool();
    ui->ToggleSwitch_VitsEnable->setIsToggled(vitsEnable);
    ui->comboBox_Vits_MASSelect->setEnabled(vitsEnable);
    ui->comboBox_Vits_ServerSelect->setEnabled(vitsEnable);
    //Vits模型选择
    ZcJsonLib config(JsonSettingPath);
    QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
    QStringList vitsMasList;
    for (const QJsonValue &val : arr)
        vitsMasList.append(val.toString());
    ui->comboBox_Vits_MASSelect->clear();
    ui->comboBox_Vits_MASSelect->addItems(vitsMasList);
    //读取当前选择的Vits模型
    QString vitsMasSelect = charUserConfig.value("vitsMasSelect").toString();
    ui->comboBox_Vits_MASSelect->setCurrentText(vitsMasSelect);
}

/*删除选中角色按钮*/
void SettingChild_Char::on_pushButton_DeleteChar_clicked()
{
    QString charName = ui->comboBox_CharList->currentText();

    //检查是否选择了角色
    if (charName.isEmpty() || charName == "未选择")
    {
        ElaMessageBar::warning(ElaMessageBarType::BottomRight, "删除失败",
                               "请先选择一个角色", 3000, this);
        return;
    }

    //删除角色文件夹
    QString charPath = QDir(CharacterAssestPath).filePath(charName);
    QDir charDir(charPath);

    if (!charDir.exists())
    {
        ElaMessageBar::warning(ElaMessageBarType::BottomRight, "删除失败",
                               "角色文件夹不存在", 3000, this);
        return;
    }

    //递归删除文件夹
    if (!charDir.removeRecursively())
    {
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "删除失败",
                             "无法删除角色文件夹，请检查权限", 5000, this);
        return;
    }

    //刷新角色列表
    RefreshCharList();

    //清空配置显示
    ui->plainTextEdit_CharPrompt->clear();
    ui->spinBox_TachieSize->setValue(0);
    ui->comboBox_ModelSelect->clear();
    ui->ToggleSwitch_VitsEnable->setIsToggled(false);
    ui->comboBox_Vits_MASSelect->clear();
    ui->comboBox_Vits_ServerSelect->clear();

    ElaMessageBar::success(ElaMessageBarType::BottomRight, "删除成功",
                           QString("角色 %1 已删除").arg(charName), 4000, this);
}

/*刷新角色列表*/
void SettingChild_Char::RefreshCharList()
{
    //获取所有角色文件夹并添加到combox
    QDir charDir(CharacterAssestPath);
    QStringList charFolders =
        charDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ui->comboBox_CharList->clear();
    ui->comboBox_CharList->addItems(charFolders);
}

/*修改选中角色*/
void SettingChild_Char::on_comboBox_CharList_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    //保存到配置文件
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
void SettingChild_Char::on_plainTextEdit_CharPrompt_textChanged()
{
    if (!isAlreadyLoading)
        return;
    //保存到角色文件夹下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString charPrompt = ui->plainTextEdit_CharPrompt->toPlainText();
    ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
    charConfig.setValue("prompt", charPrompt);
}

/*修改立绘大小*/
void SettingChild_Char::on_spinBox_TachieSize_textChanged(const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    //保存到角色配置位置下的config.json
    QString tachieSize = ui->spinBox_TachieSize->text();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("tachieSize", tachieSize);
    emit requestSetTachieSize(arg1.toInt());
}

/*切换服务商选择*/
void SettingChild_Char::on_comboBox_ServerSelect_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    Q_UNUSED(arg1)
    //保存到角色配置位置下的config.json
    QString serverSelect = ui->comboBox_ServerSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("serverSelect", serverSelect);
    RefreshModelList();
    emit requestReloadAIConfig();
}

/*刷新模型列表*/
void SettingChild_Char::RefreshModelList()
{
    QString serverSelect = ui->comboBox_ServerSelect->currentText();
    ZcJsonLib config(JsonSettingPath);
    QStringList modelList;
    QJsonArray modelArray =
        config.value("llm/" + serverSelect + "/ModelList").toArray();
    for (const auto &model : modelArray)
    {
        modelList.append(model.toString());
    }
    ui->comboBox_ModelSelect->clear();
    ui->comboBox_ModelSelect->addItems(modelList);
}

/*刷新Vits模型列表*/
void SettingChild_Char::RefreshVitsModelList()
{
    ZcJsonLib config(JsonSettingPath);
    QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
    QStringList vitsMasList;
    for (const QJsonValue &val : arr)
        vitsMasList.append(val.toString());
    ui->comboBox_Vits_MASSelect->clear();
    ui->comboBox_Vits_MASSelect->addItems(vitsMasList);
}

/*切换模型选择*/
void SettingChild_Char::on_comboBox_ModelSelect_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    Q_UNUSED(arg1)
    //保存到角色配置位置下的config.json
    QString modelSelect = ui->comboBox_ModelSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("modelSelect", modelSelect);
    emit requestReloadAIConfig();
}

/*重置立绘位置*/
void SettingChild_Char::on_pushButton_ResetTachieLoc_clicked()
{
    emit requestResetTachieLoc();
}

/*切换语音合成模型选择*/
void SettingChild_Char::on_comboBox_Vits_MASSelect_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    Q_UNUSED(arg1)
    //保存到角色配置位置下的config.json
    QString vitsMasSelect = ui->comboBox_Vits_MASSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("vitsMasSelect", vitsMasSelect);
}

/*切换是否启用Vits*/
void SettingChild_Char::on_ToggleSwitch_VitsEnable_toggled(bool checked)
{
    if (!isAlreadyLoading)
        return;
    //保存到角色配置位置下的config.json
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("vitsEnable", checked);
    ui->comboBox_Vits_MASSelect->setEnabled(checked);
    ui->comboBox_Vits_ServerSelect->setEnabled(checked);
}

/*导入角色压缩包*/
void SettingChild_Char::on_pushButton_InputChar_clicked()
{
    //选择角色压缩包
    QString zipFilePath = QFileDialog::getOpenFileName(
        this, "选择角色压缩包",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Zip Files (*.zip)");

    if (zipFilePath.isEmpty())
        return;

    QFileInfo zipInfo(zipFilePath);
    QString charName = zipInfo.completeBaseName();
    QString targetCharPath = QDir(CharacterAssestPath).filePath(charName);

    //如果目标角色文件夹已存在，询问用户是否覆盖
    if (QDir(targetCharPath).exists())
    {
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

        if (overwriteDialog.exec() != QDialog::Accepted)
            return;
    }
    QDir().mkpath(CharacterAssestPath);

#ifdef Q_OS_WIN
    QProcess process;
    QString command =
        QString("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
            .arg(zipFilePath, CharacterAssestPath);
    process.setProgram("powershell");
    process.setArguments(QStringList() << "-NoProfile" << "-Command" << command);
    process.start();
    if (!process.waitForFinished(60000))
    {
        process.kill();
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "导入失败",
                             "解压过程超时", 5000, this);
        return;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        QString error = process.readAllStandardError();
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "导入失败",
                             QString("解压失败: %1").arg(error), 5000,
                             this);
        return;
    }
#else
    QString errorMessage;
    if (!extractZipArchive(zipFilePath, CharacterAssestPath, &errorMessage))
    {
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "导入失败",
                             QString("解压失败: %1").arg(errorMessage), 5000,
                             this);
        return;
    }
#endif

    RefreshCharList();
    if (ui->comboBox_CharList->findText(charName) >= 0)
        ui->comboBox_CharList->setCurrentText(charName);

    ElaMessageBar::success(ElaMessageBarType::TopRight, "导入成功",
                           QString("角色 %1 已导入").arg(charName), 4000, this);
}

/*导出选中的角色*/
void SettingChild_Char::on_pushButton_OutputChar_clicked()
{
    //获取选中的角色名称
    QString charName = ui->comboBox_CharList->currentText();
    if (charName.isEmpty() || charName == "未选择")
    {
        ElaMessageBar::warning(ElaMessageBarType::BottomRight, "导出失败",
                               "请先选择一个角色", 3000, this);
        return;
    }

    //检查角色文件夹是否存在
    QString charPath = QDir(CharacterAssestPath).filePath(charName);
    if (!QDir(charPath).exists())
    {
        ElaMessageBar::warning(ElaMessageBarType::BottomRight, "导出失败",
                               "角色文件夹不存在", 3000, this);
        return;
    }

    //询问用户导出位置
    QString exportDir = QFileDialog::getExistingDirectory(
        this, "选择导出位置",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (exportDir.isEmpty()) //用户取消了
        return;

    //构建目标压缩包路径
    QString zipFileName = charName + ".zip";
    QString zipFilePath = QDir(exportDir).filePath(zipFileName);

    //检查文件是否已存在
    if (QFile::exists(zipFilePath))
    {
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

        if (overwriteDialog.exec() != QDialog::Accepted)
            return;
        QFile::remove(zipFilePath);
    }

    //使用系统命令压缩（跨平台支持）
    QProcess process;

#ifdef Q_OS_WIN
    //Windows: 使用PowerShell
    QString command =
        QString("Compress-Archive -Path '%1' -DestinationPath '%2' -Force")
            .arg(charPath, zipFilePath);
    process.setProgram("powershell");
    process.setArguments(QStringList() << "-NoProfile" << "-Command" << command);
#else
    //Linux/Mac: 使用zip命令
    //需要先cd到Character/Assets目录，然后压缩相对路径
    QString basePath = QDir(CharacterAssestPath).absolutePath();

    process.setWorkingDirectory(basePath);
    process.setProgram("zip");
    process.setArguments(QStringList() << "-r" << zipFilePath << charName);
#endif

    process.start();
    if (!process.waitForFinished(60000)) //等待最多60秒
    {
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "导出失败",
                             "压缩过程超时", 5000, this);
        process.kill();
        return;
    }

    if (process.exitCode() != 0)
    {
        QString error = process.readAllStandardError();
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "导出失败",
                             QString("压缩失败: %1").arg(error), 5000, this);
        return;
    }

    ElaMessageBar::success(
        ElaMessageBarType::BottomRight, "导出成功",
        QString("角色 %1 已成功导出到:\n%2").arg(charName, zipFilePath), 4000,
        this);
}

/*刷新立绘动画列表*/
void SettingChild_Char::RefreshTachieAnimationList()
{
    //兼容旧调用：现在动画绑定随动作行一起刷新
    RefreshTachieActionList();
}

/*清理动态创建的动作绑定行*/
void SettingChild_Char::ClearTachieBindingRows()
{
    for (QWidget *row : m_tachieBindingRows)
    {
        if (row)
            row->deleteLater();
    }
    m_tachieBindingRows.clear();
}

/*刷新立绘动作列表*/
void SettingChild_Char::RefreshTachieActionList()
{
    QString charName = ui->comboBox_CharList->currentText();
    ClearTachieBindingRows();
    if (charName.isEmpty() || charName == "未选择")
        return;

    //扫描角色立绘文件夹，获取所有动作名称
    QDir tachieDir(CharacterAssestPath + "/" + charName + "/Tachie");
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg";
    QStringList fileNames = tachieDir.entryList(nameFilters, QDir::Files);

    QStringList actionList;
    for (const QString &fileName : fileNames)
    {
        QString actionName = QFileInfo(fileName).completeBaseName();
        if (!actionName.isEmpty() && !actionList.contains(actionName))
            actionList.append(actionName);
    }
    if (actionList.isEmpty())
        actionList.append("default");

    //动画候选列表：统一使用唯一键（插件名_动画名）
    const QStringList animationUniqueKeys = m_pluginManager.AnimationUniqueKeys();

    ZcJsonLib charUserConfig(CharacterAssestPath + "/" + charName +
                             "/config.json");
    QJsonObject animationMap =
        charUserConfig.value("tachieAnimations", QJsonObject()).toObject();

    //创建横排行：左侧动作名称，右侧绑定下拉
    QVBoxLayout *layout = ui->verticalLayout_TachieBindingList;
    const int insertIndex = qMax(0, layout->count() - 1); //在底部弹簧前插入

    for (const QString &actionName : actionList)
    {
        ElaScrollPageArea *row =
            new ElaScrollPageArea(ui->widget_TachieBindingContainer);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(15, 0, 15, 0);

        ElaText *label = new ElaText(row);
        QFont font = label->font();
        font.setPointSize(12);
        label->setFont(font);
        label->setText(actionName);

        ElaComboBox *combo = new ElaComboBox(row);
        combo->setStyleSheet(ui->comboBox_CharList->styleSheet());
        combo->addItem("无动画", "");
        for (const QString &uniqueKey : animationUniqueKeys)
        {
            combo->addItem(uniqueKey, uniqueKey);
        }

        QString boundAnimation = animationMap.value(actionName).toString();

        combo->blockSignals(true);
        const int keyIndex = combo->findData(boundAnimation);
        if (keyIndex >= 0)
            combo->setCurrentIndex(keyIndex);
        else
            combo->setCurrentIndex(0);
        combo->blockSignals(false);

        QObject::connect(combo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this,
                         [this, actionName, combo](int index)
                         {
                             Q_UNUSED(index)
                             if (!isAlreadyLoading)
                                 return;

                             QString currentCharName =
                                 ui->comboBox_CharList->currentText();
                             ZcJsonLib charConfig(CharacterAssestPath + "/" +
                                                  currentCharName +
                                                  "/config.json");
                             QJsonObject map =
                                 charConfig.value("tachieAnimations", QJsonObject())
                                     .toObject();
                             const QString selectedUniqueKey =
                                 combo->currentData().toString();
                             if (selectedUniqueKey.isEmpty())
                                 map.remove(actionName);
                             else
                                 map.insert(actionName, selectedUniqueKey);

                             charConfig.setValue("tachieAnimations", map);
                         });

        rowLayout->addWidget(label, 3);
        rowLayout->addWidget(combo, 2);
        layout->insertWidget(insertIndex + m_tachieBindingRows.size(), row);
        m_tachieBindingRows.append(row);
    }
}

//进入立绘设置二级菜单
void SettingChild_Char::on_pushButton_Tachie_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("立绘设置");
}

//面包屑导航：返回上级
void SettingChild_Char::on_BreadcrumbBar_breadcrumbClicked(
    QString breadcrumb, QStringList lastBreadcrumbList)
{
    Q_UNUSED(lastBreadcrumbList)
    if (breadcrumb == "角色设置")
    {
        ui->stackedWidget->setCurrentIndex(0);
    }
}
