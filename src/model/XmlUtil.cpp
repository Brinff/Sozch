#include "XmlUtil.h"

namespace {
    bool getAttr(QXmlStreamReader &r, const QString &name, QString &text) {
        const QXmlStreamAttributes attrs = r.attributes();
        if (!attrs.hasAttribute(name)) {
            r.raiseError("Отсутствует атрибут " + name);
            return false;
        }
        text = attrs.value(name).toString();
        return true;
    }

    bool badValue(QXmlStreamReader &r, const QString &name, const QString &text) {
        r.raiseError("Некорректное значение атрибута " + name + ": " + text);
        return false;
    }
}

bool XmlUtil::readInt(QXmlStreamReader &r, const QString &name, int &out){
    QString text;
    if (!getAttr(r, name, text)) return false;

    bool ok = false;
    const int value = text.toInt(&ok);
    if (!ok) return badValue(r, name, text);

    out = value;
    return true;
}

bool XmlUtil::readDouble(QXmlStreamReader &r, const QString &name, double &out){
    QString text;
    if (!getAttr(r, name, text)) return false;

    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok || !qIsFinite(value)) return badValue(r, name, text);

    out = value;
    return true;
}

bool XmlUtil::readDate(QXmlStreamReader &r, const QString &name, QDate &out){
    QString text;
    if (!getAttr(r, name, text)) return false;

    const QDate value = QDate::fromString(text, Qt::ISODate);
    if (!value.isValid()) return badValue(r, name, text);

    out = value;
    return true;
}

bool XmlUtil::readString(QXmlStreamReader &r, const QString &name, QString &out){
    QString text;
    if (!getAttr(r, name, text)) return false;

    out = text.trimmed();
    return true;
}