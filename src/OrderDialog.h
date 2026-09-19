#pragma once

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QDoubleSpinBox;
class QComboBox;

class OrderDialog : public QDialog {
    Q_OBJECT

public:
    explicit OrderDialog(QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    void setupUi();

    QLineEdit *m_clientNameEdit = nullptr;
    QLineEdit *m_clientPhoneEdit = nullptr;
    QLineEdit *m_carBrandEdit = nullptr;
    QLineEdit *m_carModelEdit = nullptr;
    QLineEdit *m_plateNumberEdit = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
    QDoubleSpinBox *m_costSpinBox = nullptr;
    QComboBox *m_statusCombo = nullptr;
};
