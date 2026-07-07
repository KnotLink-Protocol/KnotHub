#include <QApplication>
#include <QCoreApplication>
#include <iostream>
#include "daemon.h"
#include "knothubcore.h"

#ifdef Q_OS_WIN
#include <windows.h>
// ── Windows Service ──────────────────────────────────────────
static SERVICE_STATUS        g_svcStatus;
static SERVICE_STATUS_HANDLE g_svcStatusHandle;
static Daemon               *g_svcDaemon = nullptr;

void WINAPI svcControlHandler(DWORD ctrl)
{
    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
        g_svcStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_svcStatusHandle, &g_svcStatus);
        if (g_svcDaemon) g_svcDaemon->stop();
        g_svcStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_svcStatusHandle, &g_svcStatus);
        break;
    default:
        SetServiceStatus(g_svcStatusHandle, &g_svcStatus);
        break;
    }
}

void WINAPI svcMain(DWORD /*argc*/, LPTSTR * /*argv*/)
{
    g_svcStatusHandle = RegisterServiceCtrlHandler(L"KnotHub", svcControlHandler);
    if (!g_svcStatusHandle) return;

    g_svcStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    g_svcStatus.dwCurrentState            = SERVICE_RUNNING;
    g_svcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    g_svcStatus.dwWin32ExitCode           = 0;
    g_svcStatus.dwServiceSpecificExitCode = 0;
    g_svcStatus.dwCheckPoint              = 0;
    g_svcStatus.dwWaitHint                = 0;
    SetServiceStatus(g_svcStatusHandle, &g_svcStatus);

    Daemon daemon;
    g_svcDaemon = &daemon;
    daemon.start();
}

int runService(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { (LPWSTR)L"KnotHub", svcMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(dispatchTable)) {
        std::cerr << "StartServiceCtrlDispatcher failed: " << GetLastError() << std::endl;
        return 1;
    }

    return app.exec();
}
#else
int runService(int, char **)
{
    std::cerr << "Service mode only supported on Windows" << std::endl;
    return 1;
}
#endif

// ── Console 模式 ─────────────────────────────────────────────
int runConsole(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Daemon daemon;

    QObject::connect(&daemon, &Daemon::logMessage, [](const QString &msg) {
        std::cout << "[KnotHub] " << msg.toStdString() << std::endl;
    });

    if (!daemon.start()) return 1;

    return app.exec();
}

// ── Tray 模式（默认）─────────────────────────────────────────
int runTray(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    KnotHubCore w;
    return app.exec();
}

// ── 入口 ─────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    // 简单参数解析（不依赖 QCommandLineParser，避免强依赖 QApplication）
    bool isService = false;
    bool isConsole = false;

    for (int i = 1; i < argc; i++) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--service" || arg == "-s") isService = true;
        if (arg == "--console" || arg == "-c") isConsole = true;
    }

    if (isService)
        return runService(argc, argv);
    else if (isConsole)
        return runConsole(argc, argv);
    else
        return runTray(argc, argv);
}
