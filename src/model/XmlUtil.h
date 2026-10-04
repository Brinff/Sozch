#pragma once
#include <QXmlStreamReader>
#include <QDate>

namespace XmlUtil {
    bool readInt(QXmlStreamReader &r, const QString &name, int    &out);
    bool readDouble(QXmlStreamReader &r, const QString &name, double &out);
    bool readDate(QXmlStreamReader &r, const QString &name, QDate  &out);
    bool readString(QXmlStreamReader &r, const QString &name, QString &out);
}