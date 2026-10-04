#include "PartCatalog.h"
#include <QSet>

int PartCatalog::indexOfGroup(int groupId) const {
    for (int i = 0; i < groups_.size(); ++i){
        if (groups_.at(i).id() == groupId)
            return i;
    }
    return -1;
}

int PartCatalog::indexOfGroupWithPart(int partId) const {
    for (int i = 0; i < groups_.size(); ++i){
        if (groups_.at(i).findPart(partId) != nullptr)
            return i;
    }
    return -1;
}

const PartGroup *PartCatalog::findGroup(int groupId) const {
    const int index = indexOfGroup(groupId);
    if (index == -1)
        return nullptr;
    return &groups_.at(index);
}

const PartGroup *PartCatalog::groupOfPart(int partId) const {
    const int index = indexOfGroupWithPart(partId);
    if (index == -1)
        return nullptr;
    return &groups_.at(index);
}

const SparePartType *PartCatalog::findPart(int partId) const {
    const int index = indexOfGroupWithPart(partId);
    if (index == -1)
        return nullptr;
    return groups_.at(index).findPart(partId);
}

SparePartType *PartCatalog::findPart(int partId) {
    const int index = indexOfGroupWithPart(partId);
    if (index == -1)
        return nullptr;
    return groups_[index].findPart(partId);
}

int PartCatalog::nextGroupId() const {
    int maxId = 0;
    for (int i = 0; i < groups_.size(); ++i){
        if (groups_.at(i).id() > maxId)
            maxId = groups_.at(i).id();
    }
    return maxId + 1;
}

int PartCatalog::nextPartId() const {
    int maxId = 0;
    for (int i = 0; i < groups_.size(); ++i){
        const QList<SparePartType> &parts = groups_.at(i).parts();
        for (int j = 0; j < parts.size(); ++j){
            if (parts.at(j).id() > maxId)
                maxId = parts.at(j).id();
        }
    }
    return maxId + 1;
}

bool PartCatalog::addGroup(const PartGroup &group) {
    if (!group.isValid() ||
        indexOfGroup(group.id()) != -1)
        return false;

    for (int i = 0; i < group.parts().size(); ++i){
        if (indexOfGroupWithPart(group.parts().at(i).id()) != -1)
            return false;
    }

    groups_.append(group);
    return true;
}

bool PartCatalog::removeGroup(int groupId) {
    const int index = indexOfGroup(groupId);
    if (index == -1 || !groups_.at(index).parts().isEmpty())
        return false;

    groups_.removeAt(index);
    return true;
}

bool PartCatalog::addPart(int groupId, const SparePartType &part) {
    const int index = indexOfGroup(groupId);
    if (index == -1 ||
        !part.isValid() ||
        indexOfGroupWithPart(part.id()) != -1)
        return false;

    return groups_[index].addPart(part);
}

bool PartCatalog::removePart(int partId) {
    const int index = indexOfGroupWithPart(partId);
    if (index == -1)
        return false;

    return groups_[index].removePart(partId);
}

bool PartCatalog::isValid() const {
    QSet<int> groupIds;
    QSet<int> partIds;

    for (int i = 0; i < groups_.size(); ++i){
        const PartGroup &group = groups_.at(i);
        if (!group.isValid() || groupIds.contains(group.id()))
            return false;
        groupIds.insert(group.id());

        for (int j = 0; j < group.parts().size(); ++j){
            const int partId = group.parts().at(j).id();
            if (partIds.contains(partId))
                return false;
            partIds.insert(partId);
        }
    }
    return true;
}

void PartCatalog::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("catalog");
    for (int i = 0; i < groups_.size(); ++i){
        groups_.at(i).saveXml(w);
    }
    w.writeEndElement();
}

void PartCatalog::loadXml(QXmlStreamReader &r) {
    PartCatalog temp;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("group")) {
            PartGroup g;
            g.loadXml(r);
            if (r.hasError())
                return;

            if (temp.indexOfGroup(g.id()) != -1){
                r.raiseError(QString("Группа с id=%1 уже есть в каталоге").arg(g.id()));
                return;
            }
            if (!temp.addGroup(g)){
                r.raiseError(QString("Типы ЗЧ группы id=%1 повторяют id типов из других групп").arg(g.id()));
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