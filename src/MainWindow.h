#pragma once

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>

class QTableView;
class QLineEdit;
class QComboBox;
class QLabel;

class OrdersModel : public QSqlQueryModel {
    Q_OBJECT
public:
    explicit OrdersModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

class OrderFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit OrderFilterProxyModel(QObject *parent = nullptr);

    void setStatusFilter(const QString &status);
    void setSearchText(const QString &text);

    QString statusFilter() const { return m_statusFilter; }
    QString searchText() const { return m_searchText; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
    QString m_statusFilter = "Все";
    QString m_searchText;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void refreshTable();
    void onFilterChanged();
    void onCreateOrderClicked();
    void onChangeStatusClicked();
    void onDeleteOrderClicked();
    void onExportCsvClicked();
    void onResetFiltersClicked();

private:
    void setupUi();
    void updateStatusBar();

    QTableView *m_tableView = nullptr;
    OrdersModel *m_ordersModel = nullptr;
    OrderFilterProxyModel *m_proxyModel = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_statusFilterCombo = nullptr;
    QLabel *m_statusSummaryLabel = nullptr;
};
