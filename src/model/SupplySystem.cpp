#include "SupplySystem.h"
#include <QFile>
#include <QSaveFile>
#include <QSet>

namespace {
// Текст ошибки разбора с позицией в файле.
QString describeError(const QXmlStreamReader &r) {
    return QString("%1 (строка %2, столбец %3)")
            .arg(r.errorString())
            .arg(r.lineNumber())
            .arg(r.columnNumber());
}
}

// ------------------------------------------------------------------ поиск

int SupplySystem::indexOfWarehouse(int warehouseId) const {
    for (int i = 0; i < warehouses_.size(); ++i){
        if (warehouses_.at(i).id() == warehouseId)
            return i;
    }
    return -1;
}

int SupplySystem::indexOfProduct(int productId) const {
    for (int i = 0; i < products_.size(); ++i){
        if (products_.at(i).id() == productId)
            return i;
    }
    return -1;
}

const Warehouse *SupplySystem::findWarehouse(int warehouseId) const {
    const int index = indexOfWarehouse(warehouseId);
    if (index == -1)
        return nullptr;
    return &warehouses_.at(index);
}

const Product *SupplySystem::findProduct(int productId) const {
    const int index = indexOfProduct(productId);
    if (index == -1)
        return nullptr;
    return &products_.at(index);
}

int SupplySystem::nextWarehouseId() const {
    int maxId = 0;
    for (int i = 0; i < warehouses_.size(); ++i){
        if (warehouses_.at(i).id() > maxId)
            maxId = warehouses_.at(i).id();
    }
    return maxId + 1;
}

int SupplySystem::nextProductId() const {
    int maxId = 0;
    for (int i = 0; i < products_.size(); ++i){
        if (products_.at(i).id() > maxId)
            maxId = products_.at(i).id();
    }
    return maxId + 1;
}

// ------------------------------------------------------------------ каталог

bool SupplySystem::addGroup(const PartGroup &group) {
    return catalog_.addGroup(group);
}

bool SupplySystem::removeGroup(int groupId) {
    // Группа удаляется только пустой, а на пустую группу никто сослаться не может.
    return catalog_.removeGroup(groupId);
}

bool SupplySystem::addPart(int groupId, const SparePartType &part) {
    return catalog_.addPart(groupId, part);
}

bool SupplySystem::isPartUsed(int partId) const {
    for (int i = 0; i < warehouses_.size(); ++i){
        if (warehouses_.at(i).findStock(partId) != nullptr)
            return true;
    }
    for (int i = 0; i < products_.size(); ++i){
        if (products_.at(i).hasPart(partId))
            return true;
    }
    return false;
}

bool SupplySystem::removePart(int partId) {
    if (isPartUsed(partId))
        return false;

    return catalog_.removePart(partId);
}

SparePartType *SupplySystem::findPart(int partId) {
    return catalog_.findPart(partId);
}

// ------------------------------------------------------------------ склады

bool SupplySystem::addWarehouse(const Warehouse &warehouse) {
    if (!warehouse.isValid() || indexOfWarehouse(warehouse.id()) != -1)
        return false;

    for (int i = 0; i < warehouse.stock().size(); ++i){
        if (catalog_.findPart(warehouse.stock().at(i).partId()) == nullptr)
            return false;
    }

    warehouses_.append(warehouse);
    return true;
}

bool SupplySystem::removeWarehouse(int warehouseId) {
    const int index = indexOfWarehouse(warehouseId);
    if (index == -1)
        return false;

    warehouses_.removeAt(index);
    return true;
}

bool SupplySystem::addStock(int warehouseId, const StockItem &item) {
    const int index = indexOfWarehouse(warehouseId);
    if (index == -1 || catalog_.findPart(item.partId()) == nullptr)
        return false;

    return warehouses_[index].addStock(item);
}

bool SupplySystem::updateStock(int warehouseId, const StockItem &item) {
    const int index = indexOfWarehouse(warehouseId);
    if (index == -1)
        return false;

    return warehouses_[index].updateStock(item);
}

bool SupplySystem::removeStock(int warehouseId, int partId) {
    const int index = indexOfWarehouse(warehouseId);
    if (index == -1)
        return false;

    return warehouses_[index].removeStock(partId);
}

// ------------------------------------------------------------------ изделия

