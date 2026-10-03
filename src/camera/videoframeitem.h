/*
 * Viewfinder: UppCamera's latest frame as a scene-graph texture.
 * updatePaintNode() runs on the render thread while the GUI thread is blocked
 * (sync phase): members may be read there without a lock.
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
    /* Degrees, (-180, 180], about the item centre. Applied on the scene-graph
     * node, not QQuickItem::rotation: pan/zoom maths works in unrotated item pixels. */
    Q_PROPERTY(qreal roll READ roll WRITE setRoll NOTIFY rollChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)

public:
    enum FillMode {
        PreserveAspectFit,
        PreserveAspectCrop,
        Stretch
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
    Q_INVOKABLE void resetRoll();

    /* Item pixels. Clamped: no black scrolled into view; non-filling axis stays centred. */
    Q_INVOKABLE void panBy(qreal dx, qreal dy);
    Q_INVOKABLE void resetPan();
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
    /* *target: screen rect; *source: sampled texture rect. */
    void computeRects(const QSize &frameSize, QRectF *target, QRectF *source) const;

    /* Nominal size before first frame. */
    QSize effectiveFrameSize() const;
    QPointF clampPan(const QPointF &pan, const QSize &frameSize) const;
    /* <= 1.0 */
    qreal rollFitFactor(const QSizeF &drawn, const QRectF &bounds) const;

    UppCamera *m_camera;
    FillMode m_fillMode;
    bool m_mirrored;
    qreal m_zoom;
    /* Item pixels, always clamped. */
    QPointF m_pan;
    qreal m_roll;
    bool m_hasFrame;

    bool m_textureDirty;
    QImage m_pendingImage;
};

#endif // VIDEOFRAMEITEM_H
