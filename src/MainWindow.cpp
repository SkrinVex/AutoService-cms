#include "MainWindow.h"
#include "OrderDialog.h"
#include "Database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QSqlRecord>
#include <QStringConverter>
#include <QBrush>
#include <QColor>

OrdersModel::OrdersModel(QObject *parent)
    : QSqlQueryModel(parent)
{
}

QVariant OrdersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole && index.column() == 8) {
        double cost = QSqlQueryModel::data(index, Qt::DisplayRole).toDouble();
        return QString::number(cost, 'f', 2) + " руб.";
    }

    if (role == Qt::TextAlignmentRole) {
        int col = index.column();
        if (col == 0 || col == 1 || col == 3 || col == 6 || col == 9) {
            return Qt::AlignCenter;
        }
        if (col == 8) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        }
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::ForegroundRole && index.column() == 9) {
        QString status = QSqlQueryModel::data(index, Qt::DisplayRole).toString();
        if (status == "Принят") return QBrush(QColor(30, 80, 180));
        if (status == "В работе") return QBrush(QColor(190, 100, 10));
        if (status == "Готов к выдаче") return QBrush(QColor(20, 140, 40));
        if (status == "Закрыт") return QBrush(QColor(110, 110, 110));
    }

    return QSqlQueryModel::data(index, role);
}

OrderFilterProxyModel::OrderFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void OrderFilterProxyModel::setStatusFilter(const QString &status)
{
    m_statusFilter = status;
    invalidate();
}

void OrderFilterProxyModel::setSearchText(const QString &text)
{
    m_searchText = text.trimmed();
    invalidate();
}

bool OrderFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_statusFilter.isEmpty() && m_statusFilter != "Все") {
        QModelIndex statusIdx = sourceModel()->index(sourceRow, 9, sourceParent);
        QString status = sourceModel()->data(statusIdx).toString();
        if (status != m_statusFilter) {
            return false;
        }
    }

    if (!m_searchText.isEmpty()) {
        QModelIndex clientIdx = sourceModel()->index(sourceRow, 2, sourceParent);
        QModelIndex plateIdx = sourceModel()->index(sourceRow, 6, sourceParent);
        QString client = sourceModel()->data(clientIdx).toString();
        QString plate = sourceModel()->data(plateIdx).toString();

        bool matchClient = client.contains(m_searchText, Qt::CaseInsensitive);
        bool matchPlate = plate.contains(m_searchText, Qt::CaseInsensitive);
        if (!matchClient && !matchPlate) {
            return false;
        }
    }

    return true;
}

bool OrderFilterProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    if (sourceLeft.column() == 0) {
        int leftId = sourceModel()->data(sourceLeft, Qt::DisplayRole).toInt();
        int rightId = sourceModel()->data(sourceRight, Qt::DisplayRole).toInt();
        return leftId < rightId;
    }
    if (sourceLeft.column() == 8) {
        const auto *sqlModel = qobject_cast<const QSqlQueryModel *>(sourceModel());
        if (sqlModel) {
            double leftCost = sqlModel->record(sourceLeft.row()).value("cost").toDouble();
            double rightCost = sqlModel->record(sourceRight.row()).value("cost").toDouble();
            return leftCost < rightCost;
        }
    }
    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    refreshTable();
}

