#include "positionmanagementdialog.h"

#include "databasemanager.h"
#include "positioneditdialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
void styleMessageBox(QMessageBox& box, QMessageBox::Icon icon, const QString& title, const QString& text)
{
    box.setWindowTitle(title);
    box.setText(text);
    box.setIcon(icon);
    box.setStandardButtons(QMessageBox::Ok);
    if (box.button(QMessageBox::Ok))
        box.button(QMessageBox::Ok)->setText("Понятно");
    box.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6FB;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
            min-width: 320px;
        }
        QMessageBox QPushButton {
            min-width: 110px;
            min-height: 38px;
            border-radius: 12px;
            background: #5E81F4;
            color: white;
            border: none;
            font-weight: 600;
            padding: 0 16px;
        }
        QMessageBox QPushButton:hover {
            background: #4E73EB;
        }
    )");
}

void showInfo(QWidget *parent, const QString& title, const QString& text)
{
    QMessageBox box(parent);
    styleMessageBox(box, QMessageBox::Information, title, text);
    box.exec();
}

void showError(QWidget *parent, const QString& title, const QString& text)
{
    QMessageBox box(parent);
    styleMessageBox(box, QMessageBox::Critical, title, text);
    box.exec();
}

QMessageBox::StandardButton showQuestion(QWidget *parent, const QString& title, const QString& text)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(text);
    box.setIcon(QMessageBox::Question);
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    if (box.button(QMessageBox::Yes))
        box.button(QMessageBox::Yes)->setText("Да");
    if (box.button(QMessageBox::No))
        box.button(QMessageBox::No)->setText("Нет");
    box.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6FB;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
            min-width: 340px;
        }
        QMessageBox QPushButton {
            min-width: 110px;
            min-height: 38px;
            border-radius: 12px;
            border: none;
            font-weight: 600;
            padding: 0 16px;
        }
        QMessageBox QPushButton[text="Да"] {
            background: #5E81F4;
            color: white;
        }
        QMessageBox QPushButton[text="Нет"] {
            background: #EEF2FF;
            color: #5E81F4;
        }
    )");
    return static_cast<QMessageBox::StandardButton>(box.exec());
}
}

PositionManagementDialog::PositionManagementDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    setWindowTitle("Управление должностями");
    resize(760, 560);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Управление должностями", this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *subtitleLabel = new QLabel(
        "Создавайте роли, задавайте оклад и настраивайте должности, которые они могут замещать.",
        this);
    subtitleLabel->setObjectName("dialogSubtitleLabel");
    subtitleLabel->setWordWrap(true);

    auto *contentCard = new QFrame(this);
    contentCard->setObjectName("contentCard");
    auto *contentLayout = new QVBoxLayout(contentCard);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(14);

    auto *sectionHeader = new QHBoxLayout();
    auto *sectionLabel = new QLabel("Список должностей", this);
    sectionLabel->setObjectName("sectionTitleLabel");
    sectionHeader->addWidget(sectionLabel);
    sectionHeader->addStretch();

    positionsListWidget = new QListWidget(this);
    positionsListWidget->setAlternatingRowColors(false);
    positionsListWidget->setFocusPolicy(Qt::NoFocus);
    positionsListWidget->setSpacing(10);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);

    auto *addButton = new QPushButton("Добавить", this);
    addButton->setObjectName("primaryButton");
    auto *editButton = new QPushButton("Редактировать", this);
    editButton->setObjectName("secondaryButton");
    auto *deleteButton = new QPushButton("Удалить", this);
    deleteButton->setObjectName("secondaryButton");
    auto *closeButton = new QPushButton("Закрыть", this);
    closeButton->setObjectName("ghostButton");

    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(editButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(closeButton);

    contentLayout->addLayout(sectionHeader);
    contentLayout->addWidget(positionsListWidget, 1);
    contentLayout->addLayout(buttonsLayout);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(contentCard, 1);

    setStyleSheet(R"(
        QDialog {
            background: #F6F6FB;
        }
        QLabel#dialogTitleLabel {
            color: #1C1D21;
            font-size: 24px;
            font-weight: 700;
        }
        QLabel#dialogSubtitleLabel {
            color: #8181A5;
            font-size: 13px;
        }
        QLabel#sectionTitleLabel {
            color: #1C1D21;
            font-size: 16px;
            font-weight: 700;
        }
        QFrame#contentCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: 0;
            padding: 4px;
        }
        QListWidget::item {
            background: #F9FAFF;
            border: 1px solid #E6EAF8;
            border-radius: 16px;
            margin: 4px 0;
            padding: 14px 16px;
            color: #1C1D21;
        }
        QListWidget::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QPushButton {
            min-height: 42px;
            border-radius: 14px;
            padding: 0 18px;
            font-size: 14px;
            font-weight: 600;
            border: none;
        }
        QPushButton#primaryButton {
            background: #5E81F4;
            color: white;
        }
        QPushButton#primaryButton:hover {
            background: #4E73EB;
        }
        QPushButton#secondaryButton {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QPushButton#secondaryButton:hover,
        QPushButton#ghostButton:hover {
            background: #E3EAFE;
        }
        QPushButton#ghostButton {
            background: #F3F4F8;
            color: #8181A5;
        }
    )");

    connect(addButton, &QPushButton::clicked, this, &PositionManagementDialog::addPosition);
    connect(editButton, &QPushButton::clicked, this, &PositionManagementDialog::editPosition);
    connect(deleteButton, &QPushButton::clicked, this, &PositionManagementDialog::deletePosition);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    loadPositions();
}

