#include "AddStockItemDialog.h"
#include "SupplySystem.h"
#include "PartGroup.h"
#include "SparePartType.h"
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDate>
#include <QPushButton>

AddStockItemDialog::AddStockItemDialog(const SupplySystem *system, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Добавление запчасти на склад");
    setModal(true);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    partCombo_ = new QComboBox;
    for (const PartGroup &g : system->catalog().groups()) {
        for (const SparePartType &p : g.parts())
            partCombo_->addItem(QString("%1 — %2").arg(p.id()).arg(p.name()), p.id());
    }

    quantitySpin_ = new QSpinBox;
    quantitySpin_->setRange(0, 1000000000);
    priceSpin_ = new QDoubleSpinBox;
    priceSpin_->setRange(0, 1000000000);
    priceSpin_->setDecimals(2);
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("dd.MM.yyyy");

    form->addRow("Запчасть:", partCombo_);
    form->addRow("Количество:", quantitySpin_);
    form->addRow("Цена:", priceSpin_);
    form->addRow("Дата поставки:", dateEdit_);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText("Добавить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (partCombo_->currentIndex() < 0) {
            QMessageBox::warning(this, "Ошибка", "Не выбрана запчасть.");
            return;
        }
        if (!dateEdit_->date().isValid()) {
            QMessageBox::warning(this, "Ошибка", "Некорректная дата поставки.");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    resize(520, 190);
}

int AddStockItemDialog::partId() const { return partCombo_->currentData().toInt(); }
int AddStockItemDialog::quantity() const { return quantitySpin_->value(); }
double AddStockItemDialog::price() const { return priceSpin_->value(); }
QDate AddStockItemDialog::deliveryDate() const { return dateEdit_->date(); }
