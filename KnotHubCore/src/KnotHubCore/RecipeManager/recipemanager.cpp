#include "recipemanager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QCoreApplication>
#include <QTemporaryDir>
#include "../quazip/JlCompress.h"
#include <KnotLinkLib>

RecipeManager::RecipeManager(QObject *parent)
    : QObject(parent)
    , m_recipesRoot(QCoreApplication::applicationDirPath() + "/Recipes")
{
    m_responder = new OpenSocketResponser("0x00000002", "0x00000013", this);
    connect(m_responder, &OpenSocketResponser::receivedData,
            this, &RecipeManager::onKnotLinkData);
}

RecipeManager::~RecipeManager()
{
    stopAll();
}

void RecipeManager::setRecipesRoot(const QString &path)
{
    QDir dir(path);
    m_recipesRoot = dir.exists() ? dir.absolutePath() : path;
}

// ── 目录树扫描 ──────────────────────────────────────────────

QByteArray RecipeManager::scanTree() const
{
    QDir dir(m_recipesRoot);
    QJsonObject root;
    root["id"] = "__root__";
    root["name"] = "Recipes";
    root["type"] = "folder";
    root["parentId"] = QJsonValue::Null;

    if (dir.exists()) {
        root["children"] = scanChildren(dir);
    } else {
        root["children"] = QJsonArray();
    }

    return QJsonDocument(root).toJson();
}

QJsonArray RecipeManager::scanChildren(const QDir &dir) const
{
    QJsonArray children;

    // 先文件夹
    QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &info : dirs) {
        QDir subDir(info.absoluteFilePath());
        QJsonObject folder;
        folder["id"] = info.absoluteFilePath();
        folder["name"] = info.fileName();
        folder["type"] = "folder";
        folder["children"] = scanChildren(subDir);
        children.append(folder);
    }

    // 配方文件：.py 和 .kln
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.py" << "*.kln", QDir::Files, QDir::Name);
    for (const QFileInfo &info : files) {
        QJsonObject recipe;
        recipe["id"] = info.absoluteFilePath();
        recipe["name"] = info.fileName();
        recipe["type"] = "recipe";
        recipe["status"] = isRunning(info.absoluteFilePath()) ? "running" : "stopped";
        children.append(recipe);
    }

    return children;
}

// ── 进程管理 ────────────────────────────────────────────────

NodeLoader *RecipeManager::getOrCreateLoader(const QString &filePath)
{
    if (m_loaders.contains(filePath))
        return m_loaders[filePath];

    NodeLoader *loader = new NodeLoader(this);
    m_loaders[filePath] = loader;
    return loader;
}

void RecipeManager::removeLoader(const QString &filePath)
{
    if (m_loaders.contains(filePath)) {
        NodeLoader *loader = m_loaders.take(filePath);
        delete loader;
    }
}

bool RecipeManager::runRecipe(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        emit logMessage(QString("Recipe not found: %1").arg(filePath));
        return false;
    }

    NodeLoader *loader = getOrCreateLoader(filePath);
    if (loader->statuscheck()) {
        emit logMessage(QString("Recipe already running: %1").arg(filePath));
        return false;
    }

    QString program = "python";
    QStringList args;

    if (filePath.endsWith(".kln")) {
        // .kln 是 zip 包，解压到临时目录，运行里面的 main.py
        QTemporaryDir *tmpDir = new QTemporaryDir();
        tmpDir->setAutoRemove(true);
        QStringList extracted = JlCompress::extractDir(filePath, tmpDir->path());
        if (extracted.isEmpty()) {
            emit logMessage(QString("Failed to extract .kln: %1").arg(filePath));
            return false;
        }
        QString mainPy = tmpDir->path() + "/main.py";
        if (!QFile::exists(mainPy)) {
            emit logMessage(QString("No main.py found in .kln: %1").arg(filePath));
            return false;
        }
        args << mainPy;
        emit logMessage(QString("Extracted .kln, running main.py"));
    } else {
        args << filePath;
    }

    loader->start(program, args);
    if (loader->statuscheck()) {
        emit recipeStarted(filePath);
        emit logMessage(QString("Recipe started: %1").arg(filePath));
        return true;
    } else {
        emit logMessage(QString("Failed to start recipe: %1").arg(filePath));
        return false;
    }
}

bool RecipeManager::stopRecipe(const QString &filePath)
{
    if (!m_loaders.contains(filePath))
        return true;

    NodeLoader *loader = m_loaders[filePath];
    if (!loader->statuscheck())
        return true;

    loader->stop();
    emit recipeStopped(filePath);
    emit logMessage(QString("Recipe stopped: %1").arg(filePath));
    return true;
}

bool RecipeManager::isRunning(const QString &filePath) const
{
    if (!m_loaders.contains(filePath))
        return false;
    return m_loaders[filePath]->statuscheck();
}

void RecipeManager::stopAll()
{
    for (auto it = m_loaders.begin(); it != m_loaders.end(); ++it) {
        if (it.value()->statuscheck()) {
            it.value()->stop();
        }
    }
}

// ── 文件操作 ────────────────────────────────────────────────

QByteArray RecipeManager::readRecipe(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return file.readAll();
}

bool RecipeManager::saveRecipe(const QString &filePath, const QString &content)
{
    QFileInfo info(filePath);
    QDir().mkpath(info.absolutePath()); // 确保父目录存在

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(content.toUtf8());
    file.close();
    return true;
}

bool RecipeManager::deleteRecipe(const QString &filePath)
{
    QFileInfo info(filePath);
    if (info.isDir()) {
        QDir dir(filePath);
        return dir.removeRecursively();
    }
    return QFile::remove(filePath);
}

// ═══════════════════════════════════════════════════════════════
// KnotLink 消息处理 — socketID: 0x00000013（配方专用）
// ═══════════════════════════════════════════════════════════════

void RecipeManager::onKnotLinkData(const QString &data, const QString &questionID)
{
    KLKVMap kvMap;
    kvMap.deserialize(data);

    qDebug() << "[Recipe] KL data:" << kvMap;

    QString cmd = kvMap["cmd"];
    QString reply;

    if (cmd == "get_recipe_tree") {
        reply = QString::fromUtf8(scanTree());

    } else if (cmd == "recipe_run") {
        QString path = kvMap["file_path"];
        reply = runRecipe(path) ? "ok" : "error: failed to run recipe";

    } else if (cmd == "recipe_stop") {
        QString path = kvMap["file_path"];
        reply = stopRecipe(path) ? "ok" : "error: failed to stop recipe";

    } else if (cmd == "recipe_status") {
        QString path = kvMap["file_path"];
        reply = isRunning(path) ? "running" : "stopped";

    } else if (cmd == "recipe_read") {
        QString path = kvMap["file_path"];
        QByteArray content = readRecipe(path);
        reply = content.isEmpty() ? "error: cannot read file" : QString::fromUtf8(content);

    } else if (cmd == "recipe_save") {
        QString path = kvMap["file_path"];
        QString content = kvMap["content"];
        reply = saveRecipe(path, content) ? "ok" : "error: cannot save file";

    } else if (cmd == "recipe_delete") {
        QString path = kvMap["file_path"];
        reply = deleteRecipe(path) ? "ok" : "error: cannot delete";

    } else {
        reply = "Error: unknown command: " + cmd;
    }

    m_responder->sendBack(reply, questionID);
}
