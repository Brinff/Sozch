#include "SparePartWidget.h"
#include "SupplySystem.h"
#include "PartGroup.h"
#include "SparePartType.h"
#include "Warehouse.h"
#include "StockItem.h"
#include "Dimensions.h"
#include "AddGroupDialog.h"
#include "AddSparePartDialog.h"
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QStringList>
#include <QLabel>

static QString money(double v) { return QString::number(v, 'f', 2); }
static QString dimensions(const Dimensions &d) {
    return QString("%1 x %2 x %3").arg(d.length(), 0, 'f', 3).arg(d.width(), 0, 'f', 3).arg(d.height(), 0, 'f', 3);
}

SparePartWidget::SparePartWidget(SupplySystem *system, const std::function<void()> &onChanged, QWidget *parent)
    : QWidget(parent), system_(system), onChanged_(onChanged)
{
    auto *root = new QVBoxLayout(this);

    auto *top = new QHBoxLayout;
    groupCombo_ = new QComboBox;
    warehouseCombo_ = new QComboBox;
    top->addWidget(new QLabel("Группа:"));
    top->addWidget(groupCombo_, 1);
    top->addSpacing(20);
    top->addWidget(new QLabel("Склад:"));
    top->addWidget(warehouseCombo_, 1);
    root->addLayout(top);

    auto *partsBox = new QGroupBox("Запасные части");
    auto *partsLayout = new QVBoxLayout(partsBox);
    partsTable_ = new QTableWidget;
    partsTable_->setColumnCount(5);
    partsTable_->setHorizontalHeaderLabels({"№", "Название", "Цена", "Масса", "Размеры"});
    partsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    partsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    partsTable_->horizontalHeader()->setStretchLastSection(true);
    partsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    partsLayout->addWidget(partsTable_);
    root->addWidget(partsBox, 2);

    auto *stockBox = new QGroupBox("Наличие на выбранном складе");
    auto *stockLayout = new QVBoxLayout(stockBox);
    stockTable_ = new QTableWidget;
    stockTable_->setColumnCount(4);
    stockTable_->setHorizontalHeaderLabels({"Запчасть", "Количество", "Цена", "Дата поставки"});
    stockTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    stockTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stockTable_->horizontalHeader()->setStretchLastSection(true);
    stockTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    stockLayout->addWidget(stockTable_);
    root->addWidget(stockBox, 1);

    auto *buttons = new QHBoxLayout;
    auto *addGroupButton = new QPushButton("Добавить группу");
    auto *addPartButton = new QPushButton("Добавить запчасть");
    buttons->addWidget(addGroupButton);
    buttons->addWidget(addPartButton);
    buttons->addStretch();
    root->addLayout(buttons);

    connect(groupCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { refreshParts(); });
    connect(warehouseCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { refreshStock(); });
    connect(addGroupButton, &QPushButton::clicked, this, [this] { addGroup(); });
    connect(addPartButton, &QPushButton::clicked, this, [this] { addPart(); });

    refresh();
}

void SparePartWidget::refresh() {
    refreshGroups();
    refreshWarehouses();
    refreshParts();
    refreshStock();
}

void SparePartWidget::refreshGroups() {
    const int old = groupCombo_->currentData().toInt();
    groupCombo_->blockSignals(true);
    groupCombo_->clear();
    for (const PartGroup &g : system_->catalog().groups())
        groupCombo_->addItem(g.name(), g.id());
    int idx = groupCombo_->findData(old);
    if (idx < 0 && groupCombo_->count() > 0) idx = 0;
    groupCombo_->setCurrentIndex(idx);
    groupCombo_->blockSignals(false);
}

void SparePartWidget::refreshWarehouses() {
    const int old = warehouseCombo_->currentData().toInt();
    warehouseCombo_->blockSignals(true);
    warehouseCombo_->clear();
    for (const Warehouse &w : system_->warehouses())
        warehouseCombo_->addItem(w.name(), w.id());
    int idx = warehouseCombo_->findData(old);
    if (idx < 0 && warehouseCombo_->count() > 0) idx = 0;
    warehouseCombo_->setCurrentIndex(idx);
    warehouseCombo_->blockSignals(false);
}

void SparePartWidget::refreshParts() {
    partsTable_->setRowCount(0);
    const PartGroup *g = system_->catalog().findGroup(groupCombo_->currentData().toInt());
    if (!g) return;
    partsTable_->setRowCount(g->parts().size());
    for (int i = 0; i < g->parts().size(); ++i) {
        const auto &p = g->parts().at(i);
        partsTable_->setItem(i, 0, new QTableWidgetItem(QString::number(p.id())));
        partsTable_->setItem(i, 1, new QTableWidgetItem(p.name()));
        partsTable_->setItem(i, 2, new QTableWidgetItem(money(p.price())));
        partsTable_->setItem(i, 3, new QTableWidgetItem(QString::number(p.mass(), 'f', 3)));
        partsTable_->setItem(i, 4, new QTableWidgetItem(dimensions(p.dimensions())));
    }
    partsTable_->resizeRowsToContents();
}

void SparePartWidget::refreshStock() {
    stockTable_->setRowCount(0);
    const int warehouseId = warehouseCombo_->currentData().toInt();
    const Warehouse *w = system_->findWarehouse(warehouseId);
    if (!w) return;
    stockTable_->setRowCount(w->stock().size());
    for (int i = 0; i < w->stock().size(); ++i) {
        const StockItem &item = w->stock().at(i);
        const SparePartType *p = system_->catalog().findPart(item.partId());
        stockTable_->setItem(i, 0, new QTableWidgetItem(p ? p->name() : QString("ID %1").arg(item.partId())));
        stockTable_->setItem(i, 1, new QTableWidgetItem(QString::number(item.quantity())));
        stockTable_->setItem(i, 2, new QTableWidgetItem(money(item.price())));
        stockTable_->setItem(i, 3, new QTableWidgetItem(item.deliveryDate().toString("dd.MM.yyyy")));
    }
    stockTable_->resizeRowsToContents();
}

void SparePartWidget::addGroup() {
    AddGroupDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    PartGroup group(system_->catalog().nextGroupId(), dialog.name());
    if (!system_->addGroup(group)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить группу.");
        return;
    }
    refresh();
    if (onChanged_) onChanged_();
    groupCombo_->setCurrentIndex(groupCombo_->findData(group.id()));
}

void SparePartWidget::addPart() {
    if (system_->catalog().groups().isEmpty()) {
        QMessageBox::information(this, "Нет групп", "Сначала добавьте хотя бы одну группу.");
        return;
    }
    AddSparePartDialog dialog(system_, this);
    if (dialog.exec() != QDialog::Accepted) return;
    SparePartType part(system_->catalog().nextPartId(), dialog.name(), dialog.mass(), dialog.price(),
                       Dimensions(dialog.length(), dialog.width(), dialog.height()));
    if (!system_->addPart(dialog.groupId(), part)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить запчасть.");
        return;
    }
    refresh();
    if (onChanged_) onChanged_();
    groupCombo_->setCurrentIndex(groupCombo_->findData(dialog.groupId()));
}
