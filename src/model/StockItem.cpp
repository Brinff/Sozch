#include "StockItem.h"
#include "XmlUtil.h"

void StockItem::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("stock");
    w.writeAttribute("partId", QString::number(partId_));
    w.writeAttribute("quantity", QString::number(quantity_));
    w.writeAttribute("price", QString::number(price_, 'f', 2));
    w.writeAttribute("deliveryDate", deliveryDate_.toString(Qt::ISODate));
    w.writeEndElement();
}

void StockItem::loadXml(QXmlStreamReader &r) {
    int partId = 0;
    int quantity = 0;
    double price = 0;
    QDate deliveryDate = QDate();

    if (!XmlUtil::readInt(r, "partId", partId) ||
        !XmlUtil::readInt(r, "quantity", quantity) ||
        !XmlUtil::readDouble(r, "price", price) ||
        !XmlUtil::readDate(r, "deliveryDate", deliveryDate))
        return;

    StockItem temp(partId, quantity, price, deliveryDate);

    if (!temp.isValid()){
        r.raiseError(QString("Некорректные значения позиции (partId=%1, quantity=%2, price=%3, deliveryDate=%4)")
                    .arg(partId).arg(quantity).arg(price, 0, 'f', 2).arg(deliveryDate.toString(Qt::ISODate)));
        return;
    }

    *this = temp;

    r.skipCurrentElement();
}