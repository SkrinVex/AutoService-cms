#pragma once

#include <QString>
#include <QSqlQuery>
#include <QSqlDatabase>

class Database {
public:
    static Database &instance();

    bool init(const QString &dbPath = QString());
    bool isOpen() const;

    bool createOrder(const QString &clientName,
                     const QString &clientPhone,
                     const QString &carBrand,
                     const QString &carModel,
                     const QString &plateNumber,
                     const QString &description,
                     double cost,
                     const QString &status,
                     QString *errorMessage = nullptr);

    bool updateOrderStatus(int orderId, const QString &newStatus, QString *errorMessage = nullptr);
    bool deleteOrder(int orderId, QString *errorMessage = nullptr);

    QSqlQuery getAllOrdersQuery() const;
    QSqlQuery getFilteredOrdersQuery(const QString &statusFilter = QString(),
                                     const QString &searchQuery = QString()) const;

private:
    Database() = default;
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool createTables(QString *errorMessage = nullptr);
    void seedInitialData();
};
