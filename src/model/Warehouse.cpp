#include "Warehouse.h"
#include <QSet>

int Warehouse::indexOfStock(int partId) const {
    for (int i = 0; i < stock_.size(); ++i){
        if (stock_.at(i).partId() == partId)
            return i;
    }
    return -1;
}

const StockItem *Warehouse::findStock(int partId) const {
    const int index = indexOfStock(partId);
    if (index == -1)
        return nullptr;
    return &stock_.at(index);
}

bool Warehouse::addStock(const StockItem &item) {
    if (!item.isValid() || indexOfStock(item.partId()) != -1)
        return false;

    stock_.append(item);
    return true;
}

bool Warehouse::updateStock(const StockItem &item) {
    const int index = indexOfStock(item.partId());
    if (index == -1 || !item.isValid())
        return false;

    stock_[index] = item;
    return true;
}

bool Warehouse::removeStock(int partId) {
    const int index = indexOfStock(partId);
    if (index == -1)
        return false;

    stock_.removeAt(index);
    return true;
}

bool Warehouse::isValid() const {
    if (id() <= 0 || name().isEmpty())
        return false;

    QSet<int> seen;
    for (int i = 0; i < stock_.size(); ++i){
        const StockItem &item = stock_.at(i);
        if (!item.isValid() || seen.contains(item.partId()))
            return false;
        seen.insert(item.partId());
    }
    return true;
}

void Warehouse::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("warehouse");
    saveId(w);
    saveName(w);
    for (int i = 0; i < stock_.size(); ++i){
        stock_.at(i).saveXml(w);
    }
    w.writeEndElement();
}

void Warehouse::loadXml(QXmlStreamReader &r) {
    Warehouse temp;

    if (!temp.loadId(r) || !temp.loadName(r))
        return;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("stock")) {
            StockItem item;
            item.loadXml(r);
            if (r.hasError())
                return;
            if (!temp.addStock(item)){
                r.raiseError(QString("Тип ЗЧ с id=%1 уже есть на складе").arg(item.partId()));
                return;
            }
        }
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError())
        return;

    *this = temp;
}
