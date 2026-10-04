#pragma once
#include "NamedEntity.h"
#include <QMap>

// Изделие: состав = тип ЗЧ -> количество.
// Условие «изделие не содержит двух однотипных ЗЧ» обеспечено самой структурой:
// в QMap один ключ (partId) не может встретиться дважды.
// Количество каждого типа не меньше 1.
class Product : public NamedEntity {
public:
    explicit Product(int id = 0, const QString &name = QString()) : NamedEntity(id, name) {}

    const QMap<int, int> &components() const { return components_; }   // partId -> quantity, по возрастанию partId
    bool hasPart(int partId) const { return components_.contains(partId); }
    int quantityOf(int partId) const { return components_.value(partId, 0); }   // 0, если типа нет в составе

    bool addComponent(int partId, int quantity);        // false: уже есть или partId <= 0 или quantity <= 0
    bool setQuantity(int partId, int quantity);         // false: нет такого или quantity <= 0
    bool removeComponent(int partId);                   // false: нет такого

    bool isValid() const;

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    QMap<int, int> components_;
};
