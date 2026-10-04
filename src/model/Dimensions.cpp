#include "Dimensions.h"
#include "XmlUtil.h"

void Dimensions::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("dimensions");
    w.writeAttribute("length", QString::number(length_, 'f', 3));
    w.writeAttribute("width", QString::number(width_, 'f', 3));
    w.writeAttribute("height", QString::number(height_, 'f', 3));
    w.writeEndElement();
}

void Dimensions::loadXml(QXmlStreamReader &r) {
    double l = 0, w = 0, h = 0;

    if (!XmlUtil::readDouble(r, "length", l) ||
        !XmlUtil::readDouble(r, "width", w) ||
        !XmlUtil::readDouble(r, "height", h))
        return;

    Dimensions temp(l, w, h);

    if (!temp.isValid()){
        r.raiseError(QString("Габариты должны быть больше нуля (length=%1, width=%2, height=%3)")
                    .arg(l).arg(w).arg(h));
        return;
    }

    *this = temp;

    r.skipCurrentElement();
}