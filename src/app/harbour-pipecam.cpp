/*
 * harbour-pipecam — USB-C pipe inspection camera viewer/recorder.
 *
 * --root-helper: root helper mode, started by systemd (see roothelper.h).
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include <QtQuick>
#include <sailfishapp.h>

#include <cstdio>
#include <cstring>

#include "capturestore.h"
#include "diaglog.h"
#include "diagreport.h"
#include "mjpegrecorder.h"
#include "rootclient.h"
#include "roothelper.h"
#include "uppcamera.h"
#include "videoframeitem.h"

static int reportMain(int argc, char *argv[])
{
    bool useRoot = false;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--root") == 0)
            useRoot = true;

    MjpegRecorder::initGStreamer(&argc, &argv);
    QCoreApplication app(argc, argv);

    RootClient *rc = RootClient::instance();
    if (useRoot) {
        rc->setHelper(true);
        QElapsedTimer t;
        t.start();
        while (!rc->active() && rc->lastError().isEmpty() && t.elapsed() < 8000) {
            app.processEvents(QEventLoop::AllEvents, 100);
            QThread::msleep(50);
        }
    }

    DiagReport report;
    QVariantMap state;
    state.insert(QStringLiteral("mode"), QStringLiteral("command line, camera worker not running"));
    const QString text = report.build(state, true, useRoot);
    if (useRoot)
        rc->setHelper(false);
    fputs(text.toUtf8().constData(), stdout);
    return 0;
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--root-helper") == 0)
            return rootHelperMain(argc, argv);

    /* before anything logs */
    DiagLog::install();

    /* --report [--root]: anonymised report to stdout */
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--report") == 0)
            return reportMain(argc, argv);

    /* before any MjpegRecorder exists; takes argv */
    MjpegRecorder::initGStreamer(&argc, &argv);

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    const char *uri = "harbour.pipecam";
    qmlRegisterType<UppCamera>(uri, 1, 0, "PipeCamera");
    qmlRegisterType<VideoFrameItem>(uri, 1, 0, "Viewfinder");
    qmlRegisterType<MjpegRecorder>(uri, 1, 0, "VideoRecorder");
    qmlRegisterType<CaptureStore>(uri, 1, 0, "CaptureStore");
    qmlRegisterType<DiagReport>(uri, 1, 0, "DiagReport");

    qInfo("pipecam: %s starting", qPrintable(DiagReport::appVersion()));

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("diagLog"), DiagLog::instance());
    view->rootContext()->setContextProperty(QStringLiteral("rootHelper"), RootClient::instance());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
