#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <iostream>
#include <cstdlib>

#include "Database.h"
#include "MainWindow.h"

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << msg << " (line " << __LINE__ << ")" << std::endl; \
            std::exit(1); \
        } \
    } while (0)

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::cout << ">>> Running Comprehensive AutoService Test Suite <<<" << std::endl;

    QString testDbPath = "test_autoservice.db";
    QFile::remove(testDbPath);

    Database &db = Database::instance();
    bool initOk = db.init(testDbPath);
    TEST_CHECK(initOk, "Database::init failed");
    TEST_CHECK(db.isOpen(), "Database::isOpen is false");
    std::cout << "[OK] Database initialization and opening" << std::endl;

    // Verify seed data (4 orders)
    QSqlQuery countQuery = db.getAllOrdersQuery();
    int seedCount = 0;
    while (countQuery.next()) {
        seedCount++;
    }
    TEST_CHECK(seedCount == 4, "Initial seed orders count must be 4");
    std::cout << "[OK] 4 initial seed orders present" << std::endl;

    // Test creating a new order
    QString err;
    bool ok = db.createOrder(
        "Сидоров Алексей Петрович",
        "+7 (915) 000-11-22",
        "Mazda",
        "CX-5",
        "Е999КХ77",
        "Замена тормозных колодок",
        5500.0,
        "Принят",
        &err
    );
    TEST_CHECK(ok, "Failed to create order: " + err.toStdString());
    std::cout << "[OK] Order creation for new client & car" << std::endl;

    // Test creating second order for existing client phone
    ok = db.createOrder(
        "Сидоров Алексей Петрович",
        "+7 (915) 000-11-22",
        "Mazda",
        "CX-5",
        "Е999КХ77",
        "Диагностика подвески",
        2500.0,
        "В работе",
        &err
    );
    TEST_CHECK(ok, "Failed to create second order: " + err.toStdString());

    // Verify no client duplicate was created
    QSqlQuery clientCountQuery(QSqlDatabase::database());
    clientCountQuery.prepare("SELECT COUNT(*) FROM clients WHERE phone = :phone");
    clientCountQuery.bindValue(":phone", "+7 (915) 000-11-22");
    TEST_CHECK(clientCountQuery.exec(), "clientCountQuery exec failed");
    TEST_CHECK(clientCountQuery.next(), "clientCountQuery next failed");
    TEST_CHECK(clientCountQuery.value(0).toInt() == 1, "Client must not be duplicated");
    std::cout << "[OK] Client reuse by phone without duplicates" << std::endl;

    // Test update status
    QSqlQuery latestOrder(QSqlDatabase::database());
    latestOrder.exec("SELECT id FROM orders ORDER BY id DESC LIMIT 1");
    TEST_CHECK(latestOrder.next(), "No order found to update status");
    int latestId = latestOrder.value(0).toInt();

    TEST_CHECK(db.updateOrderStatus(latestId, "Готов к выдаче", &err), "updateOrderStatus failed: " + err.toStdString());

    QSqlQuery checkStatus(QSqlDatabase::database());
    checkStatus.prepare("SELECT status FROM orders WHERE id = :id");
    checkStatus.bindValue(":id", latestId);
    TEST_CHECK(checkStatus.exec(), "checkStatus exec failed");
    TEST_CHECK(checkStatus.next(), "checkStatus next failed");
    TEST_CHECK(checkStatus.value(0).toString() == "Готов к выдаче", "Status was not updated");
    std::cout << "[OK] Order status update" << std::endl;

    // Test delete order
    TEST_CHECK(db.deleteOrder(latestId, &err), "deleteOrder failed: " + err.toStdString());
    QSqlQuery checkDeleted(QSqlDatabase::database());
    checkDeleted.prepare("SELECT COUNT(*) FROM orders WHERE id = :id");
    checkDeleted.bindValue(":id", latestId);
    TEST_CHECK(checkDeleted.exec(), "checkDeleted exec failed");
    TEST_CHECK(checkDeleted.next(), "checkDeleted next failed");
    TEST_CHECK(checkDeleted.value(0).toInt() == 0, "Order was not deleted");
    std::cout << "[OK] Order deletion" << std::endl;

    // Test Model and Unicode Cyrillic Search via Proxy
    OrdersModel model;
    model.setQuery(db.getAllOrdersQuery());
    while (model.canFetchMore()) {
        model.fetchMore();
    }
    TEST_CHECK(model.rowCount() == 5, "Model row count must be 5 (4 seed + 1 created)");

    // Check display role formatting of cost column
    QModelIndex costIdx = model.index(0, 8);
    QString costText = model.data(costIdx, Qt::DisplayRole).toString();
    TEST_CHECK(costText.endsWith(" руб."), "Cost display role must end with ' руб.'");
    std::cout << "[OK] Cost formatting with ' руб.'" << std::endl;

    OrderFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    // Cyrillic search lowercase "иванов"
    proxy.setSearchText("иванов");
    TEST_CHECK(proxy.rowCount() == 1, "Expected exactly 1 order for 'иванов'");
    TEST_CHECK(proxy.data(proxy.index(0, 2)).toString().contains("Иванов"), "Client name must match 'Иванов'");
    std::cout << "[OK] Cyrillic case-insensitive search by client name ('иванов')" << std::endl;

    // Cyrillic search lowercase "а123вс"
    proxy.setSearchText("а123вс");
    TEST_CHECK(proxy.rowCount() == 1, "Expected exactly 1 order for 'а123вс'");
    TEST_CHECK(proxy.data(proxy.index(0, 6)).toString() == "А123ВС77", "Plate must match 'А123ВС77'");
    std::cout << "[OK] Cyrillic case-insensitive search by plate number ('а123вс')" << std::endl;

    // Status filter
    proxy.setSearchText("");
    proxy.setStatusFilter("Принят");
    TEST_CHECK(proxy.rowCount() > 0, "Expected at least 1 order with status 'Принят'");
    for (int i = 0; i < proxy.rowCount(); ++i) {
        TEST_CHECK(proxy.data(proxy.index(i, 9)).toString() == "Принят", "Status must be 'Принят'");
    }
    std::cout << "[OK] Filter by status 'Принят' count = " << proxy.rowCount() << std::endl;

    proxy.setStatusFilter("Все");
    TEST_CHECK(proxy.rowCount() == 5, "Expected all 5 orders when filter is 'Все'");
    std::cout << "[OK] Reset status filter count = " << proxy.rowCount() << std::endl;

    // Test CSV Export logic directly
    QString csvFilePath = "test_export.csv";
    QFile csvFile(csvFilePath);
    TEST_CHECK(csvFile.open(QIODevice::WriteOnly | QIODevice::Text), "Failed to open CSV for writing");
    csvFile.write("\xEF\xBB\xBF");
    QTextStream out(&csvFile);
    out.setEncoding(QStringConverter::Utf8);

    auto escapeCsv = [](const QString &val) -> QString {
        QString str = val;
        bool hasSpecial = str.contains(';') || str.contains('"') || str.contains('\n') || str.contains('\r');
        if (str.contains('"')) {
            str.replace('"', "\"\"");
        }
        if (hasSpecial) {
            return "\"" + str + "\"";
        }
        return str;
    };

    QStringList headers;
    for (int col = 0; col < model.columnCount(); ++col) {
        headers.append(escapeCsv(model.headerData(col, Qt::Horizontal).toString()));
    }
    out << headers.join(";") << "\n";

    for (int r = 0; r < proxy.rowCount(); ++r) {
        QStringList rowValues;
        QModelIndex proxyRowIdx = proxy.index(r, 0);
        QModelIndex srcRowIdx = proxy.mapToSource(proxyRowIdx);
        int srcRow = srcRowIdx.row();
        for (int col = 0; col < model.columnCount(); ++col) {
            if (col == 8) {
                double cost = model.record(srcRow).value("cost").toDouble();
                rowValues.append(QString::number(cost, 'f', 2));
            } else {
                QString text = model.data(model.index(srcRow, col), Qt::DisplayRole).toString();
                rowValues.append(escapeCsv(text));
            }
        }
        out << rowValues.join(";") << "\n";
    }
    csvFile.close();

    // Verify CSV file exists and contains UTF-8 BOM and correct content
    QFile readCsv(csvFilePath);
    TEST_CHECK(readCsv.open(QIODevice::ReadOnly), "Failed to open CSV for reading");
    QByteArray bytes = readCsv.readAll();
    TEST_CHECK(bytes.startsWith("\xEF\xBB\xBF"), "CSV file must start with UTF-8 BOM");
    QString content = QString::fromUtf8(bytes);
    TEST_CHECK(content.contains("Иванов Иван Иванович"), "CSV must contain client name");
    TEST_CHECK(content.contains("А123ВС77"), "CSV must contain plate number");
    TEST_CHECK(content.contains(";"), "CSV must use semicolon separator");
    readCsv.close();
    QFile::remove(csvFilePath);
    std::cout << "[OK] CSV export with UTF-8 BOM and semicolon format verified" << std::endl;

    // Clean up
    QSqlDatabase::database().close();
    QFile::remove(testDbPath);

    std::cout << ">>> ALL UNIT & INTEGRATION TESTS PASSED SUCCESSFULLY! <<<" << std::endl;
    return 0;
}
