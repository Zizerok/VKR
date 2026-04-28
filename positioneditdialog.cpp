#include "positioneditdialog.h"
#include "databasemanager.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

PositionEditDialog::PositionEditDialog(int businessId, int editingPositionId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
    , currentEditingPositionId(editingPositionId)
{
    setWindowTitle(editingPositionId > 0 ? "Редактирование должности" : "Добавление должности");
    resize(560, 500);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto *titleLabel = new QLabel(
        editingPositionId > 0 ? "Параметры должности" : "Новая должность",
        this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *subtitleLabel = new QLabel(
        "Укажите наименование, оклад и список ролей, которые эта должность может закрывать.",
        this);
    subtitleLabel->setObjectName("dialogSubtitleLabel");
    subtitleLabel->setWordWrap(true);

    auto *card = new QFrame(this);
    card->setObjectName("formCard");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(14);

    auto *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(14);

    nameEdit = new QLineEdit(this);
    salaryEdit = new QLineEdit(this);
    coveredPositionsList = new QListWidget(this);
    coveredPositionsList->setObjectName("styledListWidget");
    coveredPositionsList->setAlternatingRowColors(false);
    coveredPositionsList->setFocusPolicy(Qt::NoFocus);

    nameEdit->setPlaceholderText("Например: Бариста");
    salaryEdit->setPlaceholderText("Число или -");

    formLayout->addRow("Наименование", nameEdit);
    formLayout->addRow("Оклад", salaryEdit);

    auto *coveredLabel = new QLabel("Может выполнять функции должностей", this);
    coveredLabel->setObjectName("sectionLabel");
    auto *hintLabel = new QLabel(
        "Отметьте ранее созданные должности, которые сотрудник с этой ролью тоже может закрывать. "
        "Например, у Баристы можно отметить Уборщика.",
        this);
    hintLabel->setObjectName("dialogSubtitleLabel");
    hintLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText("Сохранить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    cardLayout->addLayout(formLayout);
    cardLayout->addWidget(coveredLabel);
    cardLayout->addWidget(coveredPositionsList, 1);
    cardLayout->addWidget(hintLabel);
    cardLayout->addWidget(buttons);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(card, 1);

    setStyleSheet(R"(
        QDialog {
            background: #F6F6F6;
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
        QLabel#sectionLabel {
            color: #1C1D21;
            font-size: 14px;
            font-weight: 700;
        }
        QFrame#formCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLineEdit {
            min-height: 40px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0 12px;
            color: #1C1D21;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #5E81F4;
        }
        QListWidget#styledListWidget {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 16px;
            padding: 8px;
        }
        QListWidget#styledListWidget::item {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 10px;
            margin: 4px 0;
            color: #1C1D21;
        }
        QPushButton {
            min-height: 40px;
            border-radius: 12px;
            border: none;
            padding: 0 14px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton[text="Сохранить"] {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton[text="Отмена"] {
            background: #E9EDFB;
            color: #5E81F4;
        }
        QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
    )");

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
