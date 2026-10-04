#pragma once
#include <QDialog>
#include <QDate>

class SupplySystem;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QDateEdit;

class AddStockItemDialog : public QDialog {
public:
    explicit AddStockItemDialog(const SupplySystem *system, QWidget *parent = nullptr);
    int partId() const;
    int quantity() const;
    double price() const;
    QDate deliveryDate() const;
private:
    QComboBox *partCombo_;
    QSpinBox *quantitySpin_;
    QDoubleSpinBox *priceSpin_;
    QDateEdit *dateEdit_;
};
