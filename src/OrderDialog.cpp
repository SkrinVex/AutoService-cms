#include "OrderDialog.h"
#include "Database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>

OrderDialog::OrderDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

void OrderDialog::setupUi()
{
    setWindowTitle("Новый заказ - Автосервис");
    setModal(true);
    resize(520, 560);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QGroupBox *clientGroup = new QGroupBox("Данные клиента", this);
    QFormLayout *clientForm = new QFormLayout(clientGroup);
    m_clientNameEdit = new QLineEdit(clientGroup);
    m_clientNameEdit->setPlaceholderText("Иванов Иван Иванович");
    m_clientPhoneEdit = new QLineEdit(clientGroup);
    m_clientPhoneEdit->setPlaceholderText("+7 (999) 123-45-67");
    clientForm->addRow("ФИО клиента *:", m_clientNameEdit);
    clientForm->addRow("Телефон *:", m_clientPhoneEdit);
    mainLayout->addWidget(clientGroup);

    QGroupBox *carGroup = new QGroupBox("Данные автомобиля", this);
    QFormLayout *carForm = new QFormLayout(carGroup);
    m_carBrandEdit = new QLineEdit(carGroup);
    m_carBrandEdit->setPlaceholderText("Toyota");
    m_carModelEdit = new QLineEdit(carGroup);
    m_carModelEdit->setPlaceholderText("Camry");
    m_plateNumberEdit = new QLineEdit(carGroup);
    m_plateNumberEdit->setPlaceholderText("А123ВС77");
    carForm->addRow("Марка *:", m_carBrandEdit);
    carForm->addRow("Модель *:", m_carModelEdit);
    carForm->addRow("Госномер *:", m_plateNumberEdit);
    mainLayout->addWidget(carGroup);

    QGroupBox *orderGroup = new QGroupBox("Информация о заказе", this);
    QFormLayout *orderForm = new QFormLayout(orderGroup);
    m_descriptionEdit = new QPlainTextEdit(orderGroup);
    m_descriptionEdit->setPlaceholderText("Перечень выполняемых работ и примечания...");
    m_descriptionEdit->setMaximumHeight(85);

    m_costSpinBox = new QDoubleSpinBox(orderGroup);
    m_costSpinBox->setRange(0.0, 10000000.0);
    m_costSpinBox->setDecimals(2);
    m_costSpinBox->setSingleStep(500.0);
    m_costSpinBox->setSuffix(" руб.");

    m_statusCombo = new QComboBox(orderGroup);
    m_statusCombo->addItems({"Принят", "В работе", "Готов к выдаче", "Закрыт"});

    orderForm->addRow("Описание работ *:", m_descriptionEdit);
    orderForm->addRow("Стоимость *:", m_costSpinBox);
    orderForm->addRow("Начальный статус:", m_statusCombo);
    mainLayout->addWidget(orderGroup);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("Отмена", this);
    QPushButton *saveBtn = new QPushButton("Сохранить", this);
    saveBtn->setDefault(true);

    buttonsLayout->addWidget(cancelBtn);
    buttonsLayout->addWidget(saveBtn);
    mainLayout->addLayout(buttonsLayout);

    connect(saveBtn, &QPushButton::clicked, this, &OrderDialog::onSaveClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void OrderDialog::onSaveClicked()
{
    QString clientName = m_clientNameEdit->text().trimmed();
    QString clientPhone = m_clientPhoneEdit->text().trimmed();
    QString carBrand = m_carBrandEdit->text().trimmed();
    QString carModel = m_carModelEdit->text().trimmed();
    QString plateNumber = m_plateNumberEdit->text().trimmed().toUpper();
    QString description = m_descriptionEdit->toPlainText().trimmed();
    double cost = m_costSpinBox->value();
    QString status = m_statusCombo->currentText();

    if (clientName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, введите ФИО клиента.");
        m_clientNameEdit->setFocus();
        return;
    }
    if (clientPhone.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, введите номер телефона клиента.");
        m_clientPhoneEdit->setFocus();
        return;
    }
    if (carBrand.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, введите марку автомобиля.");
        m_carBrandEdit->setFocus();
        return;
    }
    if (carModel.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, введите модель автомобиля.");
        m_carModelEdit->setFocus();
        return;
    }
    if (plateNumber.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, введите государственный регистрационный номер автомобиля.");
        m_plateNumberEdit->setFocus();
        return;
    }
    if (description.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, введите описание работ.");
        m_descriptionEdit->setFocus();
        return;
    }
    if (cost <= 0.0) {
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, укажите корректную стоимость работ (больше 0).");
        m_costSpinBox->setFocus();
        return;
    }

    QString errorMsg;
    bool success = Database::instance().createOrder(
        clientName,
        clientPhone,
        carBrand,
        carModel,
        plateNumber,
        description,
        cost,
        status,
        &errorMsg
    );

    if (!success) {
        QMessageBox::critical(this, "Ошибка сохранения", "Не удалось сохранить заказ в базу данных: " + errorMsg);
        return;
    }

    accept();
}
