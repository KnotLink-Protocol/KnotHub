#include "knothubcore.h"
#include "ui_knothubcore.h"

#include <QtDebug>

KnotHubCore::KnotHubCore(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KnotHubCore)
{
    ui->setupUi(this);
    qDebug() << "nh";
    NodeManager *nodemgr=new NodeManager;
}

KnotHubCore::~KnotHubCore()
{
    delete ui;
}
