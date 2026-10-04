#pragma once
#include "XmlSerializable.h"
#include <QDate>

class StockItem : public XmlSerializable {
public:
    explicit StockItem(int partId = 0, int quantity = 0, double price = 0, const QDate &deliveryDate = QDate()) 
        : partId_(partId), quantity_(quantity), price_(price), deliveryDate_(deliveryDate) {}

    void setItem(int partId, int quantity, double price, const QDate &deliveryDate) 
        { partId_ = partId; quantity_ = quantity; price_ = price; deliveryDate_ = deliveryDate; }

    void setPartId(int partId) { partId_ = partId; }
    void setQuantity(int quantity) { quantity_ = quantity; }
    void setPrice(double price) { price_ = price; }
    void setDeliveryDate(const QDate &deliveryDate) { deliveryDate_ = deliveryDate; }

    int partId() const { return partId_; }
    int quantity() const { return quantity_; }
    double price() const { return price_; }
    const QDate &deliveryDate() const { return deliveryDate_; }

    bool isValid() const { return partId_ > 0 && quantity_ >= 0 && price_ >= 0 && deliveryDate_.isValid(); }

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    int partId_;
    int quantity_;
    double price_;
    QDate deliveryDate_;
};