bool SupplySystem::addProduct(const Product &product) {
    if (!product.isValid() || indexOfProduct(product.id()) != -1)
        return false;

    for (auto it = product.components().constBegin(); it != product.components().constEnd(); ++it){
        if (catalog_.findPart(it.key()) == nullptr)
            return false;
    }

    products_.append(product);
    return true;
}

bool SupplySystem::removeProduct(int productId) {
    const int index = indexOfProduct(productId);
    if (index == -1)
        return false;

    products_.removeAt(index);
    return true;
}

bool SupplySystem::addComponent(int productId, int partId, int quantity) {
    const int index = indexOfProduct(productId);
    if (index == -1 || catalog_.findPart(partId) == nullptr)
        return false;

    return products_[index].addComponent(partId, quantity);
}

bool SupplySystem::setComponentQuantity(int productId, int partId, int quantity) {
    const int index = indexOfProduct(productId);
    if (index == -1)
        return false;

    return products_[index].setQuantity(partId, quantity);
}

bool SupplySystem::removeComponent(int productId, int partId) {
    const int index = indexOfProduct(productId);
    if (index == -1)
        return false;

    return products_[index].removeComponent(partId);
}

// ------------------------------------------------------------------ запросы для главного экрана

QList<const PartGroup *> SupplySystem::groupsOfProduct(int productId) const {
    QList<const PartGroup *> result;
    const Product *product = findProduct(productId);
    if (product == nullptr)
        return result;

    for (int i = 0; i < catalog_.groups().size(); ++i){
        const PartGroup &group = catalog_.groups().at(i);
        for (int j = 0; j < group.parts().size(); ++j){
            if (product->hasPart(group.parts().at(j).id())){
                result.append(&group);
                break;                      // группу достаточно добавить один раз
            }
        }
    }
    return result;
}

QList<const SparePartType *> SupplySystem::partsOfProduct(int productId, int groupId) const {
    QList<const SparePartType *> result;
    const Product *product = findProduct(productId);
    const PartGroup *group = catalog_.findGroup(groupId);
    if (product == nullptr || group == nullptr)
        return result;

    for (int i = 0; i < group->parts().size(); ++i){
        if (product->hasPart(group->parts().at(i).id()))
            result.append(&group->parts().at(i));
    }
    return result;
}

QList<SupplySystem::StockLocation> SupplySystem::stockLocations(int partId) const {
    QList<StockLocation> result;
    for (int i = 0; i < warehouses_.size(); ++i){
        const StockItem *item = warehouses_.at(i).findStock(partId);
        if (item != nullptr){
            StockLocation location;
            location.warehouseId = warehouses_.at(i).id();
            location.warehouseName = warehouses_.at(i).name();
            location.item = *item;
            result.append(location);
        }
    }
    return result;
}

// ------------------------------------------------------------------ проверка целостности

QStringList SupplySystem::validate() const {
    QStringList errors;

    if (id() <= 0)
        errors << QString("Некорректный id системы: %1").arg(id());

    if (!catalog_.isValid())
        errors << "Каталог ЗЧ содержит некорректные или повторяющиеся данные";

    QSet<int> warehouseIds;
    for (int i = 0; i < warehouses_.size(); ++i){
        const Warehouse &w = warehouses_.at(i);
        if (!w.isValid())
            errors << QString("Склад id=%1 содержит некорректные данные").arg(w.id());
        if (warehouseIds.contains(w.id()))
            errors << QString("Повторяющийся id склада: %1").arg(w.id());
        warehouseIds.insert(w.id());

        for (int j = 0; j < w.stock().size(); ++j){
            const int partId = w.stock().at(j).partId();
            if (catalog_.findPart(partId) == nullptr)
                errors << QString("Склад «%1»: тип ЗЧ id=%2 отсутствует в каталоге").arg(w.name()).arg(partId);
        }
    }

    QSet<int> productIds;
    for (int i = 0; i < products_.size(); ++i){
        const Product &p = products_.at(i);
        if (!p.isValid())
            errors << QString("Изделие id=%1 содержит некорректные данные").arg(p.id());
        if (productIds.contains(p.id()))
            errors << QString("Повторяющийся id изделия: %1").arg(p.id());
        productIds.insert(p.id());

        for (auto it = p.components().constBegin(); it != p.components().constEnd(); ++it){
            if (catalog_.findPart(it.key()) == nullptr)
                errors << QString("Изделие «%1»: тип ЗЧ id=%2 отсутствует в каталоге").arg(p.name()).arg(it.key());
        }
    }

    return errors;
}

