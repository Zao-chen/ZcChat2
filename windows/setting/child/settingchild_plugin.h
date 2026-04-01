#ifndef SETTINGCHILD_PLUGIN_H
#define SETTINGCHILD_PLUGIN_H

#include "../../../utils/AnimePluginManager.h"

#include <QWidget>

namespace Ui
{
class SettingChild_Plugin;
}

class SettingChild_Plugin : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_Plugin(QWidget *parent = nullptr);
    ~SettingChild_Plugin();

  private slots:
    void on_pushButton_Anime_Set_clicked();
    void on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb,
                                            QStringList lastBreadcrumbList);
    void on_pushButton_ImportAnimePlugin_clicked();

  private:
    void RefreshAnimePluginList();
    void OpenPluginDetail(const QString &pluginName);
    void DeletePlugin(const QString &pluginName);

  private:
    Ui::SettingChild_Plugin *ui;
    AnimePluginManager m_pluginManager;
    QString m_currentPluginName;
};

#endif //SETTINGCHILD_PLUGIN_H