void PositionManagementDialog::loadPositions()
{
    positionsListWidget->clear();

    QSqlQuery query = DatabaseManager::instance().getPositions(currentBusinessId, true);
    while (query.next())
    {
        const int positionId = query.value("id").toInt();
        const QString name = query.value("name").toString();
        const QString salary = query.value("salary_rate").toString().trimmed();
        const QStringList covered = DatabaseManager::instance().getCoveredPositionNames(positionId);

        QStringList lines;
        lines << name;
        lines << QString("Оклад: %1").arg(salary.isEmpty() ? "-" : salary);
        lines << QString("Замещает: %1").arg(covered.isEmpty() ? "-" : covered.join(", "));

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, positionId);
        positionsListWidget->addItem(item);
    }

    if (positionsListWidget->count() == 0)
    {
        auto *item = new QListWidgetItem("Пока нет созданных должностей.");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        positionsListWidget->addItem(item);
    }
}

void PositionManagementDialog::addPosition()
{
    PositionEditDialog dialog(currentBusinessId, -1, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!DatabaseManager::instance().createPosition(
            currentBusinessId,
            dialog.positionName(),
            dialog.salaryText(),
            dialog.coveredPositionIds()))
    {
        showError(this, "Ошибка", "Не удалось добавить должность.");
        return;
    }

    loadPositions();
}

void PositionManagementDialog::editPosition()
{
    QListWidgetItem *item = currentItem();
    if (!item)
    {
        showInfo(this, "Должности", "Сначала выберите должность для редактирования.");
        return;
    }

    PositionEditDialog dialog(currentBusinessId, item->data(Qt::UserRole).toInt(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!DatabaseManager::instance().updatePosition(
            item->data(Qt::UserRole).toInt(),
            dialog.positionName(),
            dialog.salaryText(),
            dialog.coveredPositionIds()))
    {
        showError(this, "Ошибка", "Не удалось изменить должность.");
        return;
    }

    loadPositions();
}

void PositionManagementDialog::deletePosition()
{
    QListWidgetItem *item = currentItem();
    if (!item)
    {
        showInfo(this, "Должности", "Сначала выберите должность для удаления.");
        return;
    }

    const auto reply = showQuestion(
        this,
        "Удаление должности",
        QString("Удалить выбранную должность?"));

    if (reply != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deletePosition(item->data(Qt::UserRole).toInt()))
    {
        showError(this, "Ошибка", "Не удалось удалить должность.");
        return;
    }

    loadPositions();
}

QListWidgetItem *PositionManagementDialog::currentItem() const
{
    QListWidgetItem *item = positionsListWidget->currentItem();
    if (!item || !item->data(Qt::UserRole).isValid())
        return nullptr;
    return item;
}
