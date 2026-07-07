#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include "NodeManager/nodemanager.h"
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
    PluginManager *pluginManager() const;
    RecipeManager *recipeManager() const;

signals:
    void started();
    void stopped();
    void logMessage(const QString &message);

private:
    PluginManager *m_pluginManager;
    RecipeManager *m_recipeManager;
    bool m_running;
};

#endif // DAEMON_H
