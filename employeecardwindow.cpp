#include "employeecardwindow.h"
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
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

EmployeeCardWindow::EmployeeCardWindow(int employeeId, QWidget *parent)
    : QMainWindow(parent)
    , currentEmployeeId(employeeId)
{
    buildUi();
    loadEmployee();
    setEditMode(false);
}

void EmployeeCardWindow::buildUi()
{
    setWindowTitle("Карточка сотрудника");
    resize(980, 760);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    auto *central = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    auto *headerFrame = new QFrame(central);
    auto *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setSpacing(20);

    photoLabel = new QLabel("Фото\nсотрудника", headerFrame);
    photoLabel->setFixedSize(180, 220);
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setStyleSheet(
        "background-color: white;"
        "border: 1px solid #cfcfcf;"
        "font-size: 16px;"
        "color: #666666;");

    auto *headerInfoLayout = new QVBoxLayout();
    headerInfoLayout->setSpacing(10);

    headerNameLabel = new QLabel("Сотрудник", headerFrame);
    QFont nameFont = headerNameLabel->font();
    nameFont.setPointSize(20);
    nameFont.setBold(true);
    headerNameLabel->setFont(nameFont);

    headerPositionLabel = new QLabel("Должность", headerFrame);
    QFont positionFont = headerPositionLabel->font();
    positionFont.setPointSize(13);
    headerPositionLabel->setFont(positionFont);

    auto *statusLayout = new QHBoxLayout();
    auto *statusTitle = new QLabel("Статус:", headerFrame);
    statusComboBox = new QComboBox(headerFrame);
    statusComboBox->addItem("Активен");
    statusComboBox->addItem("Неактивен");
    statusLayout->addWidget(statusTitle);
    statusLayout->addWidget(statusComboBox);
    statusLayout->addStretch();

    auto *buttonsLayout = new QHBoxLayout();
    editButton = new QPushButton("Редактировать", headerFrame);
    saveButton = new QPushButton("Сохранить", headerFrame);
    buttonsLayout->addWidget(editButton);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addStretch();

    connect(editButton, &QPushButton::clicked, this, [this]() {
        setEditMode(true);
    });
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        saveEmployee();
    });

    headerInfoLayout->addWidget(headerNameLabel);
    headerInfoLayout->addWidget(headerPositionLabel);
    headerInfoLayout->addLayout(statusLayout);
    headerInfoLayout->addLayout(buttonsLayout);
    headerInfoLayout->addStretch();

    headerLayout->addWidget(photoLabel);
    headerLayout->addLayout(headerInfoLayout, 1);

    auto *mainContentLayout = new QHBoxLayout();
    mainContentLayout->setSpacing(20);

    auto *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(18);
    auto *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(18);

    auto *basicFrame = new QFrame(central);
    auto *basicLayout = new QVBoxLayout(basicFrame);
    auto *basicTitle = new QLabel("Основные данные", basicFrame);
    QFont sectionFont = basicTitle->font();
    sectionFont.setPointSize(15);
    sectionFont.setBold(true);
    basicTitle->setFont(sectionFont);

    auto *basicForm = new QFormLayout();
    basicForm->setHorizontalSpacing(14);
    basicForm->setVerticalSpacing(12);

    lastNameEdit = new QLineEdit(basicFrame);
    firstNameEdit = new QLineEdit(basicFrame);
    middleNameEdit = new QLineEdit(basicFrame);
    birthDateEdit = new QDateEdit(basicFrame);
    genderComboBox = new QComboBox(basicFrame);
    phoneEdit = new QLineEdit(basicFrame);
    vkIdEdit = new QLineEdit(basicFrame);
    positionComboBox = new QComboBox(basicFrame);

    birthDateEdit->setCalendarPopup(true);
    birthDateEdit->setDisplayFormat("dd.MM.yyyy");
    genderComboBox->addItems({"Не указан", "Мужской", "Женский"});

    basicForm->addRow("Фамилия:", lastNameEdit);
    basicForm->addRow("Имя:", firstNameEdit);
    basicForm->addRow("Отчество:", middleNameEdit);
    basicForm->addRow("Дата рождения:", birthDateEdit);
    basicForm->addRow("Пол:", genderComboBox);
    basicForm->addRow("Телефон:", phoneEdit);
    basicForm->addRow("VK ID:", vkIdEdit);
    basicForm->addRow("Должность:", positionComboBox);

    basicLayout->addWidget(basicTitle);
    basicLayout->addLayout(basicForm);

    auto *workFrame = new QFrame(central);
    auto *workLayout = new QVBoxLayout(workFrame);
    auto *workTitle = new QLabel("Рабочая информация", workFrame);
    workTitle->setFont(sectionFont);

    auto *workForm = new QFormLayout();
    workForm->setHorizontalSpacing(14);
    workForm->setVerticalSpacing(12);

    hiredDateEdit = new QDateEdit(workFrame);
    hiredDateEdit->setCalendarPopup(true);
    hiredDateEdit->setDisplayFormat("dd.MM.yyyy");
    commentEdit = new QTextEdit(workFrame);
    commentEdit->setMinimumHeight(110);
    salaryRateEdit = new QLineEdit(workFrame);
    coveredPositionsLabel = new QLabel("-", workFrame);
    coveredPositionsLabel->setWordWrap(true);

    workForm->addRow("Дата приема:", hiredDateEdit);
    workForm->addRow("Комментарий:", commentEdit);
    workForm->addRow("Оклад или ставка:", salaryRateEdit);
    workForm->addRow("Может выполнять:", coveredPositionsLabel);

    workLayout->addWidget(workTitle);
    workLayout->addLayout(workForm);

    leftColumn->addWidget(basicFrame);
    rightColumn->addWidget(workFrame);

    mainContentLayout->addLayout(leftColumn, 1);
    mainContentLayout->addLayout(rightColumn, 1);

    mainLayout->addWidget(headerFrame);
    mainLayout->addLayout(mainContentLayout);
    mainLayout->addStretch();

    scrollArea->setWidget(central);
    setCentralWidget(scrollArea);
}

