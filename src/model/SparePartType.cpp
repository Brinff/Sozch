#include "SparePartType.h"
#include "XmlUtil.h"

void SparePartType::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("part");
    saveId(w);
    saveName(w);
    w.writeAttribute("mass", QString::number(mass_, 'f', 3));
    w.writeAttribute("price", QString::number(price_, 'f', 2));
    dimensions_.saveXml(w);
    w.writeEndElement();
}

void SparePartType::loadXml(QXmlStreamReader &r) {
    SparePartType tempSPT;
    Dimensions tempDim;
    double mass = 0;
    double price = 0;
    bool haveDimensions = false;

    if (!tempSPT.loadId(r) || !tempSPT.loadName(r))
        return;

    if (!XmlUtil::readDouble(r, "mass", mass) || !XmlUtil::readDouble(r, "price", price))
        return;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("dimensions")) {
            if (haveDimensions){
                r.raiseError("Элемент dimensions встречается более одного раза");
                return;
            }
            tempDim.loadXml(r);
            if (r.hasError())
                return;
            haveDimensions = true;
        } 
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError())
        return;

    if (!haveDimensions) {
        r.raiseError("У типа ЗЧ нет элемента dimensions");
        return;
    }

    tempSPT.setMass(mass);
    tempSPT.setPrice(price);
    tempSPT.setDimensions(tempDim);

    if (!tempSPT.isValid()){
        r.raiseError(QString("Некорректные значения типа детали: (id=%1, name=%2, mass=%3, price=%4, (length=%5, width=%6, height=%7))")
            .arg(tempSPT.id()).arg(tempSPT.name()).arg(tempSPT.mass()).arg(tempSPT.price()).arg(tempDim.length()).arg(tempDim.width()).arg(tempDim.height()));
        return;
    }

    *this = tempSPT;
}