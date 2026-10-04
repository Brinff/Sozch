#pragma once
#include <QWidget>
#include <functional>

class SupplySystem;
class QComboBox;
class QTableWidget;
class QLabel;

class SparePartWidget : public QWidget {
public:
    explicit SparePartWidget(SupplySystem *system, const std::function<void()> &onChanged = {}, QWidget *parent = nullptr);
    void refresh();
private:
    SupplySystem *system_;
    std::function<void()> onChanged_;
    QComboBox *groupCombo_;
    QComboBox *warehouseCombo_;
    QTableWidget *partsTable_;
    QTableWidget *stockTable_;
    void refreshGroups();
    void refreshParts();
    void refreshWarehouses();
    void refreshStock();
    void addGroup();
    void addPart();
};
