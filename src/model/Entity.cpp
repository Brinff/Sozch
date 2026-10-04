#include "Entity.h"
#include "XmlUtil.h"

void Entity::saveId(QXmlStreamWriter &w) const {
    w.writeAttribute("id", QString::number(id_));
}

bool Entity::loadId(QXmlStreamReader &r){
    int value = 0;

    if (!XmlUtil::readInt(r, "id", value))
        return false;

    if (value <= 0) {
        r.raiseError("id должен быть больше нуля, получено: " + QString::number(value));
        return false;
    }

    id_ = value;
    return true;
}