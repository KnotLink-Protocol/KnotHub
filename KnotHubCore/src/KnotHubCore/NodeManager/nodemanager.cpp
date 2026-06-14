#include "nodemanager.h"
#include <QtDebug>

NodeManager::NodeManager()
{
    NodeLoader loader;
    qDebug() << "hh";
    loader.start("E:/ScientificAndTechnological/ApplicationDesign/Project/KnotLink/KnotLinkedFlexiNode/MsgNotification/dist/MsgNotification.exe", QStringList() << "--arg1" << "value");
    if (loader.statuscheck())
        qDebug() << "Running";
    loader.stop();
}
