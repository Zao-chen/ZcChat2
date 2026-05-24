#include "settingchild_plugin.h"
#include "ui_settingchild_plugin.h"

#include "../../../GlobalConstants.h"

#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringListModel>
#include <QVBoxLayout>

SettingChild_Plugin::SettingChild_Plugin(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_Plugin)
{
    ui->setupUi(this);
    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("插件设置");
}

SettingChild_Plugin::~SettingChild_Plugin()
{
    delete ui;
}

/*进入第二页*/
void SettingChild_Plugin::on_pushButton_Anime_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("动画插件");
    RefreshAnimePluginList();
}

/*面包屑返回*/
void SettingChild_Plugin::on_BreadcrumbBar_breadcrumbClicked(
    QString breadcrumb, QStringList lastBreadcrumbList)
{
    Q_UNUSED(lastBreadcrumbList)
    if (breadcrumb == "插件设置")
    {
        ui->stackedWidget->setCurrentIndex(0);
        return;
    }
    if (breadcrumb == "动画插件")
    {
        ui->stackedWidget->setCurrentIndex(1);
        return;
    }
    ui->stackedWidget->setCurrentIndex(2);
}

/*导入动画插件*/
void SettingChild_Plugin::on_pushButton_ImportAnimePlugin_clicked()
{
    const QString pluginJsonPath = QFileDialog::getOpenFileName(
        this, "选择动画插件",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Json Files (*.json)");
    if (pluginJsonPath.isEmpty())
        return;

    AnimePluginDefinition plugin;
    QString parseError;
    if (!LoadAnimePluginFromFile(pluginJsonPath, plugin, parseError))
    {
        ElaMessageBar::error(ElaMessageBarType::TopRight, "导入失败",
                             QString("插件格式无效: %1").arg(parseError), 5000,
                             this);
        return;
    }

    m_pluginManager.Reload();
    const QList<AnimePluginDefinition> plugins = m_pluginManager.Plugins();
    for (const AnimePluginDefinition &existPlugin : plugins)
    {
        //导入阶段直接拒绝重复插件名称，名称也是插件唯一标识
        if (existPlugin.name == plugin.name)
        {
            ElaMessageBar::warning(ElaMessageBarType::TopRight, "导入失败",
                                   QString("插件名称重复: %1").arg(plugin.name),
                                   5000, this);
            return;
        }
    }

    //统一落盘为插件名.json，避免同插件多文件名造成混淆
    QString fileSafePluginName = plugin.name;
    fileSafePluginName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    const QString targetPath =
        QDir(AnimePluginPath).filePath(fileSafePluginName + ".json");
    if (QFile::exists(targetPath))
    {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, "导入失败",
                               QString("目标文件已存在: %1").arg(targetPath),
                               5000, this);
        return;
    }

    if (!QFile::copy(pluginJsonPath, targetPath))
    {
        ElaMessageBar::error(ElaMessageBarType::TopRight, "导入失败",
                             "复制插件文件失败", 5000, this);
        return;
    }

    RefreshAnimePluginList();
    ElaMessageBar::success(ElaMessageBarType::TopRight, "导入成功",
                           QString("已导入插件: %1").arg(plugin.name), 4000,
                           this);
}