// ------------------------------------------------------------------ XML

void SupplySystem::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("supplySystem");
    saveId(w);

    catalog_.saveXml(w);

    w.writeStartElement("warehouses");
    for (int i = 0; i < warehouses_.size(); ++i){
        warehouses_.at(i).saveXml(w);
    }
    w.writeEndElement();

    w.writeStartElement("products");
    for (int i = 0; i < products_.size(); ++i){
        products_.at(i).saveXml(w);
    }
    w.writeEndElement();

    w.writeEndElement();
}

bool SupplySystem::loadWarehouses(QXmlStreamReader &r) {
    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("warehouse")) {
            Warehouse w;
            w.loadXml(r);
            if (r.hasError())
                return false;
            if (indexOfWarehouse(w.id()) != -1){
                r.raiseError(QString("Склад с id=%1 уже есть в файле").arg(w.id()));
                return false;
            }
            warehouses_.append(w);      // ссылки на каталог проверим в конце: порядок секций в файле не важен
        }
        else {
            r.skipCurrentElement();
        }
    }
    return !r.hasError();
}

bool SupplySystem::loadProducts(QXmlStreamReader &r) {
    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("product")) {
            Product p;
            p.loadXml(r);
            if (r.hasError())
                return false;
            if (indexOfProduct(p.id()) != -1){
                r.raiseError(QString("Изделие с id=%1 уже есть в файле").arg(p.id()));
                return false;
            }
            products_.append(p);
        }
        else {
            r.skipCurrentElement();
        }
    }
    return !r.hasError();
}

void SupplySystem::loadXml(QXmlStreamReader &r) {
    SupplySystem temp;

    if (!temp.loadId(r))
        return;

    bool haveCatalog = false;
    bool haveWarehouses = false;
    bool haveProducts = false;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("catalog")) {
            if (haveCatalog){
                r.raiseError("Элемент catalog встречается более одного раза");
                return;
            }
            temp.catalog_.loadXml(r);
            if (r.hasError())
                return;
            haveCatalog = true;
        }
        else if (r.name() == QLatin1String("warehouses")) {
            if (haveWarehouses){
                r.raiseError("Элемент warehouses встречается более одного раза");
                return;
            }
            if (!temp.loadWarehouses(r))
                return;
            haveWarehouses = true;
        }
        else if (r.name() == QLatin1String("products")) {
            if (haveProducts){
                r.raiseError("Элемент products встречается более одного раза");
                return;
            }
            if (!temp.loadProducts(r))
                return;
            haveProducts = true;
        }
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError())
        return;

    // Все секции прочитаны: теперь можно проверить ссылки между ними.
    const QStringList errors = temp.validate();
    if (!errors.isEmpty()){
        r.raiseError(errors.join("; "));
        return;
    }

    *this = temp;
}

// ------------------------------------------------------------------ файлы

bool SupplySystem::loadFromFile(const QString &path, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)){
        if (error)
            *error = QString("Не удалось открыть файл «%1»: %2").arg(path, file.errorString());
        return false;
    }

    QXmlStreamReader r(&file);

    if (!r.readNextStartElement()){
        if (error)
            *error = r.hasError() ? describeError(r) : QString("Файл не содержит XML-данных");
        return false;
    }
    if (r.name() != QLatin1String("supplySystem")){
        if (error)
            *error = QString("Корневой элемент должен называться supplySystem, а не %1").arg(r.name().toString());
        return false;
    }

    SupplySystem temp;
    temp.loadXml(r);
    if (r.hasError()){
        if (error)
            *error = describeError(r);
        return false;
    }

    *this = temp;
    return true;
}

bool SupplySystem::saveToFile(const QString &path, QString *error) const {
    QSaveFile file(path);       // пишет во временный файл и заменяет целевой только при commit()
    if (!file.open(QIODevice::WriteOnly)){
        if (error)
            *error = QString("Не удалось открыть файл «%1» для записи: %2").arg(path, file.errorString());
        return false;
    }

    QXmlStreamWriter w(&file);
    w.setAutoFormatting(true);
    w.setAutoFormattingIndent(2);
    w.writeStartDocument();
    saveXml(w);
    w.writeEndDocument();

    if (w.hasError() || !file.commit()){
        if (error)
            *error = QString("Не удалось записать файл «%1»: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}
