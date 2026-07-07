#include "recipemanager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QCoreApplication>

RecipeManager::RecipeManager(QObject *parent)
    : QObject(parent)
    , m_recipesRoot(QCoreApplication::applicationDirPath() + "/Recipes")
{
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

    // 再 Python 文件
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.py", QDir::Files, QDir::Name);
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

    loader->start("python", QStringList() << filePath);
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

// ── KLUDF 命令 ──────────────────────────────────────────────

QString RecipeManager::handleCommand(const QString &cmd, const KLKVMap &kvMap)
{
    if (cmd == "get_recipe_tree") {
        return QString::fromUtf8(scanTree());
    }
    else if (cmd == "recipe_run") {
        QString path = kvMap["file_path"];
        return runRecipe(path) ? "ok" : "error: failed to run recipe";
    }
    else if (cmd == "recipe_stop") {
        QString path = kvMap["file_path"];
        return stopRecipe(path) ? "ok" : "error: failed to stop recipe";
    }
    else if (cmd == "recipe_status") {
        QString path = kvMap["file_path"];
        return isRunning(path) ? "running" : "stopped";
    }
    else if (cmd == "recipe_read") {
        QString path = kvMap["file_path"];
        QByteArray content = readRecipe(path);
        return content.isEmpty() ? "error: cannot read file" : QString::fromUtf8(content);
    }
    else if (cmd == "recipe_save") {
        QString path = kvMap["file_path"];
        QString content = kvMap["content"];
        return saveRecipe(path, content) ? "ok" : "error: cannot save file";
    }
    else if (cmd == "recipe_delete") {
        QString path = kvMap["file_path"];
        return deleteRecipe(path) ? "ok" : "error: cannot delete";
    }

    return QString(); // 不是配方命令
}
