#include "positioneditdialog.h"
#include "databasemanager.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

PositionEditDialog::PositionEditDialog(int businessId, int editingPositionId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
    , currentEditingPositionId(editingPositionId)
{
    setWindowTitle(editingPositionId > 0 ? "Редактирование должности" : "Добавление должности");
    resize(520, 420);

    auto *mainLayout = new QVBoxLayout(this);
    auto *titleLabel = new QLabel(
        editingPositionId > 0 ? "Параметры должности" : "Новая должность",
        this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(14);
    formLayout->setVerticalSpacing(12);

    nameEdit = new QLineEdit(this);
    salaryEdit = new QLineEdit(this);
    coveredPositionsList = new QListWidget(this);
    coveredPositionsList->setAlternatingRowColors(true);

    nameEdit->setPlaceholderText("Например: Бариста");
    salaryEdit->setPlaceholderText("Число или -");

    formLayout->addRow("Наименование:", nameEdit);
    formLayout->addRow("Оклад:", salaryEdit);

    auto *coveredLabel = new QLabel("Может выполнять функции должностей:", this);
    auto *hintLabel = new QLabel(
        "Отметь ранее созданные должности, которые сотрудник с этой ролью тоже может закрывать. "
        "Например, у Баристы можно отметить Уборщика.",
        this);
    hintLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(coveredLabel);
    mainLayout->addWidget(coveredPositionsList);
    mainLayout->addWidget(hintLabel);
    mainLayout->addWidget(buttons);

    loadCoveredPositions();
    loadCurrentPosition();
}

QString PositionEditDialog::positionName() const
{
    return nameEdit->text().trimmed();
}

QString PositionEditDialog::salaryText() const
{
    return salaryEdit->text().trimmed();
}

QList<int> PositionEditDialog::coveredPositionIds() const
{
    QList<int> ids;

    for (int i = 0; i < coveredPositionsList->count(); ++i)
    {
        QListWidgetItem *item = coveredPositionsList->item(i);
        if (item->checkState() == Qt::Checked)
            ids.append(item->data(Qt::UserRole).toInt());
    }

    return ids;
}

void PositionEditDialog::loadCoveredPositions()
{
    QSqlQuery query = DatabaseManager::instance().getPositions(currentBusinessId, true);
    while (query.next())
    {
        const int positionId = query.value("id").toInt();
        if (positionId == currentEditingPositionId)
            continue;

        auto *item = new QListWidgetItem(query.value("name").toString(), coveredPositionsList);
        item->setData(Qt::UserRole, positionId);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void PositionEditDialog::loadCurrentPosition()
{
    if (currentEditingPositionId <= 0)
        return;

    QSqlQuery query = DatabaseManager::instance().getPositionById(currentEditingPositionId);
    if (!query.next())
        return;

    nameEdit->setText(query.value("name").toString());

    if (!query.value("salary").isNull())
        salaryEdit->setText(QString::number(query.value("salary").toDouble(), 'f', 0));
    else
        salaryEdit->setText("-");

    const QList<int> coveredIds = DatabaseManager::instance().getCoveredPositionIds(currentEditingPositionId);
    for (int i = 0; i < coveredPositionsList->count(); ++i)
    {
        QListWidgetItem *item = coveredPositionsList->item(i);
        if (coveredIds.contains(item->data(Qt::UserRole).toInt()))
            item->setCheckState(Qt::Checked);
    }
}
