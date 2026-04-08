#include "settingchild_about.h"
#include "ui_settingchild_about.h"

#include "Version.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTimer>

//版本号格式化：移除前缀'v'，便于版本比较
static QString normalizeVersion(QString version)
{
    version = version.trimmed();
    if (version.startsWith('v', Qt::CaseInsensitive))
        version.remove(0, 1);
    return version;
}

SettingChild_About::SettingChild_About(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_About), m_manager(new QNetworkAccessManager(this)), m_localVersion(normalizeVersion(APP_VERSION))
{
    ui->setupUi(this);

    /*初始化页面*/
    ui->label_version->setText(" v" + m_localVersion);

    ui->pushButton->setUrl("https://github.com/Zao-chen/ZcChat2");
    //沿用旧版ZcChat的GitHub图标
    ui->pushButton->setCardPixmap(QPixmap(":/res/img/ico/github-mark.svg"));
    ui->pushButton->setTitle("GitHub");
    ui->pushButton->setSubTitle("项目主页、源码、Release 和文档入口");

    ui->pushButton_2->setUrl("https://github.com/Zao-chen/ZcChat2/issues/new/choose");
    //沿用旧版ZcChat的Issue图标
    ui->pushButton_2->setCardPixmap(QPixmap(":/res/img/ico/circle-dot-regular.svg"));
    ui->pushButton_2->setTitle("Issue");
    ui->pushButton_2->setSubTitle("反馈 Bug、需求和使用问题");

    ui->pushButton_3->setUrl(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ZcChat2/log.txt");
    //沿用旧版ZcChat的软件日志图标
    ui->pushButton_3->setCardPixmap(QPixmap(":/res/img/ico/file-solid.svg"));
    ui->pushButton_3->setTitle("软件日志");
    ui->pushButton_3->setSubTitle("用于排查运行问题的本地日志文件");

    ui->progressBar->setVisible(false);
    ui->tableView->setSelectionMode(QAbstractItemView::NoSelection); //禁用表格行选择

    ui->pushButton_upDate->setText(tr("检查更新"));
    //延迟到事件循环中异步加载发布信息，避免构造函数中的网络阻塞
    QTimer::singleShot(0, this, [this]()
                       {
        if (m_releaseInfoLoaded)
            return;
        m_releaseInfoLoaded = true; //标记已加载，防止重复请求
        //异步获取GitHub发布列表
        QNetworkReply *reply = m_manager->get(QNetworkRequest(
            QUrl("https://api.github.com/repos/Zao-chen/ZcChat2/releases")));
        connect(reply, &QNetworkReply::finished, this,
                [this, reply]()
                { handleReleaseInfoReply(reply); }); });
}

SettingChild_About::~SettingChild_About()
{
    delete ui;
}

/*获取新版本*/
void SettingChild_About::handleReleaseInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        ui->pushButton_upDate->setText(tr("获取新版本失败"));
        showError(reply->errorString());
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();
    //解析JSON数据
    const QJsonDocument document = QJsonDocument::fromJson(data);
    if (!document.isArray())
    {
        ui->pushButton_upDate->setText(tr("获取新版本失败"));
        showError(tr("更新数据格式错误"));
        return;
    }

    const QJsonArray releases = document.array();
    refreshReleaseTable(releases);

    if (!tryLoadLatestDownloadUrl(releases))
    {
        ui->pushButton_upDate->setText(tr("获取新版本失败"));
        return;
    }

    if (!m_latestTagName.isEmpty() && m_latestTagName != m_localVersion)
    {
        ui->pushButton_upDate->setEnabled(true);
        ui->pushButton_upDate->setText(tr("发现新版本") + m_latestTagName);
    }
    else
    {
        ui->pushButton_upDate->setText(tr("当前为最新正式版"));
    }
}

/*报错显示*/
void SettingChild_About::showError(const QString &msg)
{
    ui->label_msg->setText(msg);
}

