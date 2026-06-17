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
    m_pluginManager->startAutoStartPlugins();

    KLKVMap map;
    map.deserialize("key1=value1;key2=value2");


//    QList<OpenSocketResponser*> responsers;
//    for (int i = 0; i < 100; ++i) {
//        // 如果你希望每个实例有不同的参数，可以动态生成
//        QString param1 = QString("0x%1").arg(i, 8, 16, QChar('0'));
//        QString param2 = QString("0x%1").arg(i + 100, 8, 16, QChar('0'));
//        OpenSocketResponser *resp = new OpenSocketResponser(param1, param2, this);
//        // 如果该对象需要连接服务器，可能需要调用 start() 等方法
//        // resp->start();
//        responsers.append(resp);
//    }
}

KnotHubCore::~KnotHubCore()
{
    delete ui;
}
