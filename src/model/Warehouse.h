#pragma once
#include "NamedEntity.h"
#include "StockItem.h"
#include <QList>

// Склад: перечень позиций StockItem (не более одной позиции на тип ЗЧ).
// Три операции соответствуют трём кнопкам в окне склада: добавить / редактировать / удалить.
// Неконстантного доступа к позиции нет намеренно: у StockItem partId можно менять,
// а через изменение partId легко получить дубликат. Правка идёт через updateStock().
class Warehouse : public NamedEntity {
public:
    explicit Warehouse(int id = 0, const QString &name = QString()) : NamedEntity(id, name) {}

    const QList<StockItem> &stock() const { return stock_; }
    const StockItem *findStock(int partId) const;       // nullptr, если нет (до следующего изменения списка)

    bool addStock(const StockItem &item);               // false: невалидна или такой partId уже есть
    bool updateStock(const StockItem &item);            // заменяет позицию с тем же partId; false: нет такой/невалидна
    bool removeStock(int partId);                       // false: нет такой

    bool isValid() const;

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    int indexOfStock(int partId) const;                 // -1, если нет
    QList<StockItem> stock_;
};
