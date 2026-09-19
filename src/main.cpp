#include <QApplication>
#include <QMessageBox>
#include "Database.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("AutoService");
    app.setApplicationDisplayName("Автосервис - Информационная система");
    app.setOrganizationName("Coursework");

    if (!Database::instance().init()) {
        QMessageBox::critical(
            nullptr,
            "Ошибка инициализации",
            "Не удалось подключиться к базе данных SQLite или создать структуру таблиц.\nПриложение будет закрыто."
        );
        return 1;
    }

    MainWindow window;
    window.show();

    return app.exec();
}
