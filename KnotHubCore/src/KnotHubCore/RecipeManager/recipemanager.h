#ifndef RECIPEMANAGER_H
#define RECIPEMANAGER_H

#include <QObject>
#include <QDir>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "../NodeManager/nodeloader.h"
#include "../KnotLinkLib/kludf.h"

class OpenSocketResponser;

class RecipeManager : public QObject
{
    Q_OBJECT
public:
    explicit RecipeManager(QObject *parent = nullptr);
    ~RecipeManager();

    void setRecipesRoot(const QString &path);
    QString recipesRoot() const { return m_recipesRoot; }

    // 目录树
    QByteArray scanTree() const;
    void refreshTree();

    // 进程管理
    bool runRecipe(const QString &filePath);
    bool stopRecipe(const QString &filePath);
    bool isRunning(const QString &filePath) const;
    void stopAll();

    // 文件操作
    QByteArray readRecipe(const QString &filePath) const;
    bool saveRecipe(const QString &filePath, const QString &content);
    bool deleteRecipe(const QString &filePath);
    bool importRecipe(const QString &sourcePath, const QString &targetDir,
                      bool overwrite, QString &error);
    bool createFolder(const QString &path, QString &error);

signals:
    void recipeStarted(const QString &filePath);
    void recipeStopped(const QString &filePath);
    void logMessage(const QString &message);

private slots:
    void onKnotLinkData(const QString &data, const QString &questionID);

private:
    QJsonArray scanChildren(const QDir &dir) const;
    NodeLoader *getOrCreateLoader(const QString &filePath);
    void removeLoader(const QString &filePath);

    OpenSocketResponser *m_responder;
    QString m_recipesRoot;
    QByteArray m_cachedTree;
    QMap<QString, NodeLoader *> m_loaders;
};

#endif // RECIPEMANAGER_H
