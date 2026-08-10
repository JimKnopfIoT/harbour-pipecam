/*
 * frameoverlay.cpp — see frameoverlay.h.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "frameoverlay.h"

#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QString>
#include <QtMath>

namespace overlay {

QImage rotateFit(const QImage &src, qreal degrees)
{
    if (src.isNull() || qFuzzyIsNull(degrees))
        return src;

    const qreal rad = qDegreesToRadians(degrees);
    const qreal c = qAbs(qCos(rad));
    const qreal s = qAbs(qSin(rad));

    const qreal w = src.width();
    const qreal h = src.height();
    /* Bounding box of the rotated rectangle. */
    const qreal bw = w * c + h * s;
    const qreal bh = w * s + h * c;
    const qreal scale = qMin(w / bw, h / bh);

    QImage out(src.size(), QImage::Format_RGB32);
    out.fill(Qt::black);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    /* Rotate about the centre of the canvas, then draw the source centred on
     * the origin — the order matters, and doing it with translate/rotate rather
     * than a hand-built QTransform keeps it readable. */
    p.translate(w / 2.0, h / 2.0);
    p.rotate(degrees);
    p.scale(scale, scale);
    p.drawImage(QPointF(-w / 2.0, -h / 2.0), src);
    p.end();

    return out;
}

void drawTimestamp(QImage *image, const QString &text)
{
    if (!image || image->isNull() || text.isEmpty())
        return;

    QPainter p(image);
    p.setRenderHint(QPainter::Antialiasing, true);

    QFont font = p.font();
    font.setPixelSize(qMax(11, image->height() / 22));
    font.setBold(true);
    p.setFont(font);

    const QFontMetrics fm(font);
    const int margin = qMax(6, image->height() / 48);
    const QRect box = fm.boundingRect(text);
    const int x = image->width() - box.width() - margin;
    const int y = image->height() - margin;

    /* Cheap outline: the text offset in every direction in black, then the
     * amber on top. Eight extra passes at this size cost nothing and avoid
     * pulling in QPainterPath just to stroke some glyphs. */
    p.setPen(QColor(0, 0, 0, 220));
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx || dy)
                p.drawText(x + dx, y + dy, text);

    p.setPen(QColor(0xFF, 0xD2, 0x5A));
    p.drawText(x, y, text);
}

} // namespace overlay