/*刷新发布信息表格*/
void SettingChild_About::refreshReleaseTable(const QJsonArray &releases)
{
    //创建并填充表格模型
    auto *model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({tr("状态"), tr("版本"), tr("日期"), tr("标题")});

    bool matchedVersion = false; //标记是否找到当前本地版本
    for (const QJsonValue &value : releases)
    {
        if (!value.isObject())
            continue;

        const QJsonObject release = value.toObject();
        const QString tagName = normalizeVersion(release.value("tag_name").toString());
        const QString releaseName = release.value("name").toString().trimmed();
        const QString publishedAt = release.value("published_at").toString().left(10);
        const QString displayName = releaseName.isEmpty() ? tagName : releaseName;
        const QString versionText = tagName.isEmpty() ? normalizeVersion(displayName.section(' ', 0, 0)) : tagName;

        //检查该版本是否为本地当前版本
        const bool isCurrentVersion = !matchedVersion && (!versionText.isEmpty() && versionText == m_localVersion);
        if (isCurrentVersion)
        {
            matchedVersion = true;
            m_latestTagName = versionText;
        }

        QList<QStandardItem *> rowItems;
        //当前版本用■标记，其他版本用□标记
        rowItems << new QStandardItem(isCurrentVersion ? "■" : "□");
        rowItems << new QStandardItem(versionText);
        rowItems << new QStandardItem(publishedAt);
        rowItems << new QStandardItem(displayName);
        model->appendRow(rowItems);
    }

    ui->tableView->setModel(model);

    //设置列宽：第0列自适应内容、第1/2列固定、第3列伸展填充
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
}

//从发布数据中提取最新版本号和exe下载链接
bool SettingChild_About::tryLoadLatestDownloadUrl(const QJsonArray &releases)
{
    m_latestDownloadUrl.clear();
    if (releases.isEmpty())
        return false;

    //获取最新的发布版本（第一条）
    const QJsonValue firstReleaseValue = releases.first();
    if (!firstReleaseValue.isObject())
        return false;

    const QJsonObject firstRelease = firstReleaseValue.toObject();
    const QString tagName = normalizeVersion(firstRelease.value("tag_name").toString());
    const QString releaseName = firstRelease.value("name").toString().trimmed();
    m_latestTagName = tagName.isEmpty() ? normalizeVersion(releaseName.section(' ', 0, 0)) : tagName;

    //遍历资源列表找到exe文件
    const QJsonArray assets = firstRelease.value("assets").toArray();
    for (const QJsonValue &assetValue : assets)
    {
        if (!assetValue.isObject())
            continue;

        const QJsonObject asset = assetValue.toObject();
        const QString fileName = asset.value("name").toString();
        if (!fileName.endsWith(".exe", Qt::CaseInsensitive))
            continue;

        m_latestDownloadUrl = asset.value("browser_download_url").toString();
        break;
    }

    return !m_latestDownloadUrl.isEmpty();
}

void SettingChild_About::on_pushButton_upDate_clicked()
{
    if (m_downloadInProgress)
        return; //防止重复点击

    if (m_latestDownloadUrl.isEmpty())
    {
        ui->pushButton_upDate->setText(tr("下载失败"));
        return;
    }

    //启动下载过程
    m_downloadInProgress = true;
    ui->pushButton_upDate->setEnabled(false);
    ui->pushButton_upDate->setText(tr("下载中"));
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);

    //异步下载exe文件
    QNetworkReply *reply = m_manager->get(QNetworkRequest(QUrl(m_latestDownloadUrl)));

    //更新下载进度条
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 bytesReceived, qint64 bytesTotal)
            {
                if (bytesTotal > 0)
                    ui->progressBar->setValue((bytesReceived * 100) / bytesTotal);
            });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply]()
            { handleDownloadReply(reply); });
}

/*处理exe下载的应答*/
void SettingChild_About::handleDownloadReply(QNetworkReply *reply)
{
    //恢复UI到idle状态的lambda
    auto resetDownloadUi = [this]()
    {
        m_downloadInProgress = false;
        ui->progressBar->setVisible(false);
        ui->pushButton_upDate->setEnabled(true);
    };

    if (reply->error() != QNetworkReply::NoError)
    {
        resetDownloadUi();
        ui->pushButton_upDate->setText(tr("下载失败"));
        showError(reply->errorString());
        reply->deleteLater();
        return;
    }

    //计算保存路径（下载文件夹）
    const QString downloadFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString fileName = QFileInfo(QUrl(m_latestDownloadUrl).path()).fileName();
    const QString saveFilePath = QDir(downloadFolder).filePath(fileName);

    //将下载数据保存到文件
    QFile file(saveFilePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        resetDownloadUi();
        ui->pushButton_upDate->setText(tr("文件保存失败"));
        showError(tr("安装包打开失败，请前往下载文件夹手动安装"));
        reply->deleteLater();
        return;
    }

    file.write(reply->readAll());
    file.close();
    //启动下载的exe安装程序
    QProcess::startDetached(saveFilePath);
    resetDownloadUi();
    ui->pushButton_upDate->setText(tr("下载成功，正在打开"));
    reply->deleteLater();
}