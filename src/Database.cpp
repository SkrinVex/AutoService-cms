#include "Database.h"
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>

Database &Database::instance()
{
    static Database instance;
    return instance;
}

Database::~Database()
{
    if (QCoreApplication::instance() && QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::database().close();
    }
}

bool Database::isOpen() const
{
    if (!QCoreApplication::instance() || !QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        return false;
    }
    return QSqlDatabase::database().isOpen();
}

bool Database::init(const QString &dbPath)
{
    QSqlDatabase db;
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }

    QString path = dbPath.isEmpty() ? "autoservice.db" : dbPath;
    db.setDatabaseName(path);

    if (!db.open()) {
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON;");

    if (!createTables()) {
        return false;
    }

    seedInitialData();
    return true;
}

bool Database::createTables(QString *errorMessage)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    bool ok = q.exec("CREATE TABLE IF NOT EXISTS clients ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "full_name TEXT NOT NULL, "
                     "phone TEXT NOT NULL"
                     ");");
    if (!ok) {
        if (errorMessage) *errorMessage = q.lastError().text();
        return false;
    }

    ok = q.exec("CREATE TABLE IF NOT EXISTS cars ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "client_id INTEGER NOT NULL, "
                "brand TEXT NOT NULL, "
                "model TEXT NOT NULL, "
                "plate_number TEXT NOT NULL, "
                "FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE"
                ");");
    if (!ok) {
        if (errorMessage) *errorMessage = q.lastError().text();
        return false;
    }

    ok = q.exec("CREATE TABLE IF NOT EXISTS orders ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "car_id INTEGER NOT NULL, "
                "description TEXT NOT NULL, "
                "status TEXT NOT NULL, "
                "cost REAL NOT NULL, "
                "date_created TEXT NOT NULL, "
                "FOREIGN KEY (car_id) REFERENCES cars(id) ON DELETE CASCADE"
                ");");
    if (!ok) {
        if (errorMessage) *errorMessage = q.lastError().text();
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_cars_client_id ON cars(client_id);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_cars_plate ON cars(plate_number);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_orders_car_id ON orders(car_id);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);");

    return true;
}

void Database::seedInitialData()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery check(db);
    if (check.exec("SELECT COUNT(*) FROM orders") && check.next()) {
        if (check.value(0).toInt() == 0) {
            createOrder("Иванов Иван Иванович", "+7 (916) 123-45-67", "Toyota", "Camry", "А123ВС77",
                        "Замена моторного масла и фильтров, диагностика подвески", 4500.0, "Закрыт");
            createOrder("Петров Сергей Алексеевич", "+7 (926) 987-65-43", "Kia", "Rio", "В789ОР199",
                        "Замена передних тормозных колодок и тормозных дисков", 8200.0, "Готов к выдаче");
            createOrder("Смирнова Анна Павловна", "+7 (903) 555-11-22", "Volkswagen", "Tiguan", "М456УК777",
                        "Компьютерная диагностика двигателя, замена свечей зажигания", 6800.0, "В работе");
            createOrder("Козлов Дмитрий Николаевич", "+7 (977) 333-88-99", "Hyundai", "Solaris", "С321ХЕ799",
                        "Ремонт генератора, замена приводного ремня навесного оборудования", 12500.0, "Принят");
        }
    }
}

