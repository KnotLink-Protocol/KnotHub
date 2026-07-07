#include "knothubcore.h"
#include "ui_knothubcore.h"
#include <QApplication>
#include <QCloseEvent>
#include <QAction>
#include <QStyle>

KnotHubCore::KnotHubCore(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::KnotHubCore)
    , m_daemon(new Daemon(this))
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    ui->setupUi(this);

    // 启动守护进程
    connect(m_daemon, &Daemon::logMessage, this, [](const QString &msg) {
        qDebug() << msg;
    });

    m_daemon->start();

    // 创建托盘
    createTrayIcon();

    // 默认不显示主窗口
    hide();
}

KnotHubCore::~KnotHubCore()
{
    m_daemon->stop();
    delete ui;
}

void KnotHubCore::createTrayIcon()
{
    m_trayMenu = new QMenu(this);

    QAction *showAction = m_trayMenu->addAction("显示主窗口");
    connect(showAction, &QAction::triggered, this, &KnotHubCore::showWindow);

    m_trayMenu->addSeparator();

    QAction *quitAction = m_trayMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setToolTip("KnotHub 守护进程");

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &KnotHubCore::onTrayActivated);

    m_trayIcon->show();
}

void KnotHubCore::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showWindow();
    }
}

void KnotHubCore::showWindow()
{
    if (isVisible()) {
        hide();
    } else {
        show();
        raise();
        activateWindow();
    }
}

void KnotHubCore::updateTrayTooltip()
{
    int count = m_daemon->pluginManager()->pluginNames().size();
    m_trayIcon->setToolTip(QString("KnotHub 守护进程\n插件数: %1").arg(count));
}
