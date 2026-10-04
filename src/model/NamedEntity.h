#pragma once
#include "Entity.h"

class NamedEntity : public Entity {
public:
    explicit NamedEntity(int id = 0, const QString &name = QString()) : Entity(id), name_(name) {}
    const QString &name() const { return name_; };

protected:
    void saveName(QXmlStreamWriter &w) const;
    bool loadName(QXmlStreamReader &r);

private:
    QString name_;
};