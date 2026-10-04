#include "MainWindow.h"
#include "SupplySystem.h"
#include "SparePartWidget.h"
#include "ProductWidget.h"
#include "WarehouseWidget.h"

#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(SupplySystem *system, const QString &initialXmlPath, QWidget *parent)
    : QMainWindow(parent), system_(system), currentFilePath_(initialXmlPath)
{
    setWindowTitle("Система снабжения запасными частями");

    tabs_ = new QTabWidget(this);
    setCentralWidget(tabs_);

    auto changed = [this]() {
        refreshAll();
        statusBar()->showMessage("Изменения внесены в модель", 2500);
    };

    sparePartWidget_ = new SparePartWidget(system_, changed, this);
    productWidget_ = new ProductWidget(system_, changed, this);
    warehouseWidget_ = new WarehouseWidget(system_, changed, this);

    tabs_->addTab(sparePartWidget_, "Запасные части");
    tabs_->addTab(productWidget_, "Изделия");
    tabs_->addTab(warehouseWidget_, "Склады");

    createMenu();
    statusBar()->showMessage(currentFilePath_.isEmpty()
                             ? "Данные не загружены из файла"
                             : QString("Загружено: %1").arg(currentFilePath_));
}

void MainWindow::createMenu()
{
    QMenu *fileMenu = menuBar()->addMenu("Файл");

    QAction *open = fileMenu->addAction("Открыть XML");
    QAction *save = fileMenu->addAction("Сохранить XML");
    QAction *saveAs = fileMenu->addAction("Сохранить как...");
    fileMenu->addSeparator();
    QAction *exit = fileMenu->addAction("Выход");

    connect(open, &QAction::triggered, this, [this] { openXml(); });
    connect(save, &QAction::triggered, this, [this] { saveXml(); });
    connect(saveAs, &QAction::triggered, this, [this] { saveXmlAs(); });
    connect(exit, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::openXml()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Открыть XML", currentFilePath_.isEmpty() ? QString() : currentFilePath_,
        "XML-файлы (*.xml);;Все файлы (*)");
    if (path.isEmpty()) return;

    QString error;
    if (!system_->loadFromFile(path, &error)) {
        QMessageBox::critical(this, "Ошибка загрузки XML", error);
        return;
    }

    currentFilePath_ = path;
    refreshAll();
    statusBar()->showMessage(QString("Открыт файл: %1").arg(path), 4000);
}

void MainWindow::saveXml()
{
    if (currentFilePath_.isEmpty()) {
        saveXmlAs();
        return;
    }

    QString error;
    if (!system_->saveToFile(currentFilePath_, &error)) {
        QMessageBox::critical(this, "Ошибка сохранения XML", error);
        return;
    }
    statusBar()->showMessage(QString("Сохранено: %1").arg(currentFilePath_), 4000);
}

void MainWindow::saveXmlAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Сохранить XML", currentFilePath_.isEmpty() ? "supply_system.xml" : currentFilePath_,
        "XML-файлы (*.xml);;Все файлы (*)");
    if (path.isEmpty()) return;

    QString error;
    if (!system_->saveToFile(path, &error)) {
        QMessageBox::critical(this, "Ошибка сохранения XML", error);
        return;
    }

    currentFilePath_ = path;
    statusBar()->showMessage(QString("Сохранено: %1").arg(path), 4000);
}

void MainWindow::refreshAll()
{
    sparePartWidget_->refresh();
    productWidget_->refresh();
    warehouseWidget_->refresh();
}
