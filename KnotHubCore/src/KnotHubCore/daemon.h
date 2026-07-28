#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QTimer>
#include "NodeManager/nodemanager.h"
#include "NodeManager/standalonemanager.h"
#include "RecipeManager/recipemanager.h"

class Daemon : public QObject
{
    Q_OBJECT
public:
    explicit Daemon(QObject *parent = nullptr);
    ~Daemon();

    bool start();
    void stop();
    bool isRunning() const;

    PluginManager    *pluginManager()     const { return m_pluginManager; }
    StandaloneManager *standaloneManager() const { return m_standaloneManager; }
    RecipeManager    *recipeManager()     const { return m_recipeManager; }

signals:
    void started();
    void stopped();
    void logMessage(const QString &message);

private:
    PluginManager     *m_pluginManager;
    StandaloneManager *m_standaloneManager;
    RecipeManager     *m_recipeManager;
    QTimer            *m_refreshTimer;
    bool m_running;
};

#endif // DAEMON_H