void MainWindow::setupUi()
{
    setWindowTitle("Автосервис - Управление заказами");
    resize(1180, 680);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(8);

    QLabel *searchLabel = new QLabel("Поиск:", central);
    m_searchEdit = new QLineEdit(central);
    m_searchEdit->setPlaceholderText("Госномер или ФИО клиента...");
    m_searchEdit->setClearButtonEnabled(true);

    QLabel *statusLabel = new QLabel("Статус:", central);
    m_statusFilterCombo = new QComboBox(central);
    m_statusFilterCombo->addItems({"Все", "Принят", "В работе", "Готов к выдаче", "Закрыт"});

    QPushButton *resetBtn = new QPushButton("Сбросить фильтры", central);

    filterLayout->addWidget(searchLabel);
    filterLayout->addWidget(m_searchEdit, 2);
    filterLayout->addWidget(statusLabel);
    filterLayout->addWidget(m_statusFilterCombo, 1);
    filterLayout->addWidget(resetBtn);

    mainLayout->addLayout(filterLayout);

    m_tableView = new QTableView(central);
    m_ordersModel = new OrdersModel(this);
    m_proxyModel = new OrderFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_ordersModel);

    m_tableView->setModel(m_proxyModel);
    m_tableView->setSortingEnabled(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setHighlightSections(false);
    m_tableView->horizontalHeader()->setStretchLastSection(false);

    mainLayout->addWidget(m_tableView);

    QHBoxLayout *actionsLayout = new QHBoxLayout;
    actionsLayout->setSpacing(10);

    QPushButton *createBtn = new QPushButton("Создать заказ", central);
    QPushButton *changeStatusBtn = new QPushButton("Сменить статус", central);
    QPushButton *deleteBtn = new QPushButton("Удалить заказ", central);
    QPushButton *exportBtn = new QPushButton("Выгрузить отчёт (CSV)", central);

    actionsLayout->addWidget(createBtn);
    actionsLayout->addWidget(changeStatusBtn);
    actionsLayout->addWidget(deleteBtn);
    actionsLayout->addStretch();
    actionsLayout->addWidget(exportBtn);

    mainLayout->addLayout(actionsLayout);

    m_statusSummaryLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusSummaryLabel);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(m_statusFilterCombo, &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::onResetFiltersClicked);

    connect(createBtn, &QPushButton::clicked, this, &MainWindow::onCreateOrderClicked);
    connect(changeStatusBtn, &QPushButton::clicked, this, &MainWindow::onChangeStatusClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteOrderClicked);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportCsvClicked);

    connect(m_tableView, &QTableView::doubleClicked, this, &MainWindow::onChangeStatusClicked);
}

void MainWindow::refreshTable()
{
    m_ordersModel->setQuery(Database::instance().getAllOrdersQuery());
    while (m_ordersModel->canFetchMore()) {
        m_ordersModel->fetchMore();
    }

    m_ordersModel->setHeaderData(0, Qt::Horizontal, "№ Заказа");
    m_ordersModel->setHeaderData(1, Qt::Horizontal, "Дата создания");
    m_ordersModel->setHeaderData(2, Qt::Horizontal, "Клиент");
    m_ordersModel->setHeaderData(3, Qt::Horizontal, "Телефон");
    m_ordersModel->setHeaderData(4, Qt::Horizontal, "Марка");
    m_ordersModel->setHeaderData(5, Qt::Horizontal, "Модель");
    m_ordersModel->setHeaderData(6, Qt::Horizontal, "Госномер");
    m_ordersModel->setHeaderData(7, Qt::Horizontal, "Описание работ");
    m_ordersModel->setHeaderData(8, Qt::Horizontal, "Стоимость");
    m_ordersModel->setHeaderData(9, Qt::Horizontal, "Статус");

    m_tableView->setColumnWidth(0, 75);
    m_tableView->setColumnWidth(1, 130);
    m_tableView->setColumnWidth(2, 190);
    m_tableView->setColumnWidth(3, 140);
    m_tableView->setColumnWidth(4, 95);
    m_tableView->setColumnWidth(5, 95);
    m_tableView->setColumnWidth(6, 105);
    m_tableView->setColumnWidth(8, 125);
    m_tableView->setColumnWidth(9, 125);

    m_tableView->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);

    onFilterChanged();
}

void MainWindow::onFilterChanged()
{
    QString status = m_statusFilterCombo ? m_statusFilterCombo->currentText() : "Все";
    QString search = m_searchEdit ? m_searchEdit->text() : "";

    m_proxyModel->setStatusFilter(status);
    m_proxyModel->setSearchText(search);

    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    int visibleCount = m_proxyModel->rowCount();
    int totalCount = m_ordersModel->rowCount();
    double totalCost = 0.0;

    for (int r = 0; r < visibleCount; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        totalCost += m_ordersModel->record(srcIdx.row()).value("cost").toDouble();
    }

    m_statusSummaryLabel->setText(
        QString("Показано заказов: %1 (всего: %2), Общая сумма: %3 руб.")
            .arg(visibleCount)
            .arg(totalCount)
            .arg(QString::number(totalCost, 'f', 2))
    );
}

void MainWindow::onResetFiltersClicked()
{
    m_searchEdit->clear();
    m_statusFilterCombo->setCurrentIndex(0);
}

void MainWindow::onCreateOrderClicked()
{
    OrderDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshTable();
    }
}

