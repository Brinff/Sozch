#pragma once
#include "Entity.h"
#include "PartCatalog.h"
#include "Warehouse.h"
#include "Product.h"
#include <QList>
#include <QString>
#include <QStringList>

// Система обеспечения запасными частями: каталог ЗЧ, склады, обслуживаемые изделия.
//
// SupplySystem владеет ВСЕМИ данными, поэтому только она может проверять правила,
// связывающие разные ветки модели:
//   - склад и изделие ссылаются только на типы ЗЧ, существующие в каталоге;
//   - тип ЗЧ нельзя удалить, пока он есть на складе или в составе изделия.
// Поэтому данные наружу отдаются только для чтения, а все изменения идут через методы этого класса.
// Указатели из find*/groupsOfProduct/partsOfProduct действительны до следующего изменения данных.
class SupplySystem : public Entity {
public:
    // Строка таблицы «Наличие на складах» (копия данных, не зависит от времени жизни списков).
    struct StockLocation {
        int warehouseId = 0;
        QString warehouseName;
        StockItem item;
    };

    explicit SupplySystem(int id = 1) : Entity(id) {}

    // --- чтение ---
    const PartCatalog &catalog() const { return catalog_; }
    const QList<Warehouse> &warehouses() const { return warehouses_; }
    const QList<Product> &products() const { return products_; }
    const Warehouse *findWarehouse(int warehouseId) const;
    const Product *findProduct(int productId) const;

    int nextWarehouseId() const;
    int nextProductId() const;

    // --- каталог ---
    bool addGroup(const PartGroup &group);
    bool removeGroup(int groupId);                          // только пустую группу
    bool addPart(int groupId, const SparePartType &part);
    bool removePart(int partId);                            // false, если тип используется (см. isPartUsed)
    SparePartType *findPart(int partId);                    // правка полей типа (id изменить нельзя)
    bool isPartUsed(int partId) const;                      // есть на складе или в составе изделия

    // --- склады ---
    bool addWarehouse(const Warehouse &warehouse);
    bool removeWarehouse(int warehouseId);
    bool addStock(int warehouseId, const StockItem &item);
    bool updateStock(int warehouseId, const StockItem &item);
    bool removeStock(int warehouseId, int partId);

    // --- изделия ---
    bool addProduct(const Product &product);
    bool removeProduct(int productId);
    bool addComponent(int productId, int partId, int quantity);
    bool setComponentQuantity(int productId, int partId, int quantity);
    bool removeComponent(int productId, int partId);

    // --- запросы для главного экрана ---
    QList<const PartGroup *> groupsOfProduct(int productId) const;                  // группы, из которых в изделии есть ЗЧ
    QList<const SparePartType *> partsOfProduct(int productId, int groupId) const;  // ЗЧ изделия из выбранной группы
    QList<StockLocation> stockLocations(int partId) const;                          // где и сколько лежит тип ЗЧ

    // --- проверка целостности ---
    QStringList validate() const;                           // список ошибок; пустой = всё хорошо
    bool isValid() const { return validate().isEmpty(); }

    // --- файлы ---
    bool loadFromFile(const QString &path, QString *error = nullptr);
    bool saveToFile(const QString &path, QString *error = nullptr) const;

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    int indexOfWarehouse(int warehouseId) const;
    int indexOfProduct(int productId) const;
    bool loadWarehouses(QXmlStreamReader &r);               // читает <warehouses>...</warehouses> в this
    bool loadProducts(QXmlStreamReader &r);                 // читает <products>...</products> в this

    PartCatalog catalog_;
    QList<Warehouse> warehouses_;
    QList<Product> products_;
};
