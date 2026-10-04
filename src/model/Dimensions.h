#pragma once
#include "XmlSerializable.h"

class Dimensions : public XmlSerializable {
public:
    explicit Dimensions(double length = 0, double width = 0, double height = 0) : length_(length), width_(width), height_(height) {}

    void setSize(double length, double width, double height) { length_ = length; width_ = width; height_ = height; }

    void setLength(double length) { length_ = length; }
    void setWidth(double width) { width_ = width; }
    void setHeight(double height) { height_ = height; }

    double length() const { return length_; }
    double width() const { return width_; }
    double height() const { return height_; }

    bool isValid() const { return length_ > 0 && width_ > 0 && height_ > 0; }

    void saveXml(QXmlStreamWriter &w) const override;
    void loadXml(QXmlStreamReader &r) override;

private:
    double length_;
    double width_;
    double height_;
};