#pragma once

#include <QMainWindow>
#include <QString>

class SupplySystem;
class QTabWidget;
class SparePartWidget;
class ProductWidget;
class WarehouseWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(SupplySystem *system, const QString &initialXmlPath = QString(), QWidget *parent = nullptr);

private:
    SupplySystem *system_;
    QString currentFilePath_;
    QTabWidget *tabs_;
    SparePartWidget *sparePartWidget_;
    ProductWidget *productWidget_;
    WarehouseWidget *warehouseWidget_;

    void createMenu();
    void openXml();
    void saveXml();
    void saveXmlAs();
    void refreshAll();
};
