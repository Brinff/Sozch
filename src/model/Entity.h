#pragma once
#include "XmlSerializable.h"

class Entity : public XmlSerializable {
public:
    explicit Entity(int id = 0) : id_(id) {}
    int id() const { return id_; };

protected:
    void saveId(QXmlStreamWriter &w) const;
    bool loadId(QXmlStreamReader &r);

private:
    int id_;
};