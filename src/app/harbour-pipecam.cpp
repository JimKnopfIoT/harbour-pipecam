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
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include <QtQuick>
#include <sailfishapp.h>

#include "capturestore.h"
#include "mjpegrecorder.h"
#include "uppcamera.h"
#include "videoframeitem.h"

int main(int argc, char *argv[])
{
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

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