bool Database::createOrder(const QString &clientName,
                           const QString &clientPhone,
                           const QString &carBrand,
                           const QString &carModel,
                           const QString &plateNumber,
                           const QString &description,
                           double cost,
                           const QString &status,
                           QString *errorMessage)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        if (errorMessage) *errorMessage = db.lastError().text();
        return false;
    }

    int clientId = -1;
    QSqlQuery clientQuery(db);
    clientQuery.prepare("SELECT id, full_name FROM clients WHERE phone = :phone LIMIT 1");
    clientQuery.bindValue(":phone", clientPhone.trimmed());
    if (!clientQuery.exec()) {
        db.rollback();
        if (errorMessage) *errorMessage = clientQuery.lastError().text();
        return false;
    }

    if (clientQuery.next()) {
        clientId = clientQuery.value(0).toInt();
        if (clientQuery.value(1).toString() != clientName.trimmed()) {
            QSqlQuery updateClient(db);
            updateClient.prepare("UPDATE clients SET full_name = :name WHERE id = :id");
            updateClient.bindValue(":name", clientName.trimmed());
            updateClient.bindValue(":id", clientId);
            if (!updateClient.exec()) {
                db.rollback();
                if (errorMessage) *errorMessage = updateClient.lastError().text();
                return false;
            }
        }
    } else {
        QSqlQuery insertClient(db);
        insertClient.prepare("INSERT INTO clients (full_name, phone) VALUES (:name, :phone)");
        insertClient.bindValue(":name", clientName.trimmed());
        insertClient.bindValue(":phone", clientPhone.trimmed());
        if (!insertClient.exec()) {
            db.rollback();
            if (errorMessage) *errorMessage = insertClient.lastError().text();
            return false;
        }
        clientId = insertClient.lastInsertId().toInt();
    }

    int carId = -1;
    QSqlQuery carQuery(db);
    carQuery.prepare("SELECT id FROM cars WHERE plate_number = :plate AND client_id = :clientId LIMIT 1");
    carQuery.bindValue(":plate", plateNumber.trimmed().toUpper());
    carQuery.bindValue(":clientId", clientId);
    if (!carQuery.exec()) {
        db.rollback();
        if (errorMessage) *errorMessage = carQuery.lastError().text();
        return false;
    }

    if (carQuery.next()) {
        carId = carQuery.value(0).toInt();
        QSqlQuery updateCar(db);
        updateCar.prepare("UPDATE cars SET brand = :brand, model = :model WHERE id = :carId");
        updateCar.bindValue(":brand", carBrand.trimmed());
        updateCar.bindValue(":model", carModel.trimmed());
        updateCar.bindValue(":carId", carId);
        if (!updateCar.exec()) {
            db.rollback();
            if (errorMessage) *errorMessage = updateCar.lastError().text();
            return false;
        }
    } else {
        QSqlQuery insertCar(db);
        insertCar.prepare("INSERT INTO cars (client_id, brand, model, plate_number) VALUES (:clientId, :brand, :model, :plate)");
        insertCar.bindValue(":clientId", clientId);
        insertCar.bindValue(":brand", carBrand.trimmed());
        insertCar.bindValue(":model", carModel.trimmed());
        insertCar.bindValue(":plate", plateNumber.trimmed().toUpper());
        if (!insertCar.exec()) {
            db.rollback();
            if (errorMessage) *errorMessage = insertCar.lastError().text();
            return false;
        }
        carId = insertCar.lastInsertId().toInt();
    }

    QString dateCreated = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    QSqlQuery orderQuery(db);
    orderQuery.prepare("INSERT INTO orders (car_id, description, status, cost, date_created) "
                       "VALUES (:carId, :desc, :status, :cost, :date)");
    orderQuery.bindValue(":carId", carId);
    orderQuery.bindValue(":desc", description.trimmed());
    orderQuery.bindValue(":status", status.trimmed());
    orderQuery.bindValue(":cost", cost);
    orderQuery.bindValue(":date", dateCreated);

    if (!orderQuery.exec()) {
        db.rollback();
        if (errorMessage) *errorMessage = orderQuery.lastError().text();
        return false;
    }

    if (!db.commit()) {
        db.rollback();
        if (errorMessage) *errorMessage = db.lastError().text();
        return false;
    }

    return true;
}

bool Database::updateOrderStatus(int orderId, const QString &newStatus, QString *errorMessage)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare("UPDATE orders SET status = :status WHERE id = :id");
    query.bindValue(":status", newStatus.trimmed());
    query.bindValue(":id", orderId);
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

bool Database::deleteOrder(int orderId, QString *errorMessage)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare("DELETE FROM orders WHERE id = :id");
    query.bindValue(":id", orderId);
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

QSqlQuery Database::getAllOrdersQuery() const
{
    return getFilteredOrdersQuery(QString(), QString());
}

QSqlQuery Database::getFilteredOrdersQuery(const QString &statusFilter, const QString &searchQuery) const
{
    QSqlDatabase db = QSqlDatabase::database();
    QString sql = "SELECT "
                  "orders.id, "
                  "orders.date_created, "
                  "clients.full_name, "
                  "clients.phone, "
                  "cars.brand, "
                  "cars.model, "
                  "cars.plate_number, "
                  "orders.description, "
                  "orders.cost, "
                  "orders.status "
                  "FROM orders "
                  "JOIN cars ON orders.car_id = cars.id "
                  "JOIN clients ON cars.client_id = clients.id ";

    QStringList conditions;
    if (!statusFilter.isEmpty() && statusFilter != "Все") {
        conditions.append("orders.status = :status");
    }
    if (!searchQuery.trimmed().isEmpty()) {
        conditions.append("(cars.plate_number LIKE :search OR clients.full_name LIKE :search)");
    }

    if (!conditions.isEmpty()) {
        sql += " WHERE " + conditions.join(" AND ");
    }
    sql += " ORDER BY orders.id DESC";

    QSqlQuery query(db);
    query.prepare(sql);
    if (!statusFilter.isEmpty() && statusFilter != "Все") {
        query.bindValue(":status", statusFilter);
    }
    if (!searchQuery.trimmed().isEmpty()) {
        query.bindValue(":search", "%" + searchQuery.trimmed() + "%");
    }
    query.exec();
    return query;
}
