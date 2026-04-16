#include "positionmanagementdialog.h"
#include "databasemanager.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PositionManagementDialog::PositionManagementDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    setWindowTitle("Управление должностями");
    resize(560, 420);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    auto *titleLabel = new QLabel("Список должностей", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    positionsListWidget = new QListWidget(this);
    positionsListWidget->setAlternatingRowColors(true);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);

    auto *addButton = new QPushButton("Добавить должность", this);
    auto *editButton = new QPushButton("Редактировать должность", this);
    auto *deleteButton = new QPushButton("Удалить должность", this);
    auto *closeButton = new QPushButton("Закрыть", this);

    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(editButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(closeButton);

    connect(addButton, &QPushButton::clicked, this, &PositionManagementDialog::addPosition);
    connect(editButton, &QPushButton::clicked, this, &PositionManagementDialog::editPosition);
    connect(deleteButton, &QPushButton::clicked, this, &PositionManagementDialog::deletePosition);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(positionsListWidget);
    mainLayout->addLayout(buttonsLayout);

    loadPositions();
}

void PositionManagementDialog::loadPositions()
{
    positionsListWidget->clear();

    QSqlQuery query = DatabaseManager::instance().getPositions(currentBusinessId, true);
    while (query.next())
    {
        auto *item = new QListWidgetItem(query.value("name").toString());
        item->setData(Qt::UserRole, query.value("id").toInt());
        positionsListWidget->addItem(item);
    }
}

void PositionManagementDialog::addPosition()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        "Добавление должности",
        "Название должности:",
        QLineEdit::Normal,
        "",
        &ok
        ).trimmed();

    if (!ok || name.isEmpty())
        return;

    if (!DatabaseManager::instance().createPosition(currentBusinessId, name))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить должность.");
        return;
    }

    loadPositions();
}

void PositionManagementDialog::editPosition()
{
    QListWidgetItem *item = currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Должности", "Сначала выберите должность для редактирования.");
        return;
    }

    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        "Редактирование должности",
        "Название должности:",
        QLineEdit::Normal,
        item->text(),
        &ok
        ).trimmed();

    if (!ok || name.isEmpty())
        return;

    if (!DatabaseManager::instance().updatePosition(item->data(Qt::UserRole).toInt(), name))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось изменить должность.");
        return;
    }

    loadPositions();
}

void PositionManagementDialog::deletePosition()
{
    QListWidgetItem *item = currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Должности", "Сначала выберите должность для удаления.");
        return;
    }

    const auto reply = QMessageBox::question(
        this,
        "Удаление должности",
        QString("Удалить должность \"%1\"?").arg(item->text())
        );

    if (reply != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deletePosition(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить должность.");
        return;
    }

    loadPositions();
}

QListWidgetItem *PositionManagementDialog::currentItem() const
{
    return positionsListWidget->currentItem();
}
