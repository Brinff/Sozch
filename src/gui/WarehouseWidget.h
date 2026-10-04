#pragma once
#include <QWidget>
#include <functional>

class SupplySystem;
class QComboBox;
class QTableWidget;

class WarehouseWidget : public QWidget {
public:
    explicit WarehouseWidget(SupplySystem *system, const std::function<void()> &onChanged = {}, QWidget *parent = nullptr);
    void refresh();
private:
    SupplySystem *system_;
    std::function<void()> onChanged_;
    QComboBox *warehouseCombo_;
    QTableWidget *stockTable_;
    void refreshWarehouses();
    void refreshTable();
    void addStock();
};
