#include "knothubcore.h"
#include "ui_knothubcore.h"

#include <QtDebug>

KnotHubCore::KnotHubCore(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KnotHubCore)
{
    ui->setupUi(this);
    qDebug() << "nh";
    // 构造函数中
    m_pluginManager = new PluginManager(this);
    m_pluginManager->setPluginsRoot(QCoreApplication::applicationDirPath() + "/Plugins");
    m_pluginManager->refreshPluginList();
//    m_pluginManager->startAutoStartPlugins();

    KLKVMap map;
    map.deserialize("key1=value1;key2=value2");
}

KnotHubCore::~KnotHubCore()
{
    delete ui;
}
