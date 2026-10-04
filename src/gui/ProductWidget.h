#pragma once
#include <QWidget>
#include <functional>

class SupplySystem;
class QComboBox;
class QTableWidget;

class ProductWidget : public QWidget {
public:
    explicit ProductWidget(SupplySystem *system, const std::function<void()> &onChanged = {}, QWidget *parent = nullptr);
    void refresh();
private:
    SupplySystem *system_;
    std::function<void()> onChanged_;
    QComboBox *productCombo_;
    QTableWidget *componentsTable_;
    void refreshProducts();
    void refreshTable();
    void addProduct();
    void addComponent();
};
