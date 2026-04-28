#include "addemployeedialog.h"
#include "databasemanager.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
void styleInfoBox(QMessageBox &box, const QString &buttonColor)
{
    box.setStyleSheet(QString(R"(
        QMessageBox { background: #F6F6F6; }
        QMessageBox QLabel { color: #1C1D21; font-size: 14px; }
        QMessageBox QLabel#qt_msgbox_label { font-size: 18px; font-weight: 700; }
        QMessageBox QLabel#qt_msgbox_informativelabel { color: #8181A5; font-size: 13px; min-width: 280px; }
        QMessageBox QPushButton {
            min-width: 120px; min-height: 40px; border-radius: 12px; border: none;
            padding: 0 14px; font-size: 14px; font-weight: 600;
            background: %1; color: #FFFFFF;
        }
    )").arg(buttonColor));
}

void showStyledWarning(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Понятно");
    styleInfoBox(box, "#F4B85E");
    box.exec();
}

void showStyledError(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Понятно");
    styleInfoBox(box, "#FF808B");
    box.exec();
}
}

AddEmployeeDialog::AddEmployeeDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    setWindowTitle("Добавление сотрудника");
    resize(860, 560);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Новый сотрудник", this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *subtitleLabel = new QLabel(
        "Заполните основные данные сотрудника. Фото пока остаётся как заглушка.",
        this);
    subtitleLabel->setObjectName("dialogSubtitleLabel");
    subtitleLabel->setWordWrap(true);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(22);

    auto *photoCard = new QFrame(this);
    photoCard->setObjectName("photoCard");
    photoCard->setFixedWidth(240);
    auto *photoLayout = new QVBoxLayout(photoCard);
    photoLayout->setContentsMargins(18, 18, 18, 18);
    photoLayout->setSpacing(14);

    auto *photoPlaceholder = new QLabel("Фото\nсотрудника", photoCard);
    photoPlaceholder->setObjectName("photoPlaceholder");
    photoPlaceholder->setAlignment(Qt::AlignCenter);
    photoPlaceholder->setMinimumHeight(280);

    auto *addPhotoButton = new QPushButton("Добавить фото", photoCard);
    addPhotoButton->setObjectName("secondaryButton");

    photoLayout->addWidget(photoPlaceholder, 1);
    photoLayout->addWidget(addPhotoButton);

    auto *formCard = new QFrame(this);
    formCard->setObjectName("formCard");
    auto *formCardLayout = new QVBoxLayout(formCard);
    formCardLayout->setContentsMargins(20, 20, 20, 20);
    formCardLayout->setSpacing(16);

    auto *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(14);

    lastNameEdit = new QLineEdit(this);
    firstNameEdit = new QLineEdit(this);
    middleNameEdit = new QLineEdit(this);
    birthDateEdit = new QDateEdit(this);
    genderComboBox = new QComboBox(this);
    phoneEdit = new QLineEdit(this);
    vkIdEdit = new QLineEdit(this);
    positionComboBox = new QComboBox(this);
    addEmployeeButton = new QPushButton("Добавить сотрудника", this);

    lastNameEdit->setPlaceholderText("Фамилия");
    firstNameEdit->setPlaceholderText("Имя");
    middleNameEdit->setPlaceholderText("Отчество");
    phoneEdit->setPlaceholderText("+7 (___) ___-__-__");
    vkIdEdit->setPlaceholderText("Например: 215882848");

    birthDateEdit->setCalendarPopup(true);
    birthDateEdit->setDisplayFormat("dd.MM.yyyy");
    birthDateEdit->setDate(QDate::currentDate());

    genderComboBox->addItems({"Не указан", "Мужской", "Женский"});

    positionComboBox->addItem("Не выбрана");
    const QStringList positions = DatabaseManager::instance().getPositionNames(currentBusinessId);
    for (const QString &position : positions)
        positionComboBox->addItem(position);

    formLayout->addRow("Фамилия", lastNameEdit);
    formLayout->addRow("Имя", firstNameEdit);
    formLayout->addRow("Отчество", middleNameEdit);
    formLayout->addRow("Дата рождения", birthDateEdit);
    formLayout->addRow("Пол", genderComboBox);
    formLayout->addRow("Телефон", phoneEdit);
    formLayout->addRow("VK ID", vkIdEdit);
    formLayout->addRow("Должность", positionComboBox);

    addEmployeeButton->setObjectName("primaryButton");

    formCardLayout->addLayout(formLayout);
    formCardLayout->addStretch();
    formCardLayout->addWidget(addEmployeeButton);

    contentLayout->addWidget(photoCard);
    contentLayout->addWidget(formCard, 1);

    connect(addEmployeeButton, &QPushButton::clicked, this, &AddEmployeeDialog::saveEmployee);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addLayout(contentLayout);

    setStyleSheet(R"(
        QDialog {
            background: #F6F6F6;
        }
        QLabel#dialogTitleLabel {
            color: #1C1D21;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#dialogSubtitleLabel {
            color: #8181A5;
            font-size: 14px;
        }
        QFrame#photoCard, QFrame#formCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLabel#photoPlaceholder {
            background: #FAFBFF;
            border: 1px dashed #D6DBEE;
            border-radius: 18px;
            color: #8181A5;
            font-size: 18px;
            font-weight: 600;
        }
        QLineEdit, QComboBox, QDateEdit {
            min-height: 40px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0 12px;
            color: #1C1D21;
            font-size: 14px;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus {
            border: 1px solid #5E81F4;
        }
        QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
        QPushButton {
            min-height: 42px;
            border: none;
            border-radius: 14px;
            padding: 0 16px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#primaryButton {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton#secondaryButton {
            background: #E9EDFB;
            color: #5E81F4;
        }
    )");
}

void AddEmployeeDialog::saveEmployee()
{
    const QString lastName = lastNameEdit->text().trimmed();
    const QString firstName = firstNameEdit->text().trimmed();

    if (lastName.isEmpty() || firstName.isEmpty())
    {
        showStyledWarning(this, "Ошибка", "Заполните как минимум фамилию и имя сотрудника.");
        return;
    }

    const bool ok = DatabaseManager::instance().createEmployee(
        currentBusinessId,
        lastName,
        firstName,
        middleNameEdit->text(),
        birthDateEdit->date(),
        genderComboBox->currentText(),
        phoneEdit->text(),
        vkIdEdit->text(),
        positionComboBox->currentText());

    if (!ok)
    {
        showStyledError(this, "Ошибка", "Не удалось сохранить сотрудника в базу данных.");
        return;
    }

    accept();
}
