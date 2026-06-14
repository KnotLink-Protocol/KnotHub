#ifndef KNOTHUBCORE_H
#define KNOTHUBCORE_H

#include <QWidget>
#include "NodeManager/nodemanager.h"

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
    Ui::KnotHubCore *ui;
};

#endif // KNOTHUBCORE_H
