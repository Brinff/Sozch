#pragma once
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

class XmlSerializable {
public:
    virtual ~XmlSerializable() = default;
    virtual void saveXml(QXmlStreamWriter &w) const = 0;
    virtual void loadXml(QXmlStreamReader &r) = 0;
};