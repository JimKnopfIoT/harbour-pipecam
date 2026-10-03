/*
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "videoframeitem.h"
#include "uppcamera.h"

#include <QMatrix4x4>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QSGTransformNode>
#include <QtMath>

static const qreal MAX_ZOOM = 8.0;

VideoFrameItem::VideoFrameItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_camera(0)
    , m_fillMode(PreserveAspectFit)
    , m_mirrored(false)
    , m_zoom(1.0)
    , m_pan(0, 0)
    , m_roll(0.0)
    , m_hasFrame(false)
    , m_textureDirty(false)
{
    setFlag(ItemHasContents, true);
}

VideoFrameItem::~VideoFrameItem()
{
}

qreal VideoFrameItem::maxZoom() const
{
    return MAX_ZOOM;
}

void VideoFrameItem::setCamera(UppCamera *camera)
{
    if (m_camera == camera)
        return;

    if (m_camera) {
        disconnect(m_camera, 0, this, 0);
    }
    m_camera = camera;
    if (m_camera) {
        connect(m_camera, SIGNAL(frameAvailable()), this, SLOT(onFrameAvailable()));
        /* Either may be destroyed first. */
        connect(m_camera, SIGNAL(destroyed()), this, SLOT(onCameraDestroyed()));
    }

    if (m_hasFrame) {
        m_hasFrame = false;
        emit hasFrameChanged();
    }
    m_pendingImage = QImage();
    m_textureDirty = true;
    update();
    emit cameraChanged();
}

void VideoFrameItem::onCameraDestroyed()
{
    m_camera = 0;
    emit cameraChanged();
}

void VideoFrameItem::setFillMode(FillMode mode)
{
    if (m_fillMode == mode)
        return;
    m_fillMode = mode;
    update();
    emit fillModeChanged();
}

void VideoFrameItem::setMirrored(bool mirrored)
{
    if (m_mirrored == mirrored)
        return;
    m_mirrored = mirrored;
    update();
    emit mirroredChanged();
}

void VideoFrameItem::setZoom(qreal zoom)
{
    if (zoom < 1.0)
        zoom = 1.0;
    if (zoom > MAX_ZOOM)
        zoom = MAX_ZOOM;
    if (qFuzzyCompare(m_zoom, zoom))
        return;
    m_zoom = zoom;
    m_pan = clampPan(m_pan, effectiveFrameSize());
    update();
    emit zoomChanged();
}

QSize VideoFrameItem::effectiveFrameSize() const
{
    if (!m_pendingImage.isNull())
        return m_pendingImage.size();
    return QSize(640, 480);
}

QPointF VideoFrameItem::clampPan(const QPointF &pan, const QSize &frameSize) const
{
    const QRectF bounds(0, 0, width(), height());
    if (frameSize.isEmpty() || bounds.isEmpty() || m_fillMode == Stretch)
        return QPointF(0, 0);

    const qreal fw = frameSize.width();
    const qreal fh = frameSize.height();
    const qreal sFit  = qMin(bounds.width() / fw, bounds.height() / fh);
    const qreal sFill = qMax(bounds.width() / fw, bounds.height() / fh);
    qreal sBase = (m_fillMode == PreserveAspectCrop) ? sFill : sFit;
    /* Must match computeRects() exactly, roll shrink included. */
    if (m_fillMode == PreserveAspectFit)
        sBase *= rollFitFactor(QSizeF(fw * sBase, fh * sBase), bounds);
    const qreal s = sBase * m_zoom;

    const qreal dw = fw * s;
    const qreal dh = fh * s;

    const qreal maxX = qMax(qreal(0), (dw - bounds.width())  / 2.0);
    const qreal maxY = qMax(qreal(0), (dh - bounds.height()) / 2.0);

    return QPointF(qBound(-maxX, pan.x(), maxX),
                   qBound(-maxY, pan.y(), maxY));
}

bool VideoFrameItem::canPan() const
{
    const QSize fs = effectiveFrameSize();
    const QPointF limit = clampPan(QPointF(1e6, 1e6), fs);
    return limit.x() > 0.5 || limit.y() > 0.5;
}

void VideoFrameItem::panBy(qreal dx, qreal dy)
{
    const QPointF wanted = m_pan + QPointF(dx, dy);
    const QPointF clamped = clampPan(wanted, effectiveFrameSize());
    if (qFuzzyCompare(clamped.x(), m_pan.x()) && qFuzzyCompare(clamped.y(), m_pan.y()))
        return;
    m_pan = clamped;
    update();
}

void VideoFrameItem::resetPan()
{
    if (m_pan.isNull())
        return;
    m_pan = QPointF(0, 0);
    update();
}

void VideoFrameItem::setRoll(qreal degrees)
{
    /* Normalise to (-180, 180]. */
    while (degrees > 180.0)  degrees -= 360.0;
    while (degrees <= -180.0) degrees += 360.0;

    if (qFuzzyCompare(m_roll, degrees))
        return;
    m_roll = degrees;
    /* Fit scale depends on roll. */
    m_pan = clampPan(m_pan, effectiveFrameSize());
    update();
    emit rollChanged();
}

