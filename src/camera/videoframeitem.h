/*
 * videoframeitem.h — the viewfinder: a QQuickItem that shows UppCamera's most
 * recent frame as a scene-graph texture.
 *
 * WHY NOT Image + QQuickImageProvider
 * -----------------------------------
 * The obvious QML route is an Image whose source points at a QQuickImageProvider
 * and is nudged with a cache-busting suffix every frame. That works, but at 15
 * fps it re-enters the QML image pipeline 15 times a second, re-parses a URL,
 * and defeats the texture cache by design. A QQuickItem that owns one texture
 * and re-uploads it in place is both simpler to reason about and markedly
 * cheaper — and it gives us the source-rect crop that digital zoom needs.
 *
 * THREADING
 * ---------
 * updatePaintNode() runs on the render thread, but only while the GUI thread is
 * blocked in the sync phase. Reading the camera's current frame from there is
 * therefore safe without a lock, which is exactly why the frame is pulled in
 * updatePaintNode() rather than pushed into the item from the frame signal.
 *
 * ZOOM
 * ----
 * `zoom` crops the source rectangle instead of scaling the drawn quad, so
 * zooming in shows the centre of the sensor image at full texture resolution
 * rather than magnifying an already-fitted picture. At 640x480 on a 1080p-class
 * display the frame is upscaled anyway, so cropping costs no real detail and
 * makes 2-4x genuinely useful for reading a crack or a joint inside a pipe.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef VIDEOFRAMEITEM_H
#define VIDEOFRAMEITEM_H

#include <QImage>
#include <QQuickItem>

class UppCamera;

class VideoFrameItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(UppCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY mirroredChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal maxZoom READ maxZoom CONSTANT)
    /* Roll, in degrees, applied about the centre of the item.
     *
     * A camera head sliding down a pipe rotates as it goes — it is on the end
     * of ten metres of cable and nothing holds its orientation. Being able to
     * turn the picture back upright is not a nicety; without it you are reading
     * a sideways or upside-down image of a joint and guessing which way "down"
     * is inside the pipe.
     *
     * This does NOT use QQuickItem::rotation: that would rotate the item's own
     * coordinate system, and the pan/zoom arithmetic — which works in item
     * pixels — would rotate with it and stop meaning anything. Instead the
     * scene-graph node carrying the texture gets its own rotation transform,
     * leaving the item, its bounds and the gesture maths untouched. */
    Q_PROPERTY(qreal roll READ roll WRITE setRoll NOTIFY rollChanged)
    /* True once at least one frame has been shown — lets QML fade the
     * placeholder out instead of flashing it between reconnects. */
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)

public:
    enum FillMode {
        PreserveAspectFit,   /* whole frame visible, letterboxed             */
        PreserveAspectCrop,  /* fills the item, edges cropped                */
        Stretch              /* distorts; offered because a pipe is round    */
    };
    Q_ENUMS(FillMode)

    explicit VideoFrameItem(QQuickItem *parent = 0);
    ~VideoFrameItem();

    UppCamera *camera() const { return m_camera; }
    void setCamera(UppCamera *camera);

    FillMode fillMode() const { return m_fillMode; }
    void setFillMode(FillMode mode);

    bool mirrored() const { return m_mirrored; }
    void setMirrored(bool mirrored);

    qreal zoom() const { return m_zoom; }
    void setZoom(qreal zoom);
    qreal maxZoom() const;

    qreal roll() const { return m_roll; }
    void setRoll(qreal degrees);
    /* Back to level, and the natural partner of the roll indicator's centre
     * tap. Separate from setRoll(0) so QML reads as what it means. */
    Q_INVOKABLE void resetRoll();

    /* Drag the magnified picture around. Deltas are in item (screen) pixels,
     * so QML can feed mouse movement straight in. Panning is clamped so the
     * frame can never be dragged away from the edge it fills — you cannot
     * scroll black into view. Along an axis where the frame does not fill the
     * item (the pillarbox bars at 1:1) it stays centred and the delta is
     * ignored. */
    Q_INVOKABLE void panBy(qreal dx, qreal dy);
    Q_INVOKABLE void resetPan();
    /* True when the current zoom leaves anything to pan to — QML uses it to
     * decide whether a drag should move the image. */
    Q_INVOKABLE bool canPan() const;

    bool hasFrame() const { return m_hasFrame; }

signals:
    void cameraChanged();
    void fillModeChanged();
    void mirroredChanged();
    void zoomChanged();
    void rollChanged();
    void hasFrameChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

private slots:
    void onFrameAvailable();
    void onCameraDestroyed();

private:
    /* Work out, for the current size/fillMode/zoom/mirror, where on screen the
     * frame is drawn (*target) and which part of the texture is sampled
     * (*source). The two are computed together because they are two halves of
     * one geometric answer — see the implementation for the reasoning. */
    void computeRects(const QSize &frameSize, QRectF *target, QRectF *source) const;

    /* Current frame size, or the protocol's nominal size before the first
     * frame arrives — panning has to be clamped even while nothing is shown. */
    QSize effectiveFrameSize() const;
    /* Clamp a candidate pan offset against the current geometry and zoom. */
    QPointF clampPan(const QPointF &pan, const QSize &frameSize) const;
    /* Shrink factor that keeps a rolled frame inside the item. */
    qreal rollFitFactor(const QSizeF &drawn, const QRectF &bounds) const;

    UppCamera *m_camera;
    FillMode m_fillMode;
    bool m_mirrored;
    qreal m_zoom;
    /* Translation of the drawn frame, in item pixels. Always kept clamped. */
    QPointF m_pan;
    qreal m_roll;
    bool m_hasFrame;

    /* Set when a new frame arrived; cleared once it has been uploaded. Avoids
     * re-uploading an unchanged image when the item is simply re-rendered. */
    bool m_textureDirty;
    QImage m_pendingImage;
};

#endif // VIDEOFRAMEITEM_H
