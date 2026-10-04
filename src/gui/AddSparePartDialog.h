#pragma once
#include <QDialog>

class SupplySystem;
class QComboBox;
class QLineEdit;
class QDoubleSpinBox;

class AddSparePartDialog : public QDialog {
public:
    explicit AddSparePartDialog(const SupplySystem *system, QWidget *parent = nullptr);
    int groupId() const;
    QString name() const;
    double mass() const;
    double price() const;
    double length() const;
    double width() const;
    double height() const;
private:
    QComboBox *groupCombo_;
    QLineEdit *nameEdit_;
    QDoubleSpinBox *massSpin_;
    QDoubleSpinBox *priceSpin_;
    QDoubleSpinBox *lengthSpin_;
    QDoubleSpinBox *widthSpin_;
    QDoubleSpinBox *heightSpin_;
};