void MainWindow::onChangeStatusClicked()
{
    QItemSelectionModel *selModel = m_tableView->selectionModel();
    QModelIndex proxyIdx = selModel ? selModel->currentIndex() : QModelIndex();
    if (!proxyIdx.isValid() && selModel && !selModel->selectedRows().isEmpty()) {
        proxyIdx = selModel->selectedRows().first();
    }

    if (!proxyIdx.isValid()) {
        QMessageBox::information(this, "Информация", "Выберите заказ в таблице для смены статуса.");
        return;
    }

    QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
    int srcRow = srcIdx.row();
    int orderId = m_ordersModel->data(m_ordersModel->index(srcRow, 0)).toInt();
    QString currentStatus = m_ordersModel->data(m_ordersModel->index(srcRow, 9)).toString();

    QStringList statuses = {"Принят", "В работе", "Готов к выдаче", "Закрыт"};
    int currentIndex = statuses.indexOf(currentStatus);
    if (currentIndex < 0) currentIndex = 0;

    bool ok = false;
    QString newStatus = QInputDialog::getItem(
        this,
        "Смена статуса заказа",
        QString("Выберите новый статус для заказа №%1:").arg(orderId),
        statuses,
        currentIndex,
        false,
        &ok
    );

    if (ok && !newStatus.isEmpty() && newStatus != currentStatus) {
        QString error;
        if (!Database::instance().updateOrderStatus(orderId, newStatus, &error)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось изменить статус заказа: " + error);
        } else {
            refreshTable();
        }
    }
}

void MainWindow::onDeleteOrderClicked()
{
    QItemSelectionModel *selModel = m_tableView->selectionModel();
    QModelIndex proxyIdx = selModel ? selModel->currentIndex() : QModelIndex();
    if (!proxyIdx.isValid() && selModel && !selModel->selectedRows().isEmpty()) {
        proxyIdx = selModel->selectedRows().first();
    }

    if (!proxyIdx.isValid()) {
        QMessageBox::information(this, "Информация", "Выберите заказ в таблице для удаления.");
        return;
    }

    QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
    int srcRow = srcIdx.row();
    int orderId = m_ordersModel->data(m_ordersModel->index(srcRow, 0)).toInt();
    QString clientName = m_ordersModel->data(m_ordersModel->index(srcRow, 2)).toString();
    QString carInfo = m_ordersModel->data(m_ordersModel->index(srcRow, 4)).toString() + " " +
                      m_ordersModel->data(m_ordersModel->index(srcRow, 5)).toString() + " (" +
                      m_ordersModel->data(m_ordersModel->index(srcRow, 6)).toString() + ")";

    auto reply = QMessageBox::question(
        this,
        "Подтверждение удаления",
        QString("Вы действительно хотите удалить заказ №%1?\n\nКлиент: %2\nАвтомобиль: %3")
            .arg(orderId).arg(clientName).arg(carInfo),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QString error;
        if (!Database::instance().deleteOrder(orderId, &error)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось удалить заказ: " + error);
        } else {
            refreshTable();
        }
    }
}

void MainWindow::onExportCsvClicked()
{
    if (m_proxyModel->rowCount() == 0) {
        QMessageBox::information(this, "Информация", "В текущей выборке нет заказов для экспорта.");
        return;
    }

    QString defaultName = "orders_report_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm") + ".csv";
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Выгрузка отчёта в CSV",
        defaultName,
        "CSV файлы (*.csv);;Все файлы (*.*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл для записи: " + file.errorString());
        return;
    }

    file.write("\xEF\xBB\xBF");

    QTextStream out(&file);
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
    for (int col = 0; col < m_ordersModel->columnCount(); ++col) {
        headers.append(escapeCsv(m_ordersModel->headerData(col, Qt::Horizontal).toString()));
    }
    out << headers.join(";") << "\n";

    for (int proxyRow = 0; proxyRow < m_proxyModel->rowCount(); ++proxyRow) {
        QStringList rowValues;
        QModelIndex proxyRowIdx = m_proxyModel->index(proxyRow, 0);
        QModelIndex srcRowIdx = m_proxyModel->mapToSource(proxyRowIdx);
        int srcRow = srcRowIdx.row();

        for (int col = 0; col < m_ordersModel->columnCount(); ++col) {
            if (col == 8) {
                double cost = m_ordersModel->record(srcRow).value("cost").toDouble();
                rowValues.append(QString::number(cost, 'f', 2));
            } else {
                QString text = m_ordersModel->data(m_ordersModel->index(srcRow, col), Qt::DisplayRole).toString();
                rowValues.append(escapeCsv(text));
            }
        }
        out << rowValues.join(";") << "\n";
    }

    file.close();

    QMessageBox::information(
        this,
        "Экспорт завершён",
        QString("Отчёт успешно выгружен в файл:\n%1\n\nВсего записей: %2")
            .arg(filePath)
            .arg(m_proxyModel->rowCount())
    );
}
