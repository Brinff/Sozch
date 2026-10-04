#include "MainWindow.h"
#include "SupplySystem.h"

#include <QApplication>
#include <QMessageBox>
#include <QDir>

#ifndef SAMPLE_XML
#define SAMPLE_XML ""
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Система снабжения запасными частями");
    QApplication::setOrganizationName("sozch");

    SupplySystem system;
    QString error;
    const QString defaultPath = QString::fromUtf8(SAMPLE_XML);

    if (!defaultPath.isEmpty() && !system.loadFromFile(defaultPath, &error)) {
        QMessageBox::warning(nullptr, "Ошибка загрузки",
                             QString("Не удалось загрузить демонстрационный XML:\n%1").arg(error));
    }

    MainWindow window(&system, defaultPath);
    window.resize(1100, 700);
    window.show();

    return app.exec();
}