void VideoFrameItem::resetRoll()
{
    setRoll(0.0);
}

/* Rotated bbox: w|cosθ| + h|sinθ| by w|sinθ| + h|cosθ|. */
qreal VideoFrameItem::rollFitFactor(const QSizeF &drawn, const QRectF &bounds) const
{
    if (qFuzzyIsNull(m_roll) || drawn.isEmpty() || bounds.isEmpty())
        return 1.0;

    const qreal rad = qDegreesToRadians(m_roll);
    const qreal c = qAbs(qCos(rad));
    const qreal s = qAbs(qSin(rad));

    const qreal bw = drawn.width() * c + drawn.height() * s;
    const qreal bh = drawn.width() * s + drawn.height() * c;
    if (bw <= 0 || bh <= 0)
        return 1.0;

    const qreal f = qMin(bounds.width() / bw, bounds.height() / bh);
    /* Shrink only. */
    return qMin(qreal(1.0), f);
}

void VideoFrameItem::onFrameAvailable()
{
    if (!m_camera)
        return;
    /* GUI-thread copy (COW) read by updatePaintNode(). */
    m_pendingImage = m_camera->currentImage();
    m_textureDirty = true;
    if (!m_hasFrame && !m_pendingImage.isNull()) {
        /* GUI thread only, not in updatePaintNode(). */
        m_hasFrame = true;
        emit hasFrameChanged();
    }
    update();
}

void VideoFrameItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}

/* s = s_base * zoom; drawn rect intersected with item, mapped back to texture. */
void VideoFrameItem::computeRects(const QSize &frameSize,
                                  QRectF *target, QRectF *source) const
{
    const QRectF bounds(0, 0, width(), height());
    const qreal fw = frameSize.width();
    const qreal fh = frameSize.height();

    if (frameSize.isEmpty() || bounds.isEmpty()) {
        *target = bounds;
        *source = QRectF(0, 0, fw, fh);
        return;
    }

    if (m_fillMode == Stretch) {
        const qreal w = fw / m_zoom;
        const qreal h = fh / m_zoom;
        *target = bounds;
        *source = QRectF((fw - w) / 2.0, (fh - h) / 2.0, w, h);
    } else {
        const qreal sFit  = qMin(bounds.width() / fw, bounds.height() / fh);
        const qreal sFill = qMax(bounds.width() / fw, bounds.height() / fh);
        qreal sBase = (m_fillMode == PreserveAspectCrop) ? sFill : sFit;
        if (m_fillMode == PreserveAspectFit)
            sBase *= rollFitFactor(QSizeF(fw * sBase, fh * sBase), bounds);
        const qreal s = sBase * m_zoom;

        const QPointF pan = clampPan(m_pan, frameSize);
        const QRectF drawn((bounds.width()  - fw * s) / 2.0 + pan.x(),
                           (bounds.height() - fh * s) / 2.0 + pan.y(),
                           fw * s, fh * s);

        const QRectF visible = drawn.intersected(bounds);
        *target = visible;
        *source = QRectF((visible.x() - drawn.x()) / s,
                         (visible.y() - drawn.y()) / s,
                         visible.width()  / s,
                         visible.height() / s);
    }

    if (m_mirrored)
        *source = QRectF(source->right(), source->top(),
                         -source->width(), source->height());
}

QSGNode *VideoFrameItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    /* QSGTransformNode (roll) -> QSGSimpleTextureNode. */
    QSGTransformNode *root = static_cast<QSGTransformNode *>(oldNode);

    const QImage image = m_pendingImage;
    if (image.isNull() || width() <= 0 || height() <= 0) {
        /* No node: placeholder underneath shows. */
        delete root;
        return 0;
    }

    if (!root)
        root = new QSGTransformNode();

    QSGSimpleTextureNode *node =
            root->childCount() > 0
            ? static_cast<QSGSimpleTextureNode *>(root->firstChild())
            : 0;
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setFiltering(QSGTexture::Linear);
        root->appendChildNode(node);
        m_textureDirty = true;
    }

    if (m_textureDirty) {
        QSGTexture *texture = window()->createTextureFromImage(image);
        if (!texture) {
            delete root;
            return 0;
        }
        /* Node owns the texture; frees the old one on setTexture(). */
        node->setOwnsTexture(true);
        node->setTexture(texture);
        m_textureDirty = false;
    }

    QRectF target, source;
    computeRects(image.size(), &target, &source);
    node->setRect(target);
    node->setSourceRect(source);

    /* About item centre, not frame centre. */
    QMatrix4x4 m;
    if (!qFuzzyIsNull(m_roll)) {
        m.translate(width() / 2.0, height() / 2.0);
        m.rotate(m_roll, 0.0, 0.0, 1.0);
        m.translate(-width() / 2.0, -height() / 2.0);
    }
    if (root->matrix() != m) {
        root->setMatrix(m);
        root->markDirty(QSGNode::DirtyMatrix);
    }
    return root;
}
