#include "daemon.h"
#include <QDebug>
#include <QCoreApplication>

Daemon::Daemon(QObject *parent)
    : QObject(parent)
    , m_pluginManager(nullptr)
    , m_running(false)
{
}

Daemon::~Daemon()
{
    stop();
}

bool Daemon::start()
{
    if (m_running) return true;

    m_pluginManager = new PluginManager(this);
    m_pluginManager->setPluginsRoot(QCoreApplication::applicationDirPath() + "/Plugins");
    m_pluginManager->refreshPluginList();
    m_pluginManager->startAutoStartPlugins();

    connect(m_pluginManager, &PluginManager::pluginListChanged, this, [this](const QStringList &list) {
        emit logMessage(QString("Plugin list changed: %1 plugins").arg(list.size()));
    });
    connect(m_pluginManager, &PluginManager::pluginStarted, this, [this](const QString &name) {
        emit logMessage(QString("Plugin started: %1").arg(name));
    });
    connect(m_pluginManager, &PluginManager::pluginStopped, this, [this](const QString &name) {
        emit logMessage(QString("Plugin stopped: %1").arg(name));
    });
    connect(m_pluginManager, &PluginManager::pluginError, this, [this](const QString &name, const QString &err) {
        emit logMessage(QString("Plugin error [%1]: %2").arg(name, err));
    });

    m_running = true;
    emit started();
    emit logMessage("KnotHub daemon started on 127.0.0.1:6376");
    return true;
}

void Daemon::stop()
{
    if (!m_running) return;

    m_pluginManager->stopAllPlugins();
    m_running = false;
    emit stopped();
    emit logMessage("KnotHub daemon stopped");
}

bool Daemon::isRunning() const
{
    return m_running;
}

PluginManager *Daemon::pluginManager() const
{
    return m_pluginManager;
}
