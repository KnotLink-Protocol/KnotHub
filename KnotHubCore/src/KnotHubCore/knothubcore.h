#ifndef KNOTHUBCORE_H
#define KNOTHUBCORE_H

#include <QWidget>
#include "NodeManager/nodemanager.h"

#include <KnotLinkLib>

namespace Ui {
class KnotHubCore;
}

class KnotHubCore : public QWidget
{
    Q_OBJECT

public:
    explicit KnotHubCore(QWidget *parent = 0);
    ~KnotHubCore();

private:
    PluginManager *m_pluginManager;
    Ui::KnotHubCore *ui;
};

#endif // KNOTHUBCORE_H
