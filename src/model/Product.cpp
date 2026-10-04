#include "Product.h"
#include "XmlUtil.h"

bool Product::addComponent(int partId, int quantity) {
    if (partId <= 0 || quantity <= 0 || components_.contains(partId))
        return false;

    components_.insert(partId, quantity);
    return true;
}

bool Product::setQuantity(int partId, int quantity) {
    if (quantity <= 0 || !components_.contains(partId))
        return false;

    components_[partId] = quantity;
    return true;
}

bool Product::removeComponent(int partId) {
    return components_.remove(partId) > 0;
}

bool Product::isValid() const {
    if (id() <= 0 || name().isEmpty())
        return false;

    for (auto it = components_.constBegin(); it != components_.constEnd(); ++it){
        if (it.key() <= 0 || it.value() <= 0)
            return false;
    }
    return true;
}

void Product::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("product");
    saveId(w);
    saveName(w);
    for (auto it = components_.constBegin(); it != components_.constEnd(); ++it){
        w.writeStartElement("component");
        w.writeAttribute("partId", QString::number(it.key()));
        w.writeAttribute("quantity", QString::number(it.value()));
        w.writeEndElement();
    }
    w.writeEndElement();
}

void Product::loadXml(QXmlStreamReader &r) {
    Product temp;

    if (!temp.loadId(r) || !temp.loadName(r))
        return;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("component")) {
            int partId = 0;
            int quantity = 0;

            if (!XmlUtil::readInt(r, "partId", partId) ||
                !XmlUtil::readInt(r, "quantity", quantity))
                return;

            if (temp.hasPart(partId)){
                r.raiseError(QString("Тип ЗЧ с id=%1 встречается в составе изделия более одного раза").arg(partId));
                return;
            }
            if (!temp.addComponent(partId, quantity)){
                r.raiseError(QString("Некорректный состав изделия (partId=%1, quantity=%2)").arg(partId).arg(quantity));
                return;
            }

            r.skipCurrentElement();     // у <component> нет детей: встать на его закрывающий тег
        }
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError())
        return;

    *this = temp;
}
