#include "PartGroup.h"
#include <QSet>

int PartGroup::indexOfPart(int partId) const {
    for (int i = 0; i < parts_.size(); ++i){
        if (parts_.at(i).id() == partId)
            return i;
    }
    return -1;
}

const SparePartType *PartGroup::findPart(int partId) const {
    int index = indexOfPart(partId);
    if (index == -1)
        return nullptr;
    return &parts_.at(index);
}

SparePartType *PartGroup::findPart(int partId) {
    int index = indexOfPart(partId);
    if (index == -1)
        return nullptr;
    return &parts_[index];
}

bool PartGroup::addPart(const SparePartType &part) {
    if (!part.isValid() || indexOfPart(part.id()) != -1)
        return false;
    parts_.append(part);
    return true;
}

bool PartGroup::removePart(int partId) {
    int index = indexOfPart(partId);
    if (index == -1)
        return false;
    parts_.removeAt(index);
    return true;
}

bool PartGroup::isValid() const {
    if (id() <= 0 || name().isEmpty())
        return false;

    QSet<int> seen;
    for (int i = 0; i < parts_.size(); ++i){
        const SparePartType &part = parts_.at(i);
        if (!part.isValid() || seen.contains(part.id()))
            return false;
        seen.insert(part.id());
    }
    return true;
}

void PartGroup::saveXml(QXmlStreamWriter &w) const {
    w.writeStartElement("group");
    saveId(w);
    saveName(w);
    for (int i = 0; i < parts_.size(); ++i){
        parts_.at(i).saveXml(w);
    }
    w.writeEndElement();
}

void PartGroup::loadXml(QXmlStreamReader &r) {
    PartGroup temp;

    if (!temp.loadId(r) || !temp.loadName(r))
        return;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("part")) {
            SparePartType p; 
            p.loadXml(r);
            if (r.hasError())
                return;
            if (!temp.addPart(p)){
                r.raiseError(QString("Тип ЗЧ с id=%1 уже есть в группе")
                    .arg(p.id()));
                return;
            }
        } 
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError())
        return;

    if (!temp.isValid()){
        r.raiseError(QString("Некорректные значения группы: (id=%1, name=%2)")
            .arg(temp.id()).arg(temp.name()));
        return;
    }

    *this = temp;
}