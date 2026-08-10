/*
 * frameoverlay.h — burn a timestamp into a frame.
 *
 * Shared by the snapshot path (CaptureStore) and the recording path
 * (MjpegRecorder) so a photo and a video taken seconds apart cannot end up with
 * differently drawn stamps.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef FRAMEOVERLAY_H
#define FRAMEOVERLAY_H

#include <QtGlobal>

class QImage;
class QString;

namespace overlay {

/* Return `src` rotated by `degrees`, in a canvas of the SAME size, scaled so
 * the whole rotated picture still fits inside it. Areas the picture no longer
 * covers are black. Returns `src` untouched for an angle of ~0.
 *
 * Same size on purpose: the video track's width and height are fixed when
 * recording starts, so frames cannot change dimensions part-way through a
 * rotation without corrupting the file.
 *
 * Fit rather than crop, also on purpose. Cropping to fill would look better —
 * no black corners — but it would silently throw away the edges of an
 * inspection frame, and the edge of the picture is exactly where a crack
 * disappearing out of view tends to be. Losing evidence to make the video
 * prettier is the wrong trade here. It also matches what the viewfinder does,
 * so the recording holds what the screen showed. */
QImage rotateFit(const QImage &src, qreal degrees);

/* Draw `text` into the bottom-right corner of `image`, in place.
 *
 * Outlined rather than boxed: a solid label would hide part of the picture, and
 * inside a pipe the corner is often the darkest area, where plain white text
 * would still be readable but plain dark text would not. An outline is legible
 * over both. Amber, so it cannot be mistaken for something the camera saw.
 *
 * The size scales with the frame rather than being a fixed pixel count, so this
 * keeps working if a camera in this family ever reports a different resolution.
 *
 * No-op when the image is null or the text empty. */
void drawTimestamp(QImage *image, const QString &text);

} // namespace overlay

#endif // FRAMEOVERLAY_H
