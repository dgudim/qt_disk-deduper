#include "mainwindow.h"

#include <QtGlobal>
#include <QApplication>

void captureMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    QString msg_wrap;
    LogLevel logLevel;
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
        case QtDebugMsg:
            fprintf(stdout, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            msg_wrap = "Debug: %1";
            logLevel = LogLevel::DEBUG;
            break;
        case QtWarningMsg:
            fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            msg_wrap = "Warning: <span style=\"color: yellow\">%1</span>";
            logLevel = LogLevel::WARNING;
            break;
        case QtInfoMsg:
            fprintf(stdout, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            msg_wrap = "Info: <span style=\"color: cyan\">%1</span>";
            logLevel = LogLevel::INFO;
            break;
        case QtCriticalMsg:
            fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            msg_wrap = "Critial: <span style=\"color: red\">%1</span>";
            logLevel = LogLevel::ERROR;
            break;
        case QtFatalMsg:
            fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            abort();
    }
    // transfer to ui thread
    MainWindow::appendToLog(msg_wrap.arg(msg), type == QtCriticalMsg || type == QtWarningMsg, logLevel);
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(captureMessageOutput);
    QApplication a(argc, argv);

    a.setApplicationName("disk_deduper_qt");
    a.setApplicationDisplayName("Disk deduper");

    MainWindow w;
    w.show();
    return a.exec();
}
