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

namespace
{
void styleMessage(QMessageBox &box, const QString &buttonColor)
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

void showEmployeeError(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Понятно");
    styleMessage(box, "#FF808B");
    box.exec();
}

void showEmployeeWarning(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Понятно");
    styleMessage(box, "#F4B85E");
    box.exec();
}

void showEmployeeInfo(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Понятно");
    styleMessage(box, "#5E81F4");
    box.exec();
}
}

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
    resize(1080, 820);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *central = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    auto *headerFrame = new QFrame(central);
    headerFrame->setObjectName("headerCard");
    auto *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 20, 20, 20);
    headerLayout->setSpacing(20);

    photoLabel = new QLabel("Фото\nсотрудника", headerFrame);
    photoLabel->setObjectName("photoPlaceholder");
    photoLabel->setFixedSize(180, 220);
    photoLabel->setAlignment(Qt::AlignCenter);

    auto *headerInfoLayout = new QVBoxLayout();
    headerInfoLayout->setSpacing(10);

    headerNameLabel = new QLabel("Сотрудник", headerFrame);
    headerNameLabel->setObjectName("headerNameLabel");

    headerPositionLabel = new QLabel("Должность", headerFrame);
    headerPositionLabel->setObjectName("headerPositionLabel");

    auto *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(10);
    auto *statusTitle = new QLabel("Статус", headerFrame);
    statusTitle->setObjectName("fieldLabel");
    statusComboBox = new QComboBox(headerFrame);
    statusComboBox->addItems({"Активен", "Неактивен"});
    statusLayout->addWidget(statusTitle);
    statusLayout->addWidget(statusComboBox);
    statusLayout->addStretch();

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);
    editButton = new QPushButton("Редактировать", headerFrame);
    editButton->setObjectName("secondaryButton");
    saveButton = new QPushButton("Сохранить", headerFrame);
    saveButton->setObjectName("primaryButton");
    buttonsLayout->addWidget(editButton);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addStretch();

    coveredPositionsLabel = new QLabel("-", headerFrame);
    coveredPositionsLabel->setObjectName("badgeInfoLabel");
    coveredPositionsLabel->setWordWrap(true);

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
    headerInfoLayout->addWidget(coveredPositionsLabel);
    headerInfoLayout->addStretch();

    headerLayout->addWidget(photoLabel);
    headerLayout->addLayout(headerInfoLayout, 1);

    auto *mainContentLayout = new QHBoxLayout();
    mainContentLayout->setSpacing(20);

    auto *basicFrame = new QFrame(central);
    basicFrame->setObjectName("sectionCard");
    auto *basicLayout = new QVBoxLayout(basicFrame);
    basicLayout->setContentsMargins(20, 20, 20, 20);
    basicLayout->setSpacing(14);
    auto *basicTitle = new QLabel("Основные данные", basicFrame);
    basicTitle->setObjectName("sectionTitleLabel");

    auto *basicForm = new QFormLayout();
    basicForm->setHorizontalSpacing(14);
    basicForm->setVerticalSpacing(14);

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

    basicForm->addRow("Фамилия", lastNameEdit);
    basicForm->addRow("Имя", firstNameEdit);
    basicForm->addRow("Отчество", middleNameEdit);
    basicForm->addRow("Дата рождения", birthDateEdit);
    basicForm->addRow("Пол", genderComboBox);
    basicForm->addRow("Телефон", phoneEdit);
    basicForm->addRow("VK ID", vkIdEdit);
    basicForm->addRow("Должность", positionComboBox);

    basicLayout->addWidget(basicTitle);
    basicLayout->addLayout(basicForm);

    auto *workFrame = new QFrame(central);
    workFrame->setObjectName("sectionCard");
    auto *workLayout = new QVBoxLayout(workFrame);
    workLayout->setContentsMargins(20, 20, 20, 20);
    workLayout->setSpacing(14);
    auto *workTitle = new QLabel("Рабочая информация", workFrame);
    workTitle->setObjectName("sectionTitleLabel");

    auto *workForm = new QFormLayout();
    workForm->setHorizontalSpacing(14);
    workForm->setVerticalSpacing(14);

    hiredDateEdit = new QDateEdit(workFrame);
    hiredDateEdit->setCalendarPopup(true);
    hiredDateEdit->setDisplayFormat("dd.MM.yyyy");
    commentEdit = new QTextEdit(workFrame);
    commentEdit->setMinimumHeight(120);
    salaryRateEdit = new QLineEdit(workFrame);

    workForm->addRow("Дата приёма", hiredDateEdit);
    workForm->addRow("Комментарий", commentEdit);
    workForm->addRow("Оклад или ставка", salaryRateEdit);

    workLayout->addWidget(workTitle);
    workLayout->addLayout(workForm);

    mainContentLayout->addWidget(basicFrame, 1);
    mainContentLayout->addWidget(workFrame, 1);

    mainLayout->addWidget(headerFrame);
    mainLayout->addLayout(mainContentLayout);
    mainLayout->addStretch();

    scrollArea->setWidget(central);
    setCentralWidget(scrollArea);

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #F6F6F6;
        }
        QFrame#headerCard, QFrame#sectionCard {
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
        QLabel#headerNameLabel {
            color: #1C1D21;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#headerPositionLabel {
            color: #8181A5;
            font-size: 15px;
            font-weight: 600;
        }
        QLabel#sectionTitleLabel {
            color: #1C1D21;
            font-size: 18px;
            font-weight: 700;
        }
        QLabel#fieldLabel {
            color: #8181A5;
            font-size: 13px;
            font-weight: 600;
        }
        QLabel#badgeInfoLabel {
            background: #FAFBFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            padding: 10px 12px;
            color: #1C1D21;
            font-size: 13px;
        }
        QLineEdit, QComboBox, QDateEdit, QTextEdit {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 8px 12px;
            color: #1C1D21;
            font-size: 14px;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QTextEdit:focus {
            border: 1px solid #5E81F4;
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
        QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
        QScrollArea {
            border: none;
        }
    )");
}

void EmployeeCardWindow::loadEmployee()
{
    QSqlQuery query = DatabaseManager::instance().getEmployeeById(currentEmployeeId);
    if (!query.next())
    {
        showEmployeeError(this, "Ошибка", "Не удалось загрузить данные сотрудника.");
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

    connect(positionComboBox, &QComboBox::currentTextChanged, this, [this](const QString &) {
        updateHeader();
    });
    connect(lastNameEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        updateHeader();
    });
    connect(firstNameEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        updateHeader();
    });
    connect(middleNameEdit, &QLineEdit::textChanged, this, [this](const QString &) {
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
    coveredPositionsLabel->setText(
        coveredPositions.isEmpty()
            ? "Может выполнять: -"
            : QString("Может выполнять: %1").arg(coveredPositions.join(", ")));
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
        showEmployeeWarning(this, "Ошибка", "Фамилия и имя сотрудника обязательны.");
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
        salaryRateEdit->text());

    if (!ok)
    {
        showEmployeeError(this, "Ошибка", "Не удалось сохранить карточку сотрудника.");
        return;
    }

    updateHeader();
    setEditMode(false);
    showEmployeeInfo(this, "Сохранено", "Данные сотрудника обновлены.");
}
