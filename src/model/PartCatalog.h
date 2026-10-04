#pragma once
#include "XmlSerializable.h"
#include "PartGroup.h"
#include <QList>

class PartCatalog : public XmlSerializable {
public:
    const QList<PartGroup> &groups() const { return groups_; }
    const PartGroup *findGroup(int groupId) const;
    const PartGroup *groupOfPart(int partId) const;
    const SparePartType *findPart(int partId) const;
    SparePartType *findPart(int partId);

    int nextGroupId() const;
    int nextPartId() const;

    bool addGroup(const PartGroup &group);
    bool removeGroup(int groupId);
    bool addPart(int groupId, const SparePartType &part);
    bool removePart(int partId);

    bool isValid() const;

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    int indexOfGroup(int groupId) const;
    int indexOfGroupWithPart(int partId) const;
    QList<PartGroup> groups_;
};