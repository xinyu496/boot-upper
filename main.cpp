#include "iapcodec.h"
#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <cstdio>
#include <windows.h>

static void attachParentConsole()
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        return;
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}
#endif

int main(int argc, char *argv[])
{
    bool selfCheckOnly = false;
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--self-check") == 0)
            selfCheckOnly = true;
    }
    if (selfCheckOnly) {
#ifdef Q_OS_WIN
        attachParentConsole();
#endif
        const QString error = IapCodec::selfCheckError();
        if (!error.isEmpty()) {
            fprintf(stderr, "self-check failed: %s\n", error.toUtf8().constData());
            return 1;
        }
        printf("self-check ok\n");
        fflush(stdout);
        return 0;
    }

    QApplication application(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QCoreApplication::setOrganizationName(QStringLiteral("BootUpper"));
    QCoreApplication::setApplicationName(QStringLiteral("BootUpper"));

    const QString error = IapCodec::selfCheckError();
    if (!error.isEmpty()) {
        QMessageBox::critical(nullptr, QStringLiteral("协议自检失败"), error);
        return 1;
    }

    MainWindow window;
    window.show();
    return QApplication::exec();
}
