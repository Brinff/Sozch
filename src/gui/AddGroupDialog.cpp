#include "AddGroupDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

AddGroupDialog::AddGroupDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Добавление группы");
    setModal(true);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText("Название группы");
    form->addRow("Название:", nameEdit_);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText("Добавить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (nameEdit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Название группы не может быть пустым.");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    resize(420, 120);
}

QString AddGroupDialog::name() const { return nameEdit_->text().trimmed(); }
