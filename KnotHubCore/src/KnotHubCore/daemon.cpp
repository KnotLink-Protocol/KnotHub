#include "daemon.h"
#include <QDebug>
#include <QCoreApplication>

Daemon::Daemon(QObject *parent)
    : QObject(parent)
    , m_pluginManager(nullptr)
    , m_standaloneManager(nullptr)
    , m_recipeManager(nullptr)
    , m_refreshTimer(nullptr)
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

    // ── 1. 插入式节点管理器 — socketID: 0x00000011 ──────
    m_pluginManager = new PluginManager(this);
    m_pluginManager->setPluginsRoot(
        QCoreApplication::applicationDirPath() + "/Plugins");
    m_pluginManager->refreshPluginList();
    m_pluginManager->startAutoStartPlugins();

    connect(m_pluginManager, &PluginManager::pluginListChanged,
            this, [this](const QStringList &list) {
        emit logMessage(QString("Plugins refreshed: %1").arg(list.size()));
    });
    connect(m_pluginManager, &PluginManager::pluginStarted,
            this, [this](const QString &name) {
        emit logMessage(QString("Plugin started: %1").arg(name));
    });
    connect(m_pluginManager, &PluginManager::pluginStopped,
            this, [this](const QString &name) {
        emit logMessage(QString("Plugin stopped: %1").arg(name));
    });
    connect(m_pluginManager, &PluginManager::pluginError,
            this, [this](const QString &name, const QString &err) {
        emit logMessage(QString("Plugin error [%1]: %2").arg(name, err));
    });

    // ── 2. 独立式节点管理器 — socketID: 0x00000012 ──────
    m_standaloneManager = new StandaloneManager(this);
    m_standaloneManager->scan();

    connect(m_standaloneManager, &StandaloneManager::nodeListChanged,
            this, [this]() {
        int count = m_standaloneManager->nodes().size();
        emit logMessage(QString("Standalone nodes refreshed: %1").arg(count));
    });

    // ── 3. 配方管理器 — socketID: 0x00000013 ────────────
    m_recipeManager = new RecipeManager(this);
    m_recipeManager->setRecipesRoot(
        QCoreApplication::applicationDirPath() + "/Recipes");

    connect(m_recipeManager, &RecipeManager::recipeStarted,
            this, [this](const QString &path) {
        emit logMessage(QString("Recipe started: %1").arg(path));
    });
    connect(m_recipeManager, &RecipeManager::recipeStopped,
            this, [this](const QString &path) {
        emit logMessage(QString("Recipe stopped: %1").arg(path));
    });

    // ── 4. 定时刷新 ─────────────────────────────────────
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(15000);  // 每 15 秒刷新一次
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        m_pluginManager->refreshPluginList();
        m_standaloneManager->scan();
        m_recipeManager->refreshTree();
    });
    m_refreshTimer->start();

    m_running = true;
    emit started();
    emit logMessage("KnotHub started (plugins + standalone + recipes + auto-refresh)");
    return true;
}

void Daemon::stop()
{
    if (!m_running) return;

    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
    m_pluginManager->stopAllPlugins();
    m_recipeManager->stopAll();
    m_running = false;
    emit stopped();
    emit logMessage("KnotHub stopped");
}

bool Daemon::isRunning() const
{
    return m_running;
}
