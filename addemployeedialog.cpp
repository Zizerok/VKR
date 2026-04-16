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

AddEmployeeDialog::AddEmployeeDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    setWindowTitle("Добавление сотрудника");
    resize(760, 520);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Новый сотрудник", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(24);

    auto *photoLayout = new QVBoxLayout();
    photoLayout->setSpacing(12);

    auto *photoFrame = new QFrame(this);
    photoFrame->setFixedSize(220, 280);
    photoFrame->setFrameShape(QFrame::StyledPanel);
    photoFrame->setStyleSheet("background-color: white;");

    auto *photoFrameLayout = new QVBoxLayout(photoFrame);
    photoFrameLayout->setContentsMargins(16, 16, 16, 16);
    photoFrameLayout->setSpacing(12);

    auto *photoPlaceholder = new QLabel("Фото\nсотрудника", photoFrame);
    photoPlaceholder->setAlignment(Qt::AlignCenter);
    photoPlaceholder->setStyleSheet(
        "border: 2px dashed #bdbdbd;"
        "background-color: #fafafa;"
        "color: #666666;"
        "font-size: 16px;"
        "padding: 24px;");

    auto *addPhotoButton = new QPushButton("Добавить фото", photoFrame);

    photoFrameLayout->addWidget(photoPlaceholder, 1);
    photoFrameLayout->addWidget(addPhotoButton);

    photoLayout->addWidget(photoFrame);
    photoLayout->addStretch();

    auto *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignTop);
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

    birthDateEdit->setCalendarPopup(true);
    birthDateEdit->setDisplayFormat("dd.MM.yyyy");
    birthDateEdit->setDate(QDate::currentDate());

    genderComboBox->addItem("Не указан");
    genderComboBox->addItem("Мужской");
    genderComboBox->addItem("Женский");

    positionComboBox->addItem("Администратор");
    positionComboBox->addItem("Менеджер");
    positionComboBox->addItem("Сотрудник");

    lastNameEdit->setPlaceholderText("Введите фамилию");
    firstNameEdit->setPlaceholderText("Введите имя");
    middleNameEdit->setPlaceholderText("Введите отчество");
    phoneEdit->setPlaceholderText("+7 (___) ___-__-__");
    vkIdEdit->setPlaceholderText("Например: id123456789");

    formLayout->addRow("Фамилия:", lastNameEdit);
    formLayout->addRow("Имя:", firstNameEdit);
    formLayout->addRow("Отчество:", middleNameEdit);
    formLayout->addRow("Дата рождения:", birthDateEdit);
    formLayout->addRow("Пол:", genderComboBox);
    formLayout->addRow("Номер телефона:", phoneEdit);
    formLayout->addRow("ID VK:", vkIdEdit);
    formLayout->addRow("Должность:", positionComboBox);

    auto *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(18);
    rightLayout->addLayout(formLayout);
    rightLayout->addStretch();
    rightLayout->addWidget(addEmployeeButton);

    contentLayout->addLayout(photoLayout);
    contentLayout->addLayout(rightLayout, 1);

    connect(addEmployeeButton, &QPushButton::clicked, this, &AddEmployeeDialog::saveEmployee);

    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(contentLayout);
}

void AddEmployeeDialog::saveEmployee()
{
    const QString lastName = lastNameEdit->text().trimmed();
    const QString firstName = firstNameEdit->text().trimmed();

    if (lastName.isEmpty() || firstName.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Заполните как минимум фамилию и имя сотрудника.");
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
        positionComboBox->currentText()
        );

    if (!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить сотрудника в базу данных.");
        return;
    }

    accept();
}