/*刷新动画插件列表*/
void SettingChild_Plugin::RefreshAnimePluginList()
{
    //先清空旧卡片，避免重复渲染
    QVBoxLayout *cardsLayout = ui->verticalLayout_PluginCards;
    while (cardsLayout->count() > 0)
    {
        QLayoutItem *item = cardsLayout->takeAt(0);
        if (!item)
            continue;
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    QStringList errorList;
    if (!m_pluginManager.Reload())
    {
        const QStringList errors = m_pluginManager.LastErrors();
        for (const QString &error : errors)
            errorList.append(QString("[错误] %1").arg(error));
    }

    const QList<AnimePluginDefinition> plugins = m_pluginManager.Plugins();
    //创建新卡片
    for (const AnimePluginDefinition &plugin : plugins)
    {
        ElaScrollPageArea *pluginCard =
            new ElaScrollPageArea(ui->scrollAreaWidgetContents_Plugins);
        QHBoxLayout *cardLayout = new QHBoxLayout(pluginCard);
        cardLayout->setContentsMargins(15, 8, 15, 8);

        QWidget *textArea = new QWidget(pluginCard);
        QVBoxLayout *textLayout = new QVBoxLayout(textArea);
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(2);

        ElaText *nameText = new ElaText(textArea);
        QFont nameFont = nameText->font();
        nameFont.setPointSize(12);
        nameText->setFont(nameFont);
        nameText->setText(plugin.name);

        ElaText *authorText = new ElaText(textArea);
        QFont authorFont = authorText->font();
        authorFont.setPointSize(10);
        authorText->setFont(authorFont);
        authorText->setText(
            QString("版本:%1  作者:%2").arg(plugin.version, plugin.author));

        textLayout->addWidget(nameText);
        textLayout->addWidget(authorText);

        ElaPushButton *openDetailButton =
            new ElaPushButton("查看动画", pluginCard);

        ElaPushButton *deletePluginButton =
            new ElaPushButton("删除插件", pluginCard);

        connect(openDetailButton, &QPushButton::clicked, this,
                [this, plugin]()
                { OpenPluginDetail(plugin.name); });
        connect(deletePluginButton, &QPushButton::clicked, this,
                [this, plugin]()
                { DeletePlugin(plugin.name); });

        QWidget *buttonArea = new QWidget(pluginCard);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonArea);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(8);
        //两个按钮平分右侧区域
        buttonLayout->addWidget(deletePluginButton, 1);
        buttonLayout->addWidget(openDetailButton, 1);

        //整行左右各占一半：左文本、右按钮区
        cardLayout->addWidget(textArea, 1);
        cardLayout->addWidget(buttonArea, 1);
        cardsLayout->addWidget(pluginCard);
    }

    if (plugins.isEmpty())
    {
        QLabel *emptyLabel =
            new QLabel("暂无动画插件，请先导入json插件文件", ui->scrollAreaWidgetContents_Plugins);
        cardsLayout->addWidget(emptyLabel);
    }

    for (const QString &error : errorList)
    {
        QLabel *errorLabel = new QLabel(error, ui->scrollAreaWidgetContents_Plugins);
        errorLabel->setWordWrap(true);
        cardsLayout->addWidget(errorLabel);
    }

    //底部弹性空间，确保卡片从上向下紧凑排列
    cardsLayout->addStretch();
}

/* 打开插件详情 */
void SettingChild_Plugin::OpenPluginDetail(const QString &pluginName)
{
    const QList<AnimePluginDefinition> plugins = m_pluginManager.Plugins();
    for (const AnimePluginDefinition &plugin : plugins)
    {
        if (plugin.name != pluginName)
            continue;

        m_currentPluginName = pluginName;
        ui->label_CurrentPlugin->setText(QString("动画列表 - %1").arg(plugin.name));

        QStringList animationList;
        for (const AnimePluginAnimation &animation : plugin.animations)
            animationList.append(animation.BuildUniqueKey(plugin.name));
        if (animationList.isEmpty())
            animationList.append("该插件没有可用动画");

        QStringListModel *model =
            new QStringListModel(animationList, ui->listView_PluginAnimations);
        ui->listView_PluginAnimations->setModel(model);

        ui->stackedWidget->setCurrentIndex(2);
        ui->BreadcrumbBar->appendBreadcrumb(plugin.name);
        return;
    }

    ElaMessageBar::warning(ElaMessageBarType::TopRight, "提示",
                           "未找到该插件，可能已被删除", 4000, this);
}

/*删除插件*/
void SettingChild_Plugin::DeletePlugin(const QString &pluginName)
{
    const QList<AnimePluginDefinition> plugins = m_pluginManager.Plugins();
    for (const AnimePluginDefinition &plugin : plugins)
    {
        if (plugin.name != pluginName)
            continue;

        if (!QFile::remove(plugin.filePath))
        {
            ElaMessageBar::error(ElaMessageBarType::TopRight, "删除失败",
                                 "无法删除插件文件", 5000, this);
            return;
        }

        if (m_currentPluginName == pluginName)
        {
            m_currentPluginName.clear();
            ui->stackedWidget->setCurrentIndex(1);
        }

        RefreshAnimePluginList();
        ElaMessageBar::success(ElaMessageBarType::TopRight, "删除成功",
                               QString("已删除插件: %1").arg(plugin.name),
                               3500, this);
        return;
    }

    ElaMessageBar::warning(ElaMessageBarType::TopRight, "提示",
                           "未找到该插件，可能已被删除", 4000, this);
}
