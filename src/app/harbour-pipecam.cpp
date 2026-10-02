/*
 * harbour-pipecam — a viewer and recorder for USB-C pipe inspection cameras
 * on Sailfish OS.
 *
 * Entry point. Registers the three C++ types QML needs and hands off to
 * libsailfishapp:
 *
 *   PipeCam.Camera    UppCamera      the USB camera (see src/camera/uppcamera.h)
 *   PipeCam.Viewfinder VideoFrameItem the scene-graph item that draws frames
 *   PipeCam.Recorder  MjpegRecorder  MJPEG -> .mp4 muxer
 *   PipeCam.Captures  CaptureStore   where snapshots and videos live
 *   PipeCam.DiagReport DiagReport    the anonymised diagnostic report
 *
 * and two context properties, diagLog (DiagLog) and rootHelper (RootClient).
 *
 * `harbour-pipecam --root-helper` is not the app: it is the optional root
 * helper, started by systemd for the diagnostic report (src/diag/roothelper.h).
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

    /* Before anything can log, so the report sees the whole run. */
    DiagLog::install();

    /* `harbour-pipecam --report [--root]`: the same anonymised report as the
     * About page, on stdout, for a terminal or for when the UI will not come
     * up at all. --root asks the helper for the kernel's view as well. */
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--report") == 0)
            return reportMain(argc, argv);

    /* GStreamer must be initialised before any recorder is constructed, and it
     * wants a crack at argv. Doing it here — rather than lazily on first
     * record — means a broken plugin set is reported at startup instead of the
     * moment the user presses record. */
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
