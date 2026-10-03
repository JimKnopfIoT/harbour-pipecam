/*
 * Translated text for the UI, English source for the log.
 * lupdate does not expand macros: mark strings with QT_TRANSLATE_NOOP at the
 * call site; context must match the .ts context:
 *     UserText("Ctx", QT_TRANSLATE_NOOP("Ctx", "No camera %1.")).arg(n)
 */
#ifndef PIPECAM_USERTEXT_H
#define PIPECAM_USERTEXT_H

#include <QCoreApplication>
#include <QString>

struct UserText
{
    QString ui;     /* translated */
    QString log;    /* English source */

    UserText() {}
    UserText(const char *context, const char *source)
        : ui(QCoreApplication::translate(context, source))
        , log(QString::fromUtf8(source)) {}

    UserText arg(int a) const { return both(ui.arg(a), log.arg(a)); }
    UserText arg(const QString &a) const { return both(ui.arg(a), log.arg(a)); }

    /* appended text is not translated */
    UserText &operator+=(const QString &s) { ui += s; log += s; return *this; }
    UserText operator+(const QString &s) const { return both(ui + s, log + s); }

    bool isEmpty() const { return log.isEmpty(); }

private:
    static UserText both(const QString &u, const QString &l)
    {
        UserText t;
        t.ui = u;
        t.log = l;
        return t;
    }
};

#endif
