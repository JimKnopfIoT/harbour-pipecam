/*
 * usertext.h — a message that is both shown to the user and logged.
 *
 * The UI wants the translation, the log wants English: a German log line in a
 * bug report is useless to most readers and cannot be searched for in the
 * source. So such a message carries both, built from the same source string.
 *
 * lupdate does not expand macros, so every call site marks its string itself:
 *
 *     UserText("Ctx", QT_TRANSLATE_NOOP("Ctx", "No camera %1.")).arg(n)
 *
 * The context must match the class whose translations are meant — that keeps
 * the existing entries in translations/*.ts valid.
 */
#ifndef PIPECAM_USERTEXT_H
#define PIPECAM_USERTEXT_H

#include <QCoreApplication>
#include <QString>

struct UserText
{
    QString ui;     /* translated, for the screen */
    QString log;    /* English source, for the log */

    UserText() {}
    UserText(const char *context, const char *source)
        : ui(QCoreApplication::translate(context, source))
        , log(QString::fromUtf8(source)) {}

    UserText arg(int a) const { return both(ui.arg(a), log.arg(a)); }
    UserText arg(const QString &a) const { return both(ui.arg(a), log.arg(a)); }

    /* Appended text is not translated: error codes, paths, library messages. */
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
