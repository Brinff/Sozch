#include "NamedEntity.h"
#include "XmlUtil.h"

void NamedEntity::saveName(QXmlStreamWriter &w) const {
    w.writeAttribute("name", name_);
}

bool NamedEntity::loadName(QXmlStreamReader &r){
    QString value;
    
    if (!XmlUtil::readString(r, "name", value))
        return false;

    if (value.isEmpty()) {
        r.raiseError("Атрибут name не должен быть пустым");
        return false;
    }

    name_ = value;
    return true;
}