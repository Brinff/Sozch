#include "AddSparePartDialog.h"
#include "SupplySystem.h"
#include "PartGroup.h"
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

static QDoubleSpinBox *makeDouble(double min, double max, int decimals = 3)
{
    auto *s = new QDoubleSpinBox;
    s->setRange(min, max);
    s->setDecimals(decimals);
    s->setSingleStep(0.1);
    return s;
}

AddSparePartDialog::AddSparePartDialog(const SupplySystem *system, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Добавление запчасти");
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    groupCombo_ = new QComboBox;
    for (const PartGroup &g : system->catalog().groups())
        groupCombo_->addItem(g.name(), g.id());

    nameEdit_ = new QLineEdit;
    massSpin_ = makeDouble(0.001, 1000000.0);
    priceSpin_ = makeDouble(0.0, 1000000000.0);
    lengthSpin_ = makeDouble(0.001, 1000000.0);
    widthSpin_ = makeDouble(0.001, 1000000.0);
    heightSpin_ = makeDouble(0.001, 1000000.0);

    form->addRow("Группа:", groupCombo_);
    form->addRow("Название:", nameEdit_);
    form->addRow("Масса:", massSpin_);
    form->addRow("Цена:", priceSpin_);
    form->addRow("Длина:", lengthSpin_);
    form->addRow("Ширина:", widthSpin_);
    form->addRow("Высота:", heightSpin_);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText("Добавить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (groupCombo_->currentIndex() < 0) {
            QMessageBox::warning(this, "Ошибка", "Не выбрана группа.");
            return;
        }
        if (nameEdit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Название запчасти не может быть пустым.");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    resize(460, 300);
}

int AddSparePartDialog::groupId() const { return groupCombo_->currentData().toInt(); }
QString AddSparePartDialog::name() const { return nameEdit_->text().trimmed(); }
double AddSparePartDialog::mass() const { return massSpin_->value(); }
double AddSparePartDialog::price() const { return priceSpin_->value(); }
double AddSparePartDialog::length() const { return lengthSpin_->value(); }
double AddSparePartDialog::width() const { return widthSpin_->value(); }
double AddSparePartDialog::height() const { return heightSpin_->value(); }
