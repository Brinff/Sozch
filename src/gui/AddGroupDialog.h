#pragma once
#include <QDialog>

class QLineEdit;

class AddGroupDialog : public QDialog {
public:
    explicit AddGroupDialog(QWidget *parent = nullptr);
    QString name() const;
private:
    QLineEdit *nameEdit_;
};
