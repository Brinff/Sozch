#include "ProductWidget.h"
#include "SupplySystem.h"
#include "Product.h"
#include "PartGroup.h"
#include "SparePartType.h"
#include "AddProductDialog.h"
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>

class AddComponentDialog : public QDialog {
public:
    explicit AddComponentDialog(const SupplySystem *system, int productId, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        Q_UNUSED(productId);
        setWindowTitle("Добавление запчасти в изделие");
        setModal(true);
        auto *root = new QVBoxLayout(this);
        auto *form = new QFormLayout;
        partCombo_ = new QComboBox;
        for (const PartGroup &g : system->catalog().groups())
            for (const SparePartType &p : g.parts())
                partCombo_->addItem(QString("%1 — %2").arg(p.id()).arg(p.name()), p.id());
        quantity_ = new QSpinBox;
        quantity_->setRange(1, 1000000000);
        form->addRow("Тип запчасти:", partCombo_);
        form->addRow("Количество:", quantity_);
        root->addLayout(form);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        buttons->button(QDialogButtonBox::Ok)->setText("Добавить");
        buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
        connect(buttons, &QDialogButtonBox::accepted, this, [this] {
            if (partCombo_->currentIndex() < 0) {
                QMessageBox::warning(this, "Ошибка", "Не выбрана запчасть.");
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);
        resize(520, 150);
    }
    int partId() const { return partCombo_->currentData().toInt(); }
    int quantity() const { return quantity_->value(); }
private:
    QComboBox *partCombo_;
    QSpinBox *quantity_;
};

ProductWidget::ProductWidget(SupplySystem *system, const std::function<void()> &onChanged, QWidget *parent)
    : QWidget(parent), system_(system), onChanged_(onChanged)
{
    auto *root = new QVBoxLayout(this);
    auto *top = new QHBoxLayout;
    productCombo_ = new QComboBox;
    top->addWidget(new QLabel("Изделие:"));
    top->addWidget(productCombo_, 1);
    root->addLayout(top);

    auto *box = new QGroupBox("Состав изделия");
    auto *boxLayout = new QVBoxLayout(box);
    componentsTable_ = new QTableWidget;
    componentsTable_->setColumnCount(2);
    componentsTable_->setHorizontalHeaderLabels({"Тип запчасти", "Количество"});
    componentsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    componentsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    componentsTable_->horizontalHeader()->setStretchLastSection(true);
    componentsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    boxLayout->addWidget(componentsTable_);
    root->addWidget(box, 1);

    auto *buttons = new QHBoxLayout;
    auto *addProductButton = new QPushButton("Добавить изделие");
    auto *addPartButton = new QPushButton("Добавить запчасть");
    buttons->addWidget(addProductButton);
    buttons->addWidget(addPartButton);
    buttons->addStretch();
    root->addLayout(buttons);

    connect(productCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { refreshTable(); });
    connect(addProductButton, &QPushButton::clicked, this, [this] { addProduct(); });
    connect(addPartButton, &QPushButton::clicked, this, [this] { addComponent(); });
    refresh();
}

void ProductWidget::refresh() {
    refreshProducts();
    refreshTable();
}

void ProductWidget::refreshProducts() {
    const int old = productCombo_->currentData().toInt();
    productCombo_->blockSignals(true);
    productCombo_->clear();
    for (const Product &p : system_->products())
        productCombo_->addItem(p.name(), p.id());
    int idx = productCombo_->findData(old);
    if (idx < 0 && productCombo_->count() > 0) idx = 0;
    productCombo_->setCurrentIndex(idx);
    productCombo_->blockSignals(false);
}

void ProductWidget::refreshTable() {
    componentsTable_->setRowCount(0);
    const Product *p = system_->findProduct(productCombo_->currentData().toInt());
    if (!p) return;
    componentsTable_->setRowCount(p->components().size());
    int row = 0;
    for (auto it = p->components().constBegin(); it != p->components().constEnd(); ++it, ++row) {
        const SparePartType *part = system_->catalog().findPart(it.key());
        componentsTable_->setItem(row, 0, new QTableWidgetItem(part ? part->name() : QString("ID %1").arg(it.key())));
        componentsTable_->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
    }
}

void ProductWidget::addProduct() {
    AddProductDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    Product p(system_->nextProductId(), dialog.name());
    if (!system_->addProduct(p)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить изделие.");
        return;
    }
    refresh();
    if (onChanged_) onChanged_();
    productCombo_->setCurrentIndex(productCombo_->findData(p.id()));
}

void ProductWidget::addComponent() {
    const Product *p = system_->findProduct(productCombo_->currentData().toInt());
    if (!p) {
        QMessageBox::information(this, "Нет изделия", "Сначала добавьте или выберите изделие.");
        return;
    }
    if (system_->catalog().groups().isEmpty()) {
        QMessageBox::information(this, "Нет запчастей", "Сначала добавьте запчасть в каталог.");
        return;
    }
    AddComponentDialog dialog(system_, p->id(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    if (p->hasPart(dialog.partId())) {
        QMessageBox::warning(this, "Дубликат", "Этот тип запчасти уже присутствует в изделии.");
        return;
    }
    if (!system_->addComponent(p->id(), dialog.partId(), dialog.quantity())) {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить запчасть в изделие.");
        return;
    }
    refresh();
    if (onChanged_) onChanged_();
}
