/*
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef FRAMEOVERLAY_H
#define FRAMEOVERLAY_H

#include <QtGlobal>

class QImage;
class QString;

namespace overlay {

/* Same-size canvas, scaled to fit, black corners; src unchanged at ~0.
 * Size must not change: video track dimensions are fixed at start. */
QImage rotateFit(const QImage &src, qreal degrees);

/* Bottom-right, in place. No-op for null image or empty text. */
void drawTimestamp(QImage *image, const QString &text);

} // namespace overlay

#endif // FRAMEOVERLAY_H
