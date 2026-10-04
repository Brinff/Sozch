#pragma once
#include "NamedEntity.h"
#include "Dimensions.h"

class SparePartType : public NamedEntity {
public:
    explicit SparePartType(int id = 0, const QString &name = QString(), double mass = 0, double price = 0, const Dimensions &dimensions = Dimensions()) 
        : NamedEntity(id, name), mass_(mass), price_(price), dimensions_(dimensions) {}

    void setMass(double mass) { mass_ = mass; }
    void setPrice(double price) { price_ = price; }
    void setDimensions(const Dimensions &dimensions) { dimensions_ = dimensions; }

    double mass() const { return mass_; }
    double price() const { return price_; }
    const Dimensions &dimensions() const { return dimensions_; }

    bool isValid() const { return id() > 0 && !name().isEmpty() && mass_ > 0 && price_ >= 0 && dimensions_.isValid(); }    

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    double mass_;
    double price_;
    Dimensions dimensions_;
};