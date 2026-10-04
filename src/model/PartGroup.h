#pragma once
#include "NamedEntity.h"
#include "SparePartType.h"
#include <QList>

class PartGroup : public NamedEntity {
public:
    explicit PartGroup(int id = 0, const QString &name = QString()) : NamedEntity(id, name) {}

    const QList<SparePartType> &parts() const { return parts_; }
    const SparePartType *findPart(int partId) const;
    SparePartType *findPart(int partId);

    bool addPart(const SparePartType &part);
    bool removePart(int partId);
    
    bool isValid() const;

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;
    
private:
    int indexOfPart(int partId) const;
    QList<SparePartType> parts_;
};