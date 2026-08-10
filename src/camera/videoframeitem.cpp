/*
 * videoframeitem.cpp — see videoframeitem.h.
 *
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

/* Digital zoom ceiling.
 *
 * The sensor gives us 640x480, so at 8x the sampled window is only 80x60 pixels
 * and the display is interpolating heavily — past roughly 4x no new detail can
 * appear, only bigger pixels. It still earns its place: when you are trying to
 * decide whether a dark line at the far end of a pipe is a crack or a shadow,
 * magnifying the JPEG blocks is genuinely easier to judge than squinting at a
 * 1:1 image, and the alternative is holding the phone closer to your face while
 * both hands are busy with cable. So the limit is set by usefulness, not by
 * optics. */
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
        /* The camera is owned by QML/the root context and can outlive or
         * predecease us in either order; drop the pointer if it goes first. */
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
    /* Zooming out shrinks the frame, so an offset that was legal a moment ago
     * may now pull black into view. Re-clamp rather than leaving it stale. */
    m_pan = clampPan(m_pan, effectiveFrameSize());
    update();
    emit zoomChanged();
}

QSize VideoFrameItem::effectiveFrameSize() const
{
    if (!m_pendingImage.isNull())
        return m_pendingImage.size();
    /* Before the first frame the geometry is still known — the camera has one
     * fixed resolution and cannot be asked to change it. */
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
    /* Must mirror computeRects() exactly, roll shrink included — otherwise the
     * pan limits describe a frame of a different size than the one on screen,
     * and the picture can be dragged just far enough to show an edge. */
    if (m_fillMode == PreserveAspectFit)
        sBase *= rollFitFactor(QSizeF(fw * sBase, fh * sBase), bounds);
    const qreal s = sBase * m_zoom;

    const qreal dw = fw * s;
    const qreal dh = fh * s;

    /* Along an axis the frame overflows, the offset may range over exactly the
     * overflow — that keeps the frame's edge from ever coming inside the view.
     * Along an axis it does not fill, there is nothing to pan to, so pin it to
     * centred (offset 0). */
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
    /* Normalise to (-180, 180] so the indicator never has to deal with 720°
     * and "reset to level" is always the shortest way round. */
    while (degrees > 180.0)  degrees -= 360.0;
    while (degrees <= -180.0) degrees += 360.0;

    if (qFuzzyCompare(m_roll, degrees))
        return;
    m_roll = degrees;
    /* The fit scale depends on the roll (see computeRects), so an offset that
     * was legal before may now expose an edge. */
    m_pan = clampPan(m_pan, effectiveFrameSize());
    update();
    emit rollChanged();
}

void VideoFrameItem::resetRoll()
{
    setRoll(0.0);
}

/* How much the frame must shrink so that, once rolled, it still fits the item.
 *
 * A w x h rectangle rotated by θ occupies a bounding box of
 *     w|cosθ| + h|sinθ|   by   w|sinθ| + h|cosθ|
 * so fitting that box inside the item is what keeps the whole picture visible
 * at every angle. Without this, rolling a 4:3 frame by 90° inside a wide
 * landscape window would throw most of it off the sides — the control would
 * technically work and be useless. */
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
    /* Only ever shrink: at 0° this is 1.0 and must not enlarge anything. */
    return qMin(qreal(1.0), f);
}

