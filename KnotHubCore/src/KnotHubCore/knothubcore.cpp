#include "knothubcore.h"
#include "ui_knothubcore.h"
#include <QApplication>
#include <QCloseEvent>
#include <QAction>
#include <QStyle>
#include <QDateTime>
#include <QScrollBar>

KnotHubCore::KnotHubCore(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::KnotHubCore)
    , m_daemon(new Daemon(this))
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    ui->setupUi(this);

    // ── 日志输出 ──────────────────────────────────────────
    connect(m_daemon, &Daemon::logMessage, this, &KnotHubCore::appendLog);

    // ── 清空按钮 ──────────────────────────────────────────
    connect(ui->clearBtn, &QPushButton::clicked, this, [this]() {
        ui->logView->clear();
    });

    // ── 启动守护进程 ──────────────────────────────────────
    m_daemon->start();
    appendLog("KnotHub 守护进程已启动");

    // ── 创建托盘 ──────────────────────────────────────────
    createTrayIcon();

    // ── 更新统计 ──────────────────────────────────────────
    updateStatus();

    // ── 默认不显示主窗口 ──────────────────────────────────
    hide();
}

KnotHubCore::~KnotHubCore()
{
    m_daemon->stop();
    delete ui;
}

void KnotHubCore::appendLog(const QString &msg)
{
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->logView->appendPlainText(QString("[%1] %2").arg(ts, msg));

    // 自动滚到底部
    QScrollBar *bar = ui->logView->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void KnotHubCore::updateStatus()
{
    int plugins    = m_daemon->pluginManager()->pluginNames().size();
    int standalones = m_daemon->standaloneManager()->nodes().size();

    ui->statusLabel->setText(
        QString("插件: %1  |  独立式: %2").arg(plugins).arg(standalones));

    updateTrayTooltip();
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
        updateStatus();
        show();
        raise();
        activateWindow();
    }
}

void KnotHubCore::updateTrayTooltip()
{
    int plugins     = m_daemon->pluginManager()->pluginNames().size();
    int standalones  = m_daemon->standaloneManager()->nodes().size();
    m_trayIcon->setToolTip(
        QString("KnotHub 守护进程\n插件: %1  独立式: %2")
            .arg(plugins).arg(standalones));
}
