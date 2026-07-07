#ifndef KNOTHUBCORE_H
#define KNOTHUBCORE_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include "daemon.h"

namespace Ui {
class KnotHubCore;
}

class KnotHubCore : public QWidget
{
    Q_OBJECT

public:
    explicit KnotHubCore(QWidget *parent = nullptr);
    ~KnotHubCore();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showWindow();

private:
    void createTrayIcon();
    void updateTrayTooltip();

    Ui::KnotHubCore *ui;
    Daemon *m_daemon;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
};

#endif // KNOTHUBCORE_H