void VideoFrameItem::onFrameAvailable()
{
    if (!m_camera)
        return;
    /* Take a reference now, on the GUI thread. QImage is copy-on-write, so this
     * is a refcount bump, and it guarantees the render thread sees exactly the
     * frame that triggered this update even if another arrives meanwhile. */
    m_pendingImage = m_camera->currentImage();
    m_textureDirty = true;
    if (!m_hasFrame && !m_pendingImage.isNull()) {
        /* Flip the flag here rather than in updatePaintNode(): this is the GUI
         * thread, so the QML bindings that depend on it are evaluated where
         * they belong instead of on the render thread. */
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

/*
 * THE ZOOM MODEL
 *
 * The camera is 4:3 and the screen is a wide landscape, so at 1:1 the frame is
 * pillarboxed: full height, black bars left and right. That is correct for an
 * overview.
 *
 * The moment you zoom, though, those bars are wasted screen. Zooming must
 * therefore grow the picture into them — out to the left and right edges of the
 * display — and accept that the top and bottom run off the screen. Magnifying
 * *within* the pillarboxed rectangle would keep the bars forever and throw away
 * a third of the display exactly when you need it most.
 *
 * The clean way to express that is to stop thinking in terms of "fit or crop"
 * and think in terms of one scale factor:
 *
 *     s = s_base * zoom
 *
 * where s_base is the scale at which zoom == 1 looks right (fit, for the
 * pillarboxed overview). Draw the frame at that scale, centred, and let it be
 * as big as it likes — then simply show the part of it that lands inside the
 * item.
 *
 * So: build the full drawn rectangle, intersect it with the item, and map that
 * intersection back into texture coordinates. One formula covers everything —
 * pillarboxed at 1:1, edge-to-edge and vertically cropped past that, and
 * cropped on all four sides at high zoom. No clipping node needed, and nothing
 * is ever rendered outside the visible area.
 */
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
        /* Stretch has no aspect to preserve, so the quad is always the whole
         * item and zoom can only narrow the sampled window. */
        const qreal w = fw / m_zoom;
        const qreal h = fh / m_zoom;
        *target = bounds;
        *source = QRectF((fw - w) / 2.0, (fh - h) / 2.0, w, h);
    } else {
        const qreal sFit  = qMin(bounds.width() / fw, bounds.height() / fh);
        const qreal sFill = qMax(bounds.width() / fw, bounds.height() / fh);
        qreal sBase = (m_fillMode == PreserveAspectCrop) ? sFill : sFit;
        /* Fit mode promises the whole frame is visible at zoom 1, and that has
         * to keep holding once the picture is rolled. */
        if (m_fillMode == PreserveAspectFit)
            sBase *= rollFitFactor(QSizeF(fw * sBase, fh * sBase), bounds);
        const qreal s = sBase * m_zoom;

        /* Centred, then shifted by the (already clamped) pan offset. */
        const QPointF pan = clampPan(m_pan, frameSize);
        const QRectF drawn((bounds.width()  - fw * s) / 2.0 + pan.x(),
                           (bounds.height() - fh * s) / 2.0 + pan.y(),
                           fw * s, fh * s);

        const QRectF visible = drawn.intersected(bounds);
        *target = visible;
        /* Map the visible part of the quad back onto the texture. Dividing by
         * the same `s` that produced the quad is what keeps this exact at every
         * zoom level. */
        *source = QRectF((visible.x() - drawn.x()) / s,
                         (visible.y() - drawn.y()) / s,
                         visible.width()  / s,
                         visible.height() / s);
    }

    /* Mirroring is a horizontal flip of the sampled window — free, as opposed
     * to a transform on the node. */
    if (m_mirrored)
        *source = QRectF(source->right(), source->top(),
                         -source->width(), source->height());
}

QSGNode *VideoFrameItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    /* The tree is a transform node (which carries the roll) with the texture
     * node as its only child. Keeping the transform in the scene graph rather
     * than on the QQuickItem leaves the item's own coordinate system — and
     * therefore all the pan/zoom arithmetic — unrotated. */
    QSGTransformNode *root = static_cast<QSGTransformNode *>(oldNode);

    const QImage image = m_pendingImage;
    if (image.isNull() || width() <= 0 || height() <= 0) {
        /* Nothing to show (yet). Drop the node so the placeholder underneath
         * is visible rather than a frozen last frame. */
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
        /* setTexture() does not free the previous one, and the node does not
         * own it by default — so take ownership explicitly and let the node
         * release the old texture for us on the next assignment. */
        node->setOwnsTexture(true);
        node->setTexture(texture);
        m_textureDirty = false;
    }

    QRectF target, source;
    computeRects(image.size(), &target, &source);
    node->setRect(target);
    node->setSourceRect(source);

    /* Roll about the centre of the item, not the centre of the drawn frame:
     * when the picture is panned, the user still expects it to turn around the
     * middle of the screen they are looking at. */
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
