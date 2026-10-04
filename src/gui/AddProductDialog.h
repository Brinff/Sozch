#pragma once
#include <QDialog>
class QLineEdit;

class AddProductDialog : public QDialog {
public:
    explicit AddProductDialog(QWidget *parent = nullptr);
    QString name() const;
private:
    QLineEdit *nameEdit_;
};