void EmployeeCardWindow::loadEmployee()
{
    QSqlQuery query = DatabaseManager::instance().getEmployeeById(currentEmployeeId);
    if (!query.next())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные сотрудника.");
        close();
        return;
    }

    currentBusinessId = query.value("business_id").toInt();
    loadPositions();

    lastNameEdit->setText(query.value("last_name").toString());
    firstNameEdit->setText(query.value("first_name").toString());
    middleNameEdit->setText(query.value("middle_name").toString());

    const QString birthDateText = query.value("birth_date").toString();
    birthDateEdit->setDate(birthDateText.isEmpty() ? QDate::currentDate() : QDate::fromString(birthDateText, Qt::ISODate));

    const QString gender = query.value("gender").toString();
    const int genderIndex = genderComboBox->findText(gender);
    if (genderIndex >= 0)
        genderComboBox->setCurrentIndex(genderIndex);

    phoneEdit->setText(query.value("phone").toString());
    vkIdEdit->setText(query.value("vk_id").toString());

    const QString position = query.value("position").toString();
    const int positionIndex = positionComboBox->findText(position);
    if (positionIndex >= 0)
        positionComboBox->setCurrentIndex(positionIndex);

    statusComboBox->setCurrentIndex(query.value("is_active").toInt() == 0 ? 1 : 0);

    const QString hiredDateText = query.value("hired_date").toString();
    hiredDateEdit->setDate(hiredDateText.isEmpty() ? QDate::currentDate() : QDate::fromString(hiredDateText, Qt::ISODate));

    commentEdit->setPlainText(query.value("comment").toString());
    salaryRateEdit->setText(query.value("salary_rate").toString());

    updateHeader();

    connect(positionComboBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        updateHeader();
    });
    connect(lastNameEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        updateHeader();
    });
    connect(firstNameEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        updateHeader();
    });
    connect(middleNameEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        updateHeader();
    });
}

void EmployeeCardWindow::loadPositions()
{
    positionComboBox->clear();

    QSqlQuery query = DatabaseManager::instance().getPositions(currentBusinessId, true);
    while (query.next())
        positionComboBox->addItem(query.value("name").toString());
}

void EmployeeCardWindow::updateHeader()
{
    const QString fullName = QString("%1 %2 %3")
                                 .arg(lastNameEdit->text().trimmed(),
                                      firstNameEdit->text().trimmed(),
                                      middleNameEdit->text().trimmed())
                                 .simplified();
    headerNameLabel->setText(fullName.isEmpty() ? "Сотрудник" : fullName);

    const QString position = positionComboBox->currentText();
    headerPositionLabel->setText(position.isEmpty() ? "Должность не указана" : position);

    const QStringList coveredPositions =
        DatabaseManager::instance().getCoveredPositionNamesByPositionName(currentBusinessId, position);
    coveredPositionsLabel->setText(coveredPositions.isEmpty() ? "-" : coveredPositions.join(", "));
}

void EmployeeCardWindow::setEditMode(bool enabled)
{
    editMode = enabled;

    lastNameEdit->setReadOnly(!enabled);
    firstNameEdit->setReadOnly(!enabled);
    middleNameEdit->setReadOnly(!enabled);
    birthDateEdit->setEnabled(enabled);
    genderComboBox->setEnabled(enabled);
    phoneEdit->setReadOnly(!enabled);
    vkIdEdit->setReadOnly(!enabled);
    positionComboBox->setEnabled(enabled);
    statusComboBox->setEnabled(enabled);
    hiredDateEdit->setEnabled(enabled);
    commentEdit->setReadOnly(!enabled);
    salaryRateEdit->setReadOnly(!enabled);

    saveButton->setEnabled(enabled);
    editButton->setEnabled(!enabled);
}

void EmployeeCardWindow::saveEmployee()
{
    if (lastNameEdit->text().trimmed().isEmpty() || firstNameEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Фамилия и имя сотрудника обязательны.");
        return;
    }

    const bool ok = DatabaseManager::instance().updateEmployee(
        currentEmployeeId,
        lastNameEdit->text(),
        firstNameEdit->text(),
        middleNameEdit->text(),
        birthDateEdit->date(),
        genderComboBox->currentText(),
        phoneEdit->text(),
        vkIdEdit->text(),
        positionComboBox->currentText(),
        statusComboBox->currentIndex() == 0,
        hiredDateEdit->date(),
        commentEdit->toPlainText(),
        salaryRateEdit->text()
        );

    if (!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить карточку сотрудника.");
        return;
    }

    updateHeader();
    setEditMode(false);
    QMessageBox::information(this, "Сохранено", "Данные сотрудника обновлены.");
}
