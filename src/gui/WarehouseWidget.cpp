#include "WarehouseWidget.h"
#include "SupplySystem.h"
#include "Warehouse.h"
#include "StockItem.h"
#include "SparePartType.h"
#include "AddStockItemDialog.h"
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>

WarehouseWidget::WarehouseWidget(SupplySystem *system, const std::function<void()> &onChanged, QWidget *parent)
    : QWidget(parent), system_(system), onChanged_(onChanged)
{
    auto *root = new QVBoxLayout(this);
    auto *top = new QHBoxLayout;
    warehouseCombo_ = new QComboBox;
    top->addWidget(new QLabel("Склад:"));
    top->addWidget(warehouseCombo_, 1);
    root->addLayout(top);

    auto *box = new QGroupBox("Содержимое склада");
    auto *boxLayout = new QVBoxLayout(box);
    stockTable_ = new QTableWidget;
    stockTable_->setColumnCount(4);
    stockTable_->setHorizontalHeaderLabels({"Запчасть", "Количество", "Цена", "Дата поставки"});
    stockTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    stockTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stockTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    stockTable_->horizontalHeader()->setStretchLastSection(true);
    boxLayout->addWidget(stockTable_);
    root->addWidget(box, 1);

    auto *buttons = new QHBoxLayout;
    auto *addButton = new QPushButton("Добавить запчасть");
    buttons->addWidget(addButton);
    buttons->addStretch();
    root->addLayout(buttons);

    connect(warehouseCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { refreshTable(); });
    connect(addButton, &QPushButton::clicked, this, [this] { addStock(); });
    refresh();
}

void WarehouseWidget::refresh() {
    refreshWarehouses();
    refreshTable();
}

void WarehouseWidget::refreshWarehouses() {
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

void WarehouseWidget::refreshTable() {
    stockTable_->setRowCount(0);
    const Warehouse *w = system_->findWarehouse(warehouseCombo_->currentData().toInt());
    if (!w) return;
    stockTable_->setRowCount(w->stock().size());
    for (int i = 0; i < w->stock().size(); ++i) {
        const StockItem &item = w->stock().at(i);
        const SparePartType *p = system_->catalog().findPart(item.partId());
        stockTable_->setItem(i, 0, new QTableWidgetItem(p ? p->name() : QString("ID %1").arg(item.partId())));
        stockTable_->setItem(i, 1, new QTableWidgetItem(QString::number(item.quantity())));
        stockTable_->setItem(i, 2, new QTableWidgetItem(QString::number(item.price(), 'f', 2)));
        stockTable_->setItem(i, 3, new QTableWidgetItem(item.deliveryDate().toString("dd.MM.yyyy")));
    }
}

void WarehouseWidget::addStock() {
    const int warehouseId = warehouseCombo_->currentData().toInt();
    if (!system_->findWarehouse(warehouseId)) {
        QMessageBox::information(this, "Нет склада", "Сначала выберите склад.");
        return;
    }
    if (system_->catalog().groups().isEmpty()) {
        QMessageBox::information(this, "Нет запчастей", "Сначала добавьте запчасть в каталог.");
        return;
    }

    AddStockItemDialog dialog(system_, this);
    if (dialog.exec() != QDialog::Accepted) return;

    const Warehouse *w = system_->findWarehouse(warehouseId);
    if (w && w->findStock(dialog.partId())) {
        QMessageBox::warning(this, "Дубликат", "Такая запчасть уже есть на выбранном складе.");
        return;
    }

    StockItem item(dialog.partId(), dialog.quantity(), dialog.price(), dialog.deliveryDate());
    if (!system_->addStock(warehouseId, item)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить запчасть на склад.");
        return;
    }
    refresh();
    if (onChanged_) onChanged_();
}
