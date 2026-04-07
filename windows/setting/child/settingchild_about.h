#ifndef SETTINGCHILD_ABOUT_H
#define SETTINGCHILD_ABOUT_H

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWidget>

namespace Ui
{
class SettingChild_About;
}

class SettingChild_About : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_About(QWidget *parent = nullptr);
    ~SettingChild_About();

  private slots:
    void on_pushButton_upDate_clicked(); //"检查更新"按钮点击

  private:
    void handleReleaseInfoReply(QNetworkReply *reply);         //处理发布信息网络请求的应答
    void handleDownloadReply(QNetworkReply *reply);            //处理exe下载的应答
    void showError(const QString &msg);                        //显示错误消息
    void refreshReleaseTable(const QJsonArray &releases);      //刷新发布日志表格
    bool tryLoadLatestDownloadUrl(const QJsonArray &releases); //从发布数据中提取最新版本的下载链接

  private:
    Ui::SettingChild_About *ui;
    QNetworkAccessManager *m_manager;  //网络请求管理器
    QString m_latestTagName;           //最新版本号标签
    QString m_latestDownloadUrl;       //最新版本exe下载链接
    QString m_localVersion;            //当前本地版本号
    bool m_releaseInfoLoaded = false;  //发布信息是否已加载（防止重复加载）
    bool m_downloadInProgress = false; //是否正在下载中（防止并发下载）
};

#endif //SETTINGCHILD_ABOUT_H