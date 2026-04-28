#include "businesslist.h"
#include "ui_businesslist.h"

#include "addbusinessdialog.h"

#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QSize>

namespace
{
QString formatBusinessDate(const QString &value)
{
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");

    return dateTime.isValid() ? dateTime.toString("dd.MM.yyyy") : value;
}
}

BusinessList::BusinessList(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BusinessList)
{
    ui->setupUi(this);
    applyStyles();
    updateSelectionState();
}

BusinessList::BusinessList(int userid, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BusinessList)
{
    ui->setupUi(this);
    currentUserId = userid;

    applyStyles();

    connect(ui->lineEdit_search, &QLineEdit::textChanged, this, [this](const QString &text) {
        const QString pattern = text.trimmed().toLower();
        for (int row = 0; row < ui->listWidget_business->count(); ++row)
        {
            QListWidgetItem *item = ui->listWidget_business->item(row);
            item->setHidden(!pattern.isEmpty() && !item->text().toLower().contains(pattern));
        }
    });

    connect(ui->listWidget_business, &QListWidget::currentRowChanged, this, [this](int) {
        updateSelectionState();
    });

    loadBusinesses(currentUserId);
}

BusinessList::~BusinessList()
{
    delete ui;
}

void BusinessList::applyStyles()
{
    setModal(true);
    setMinimumSize(1040, 680);
    ui->listWidget_business->setAlternatingRowColors(false);
    ui->listWidget_business->setFocusPolicy(Qt::NoFocus);

    setStyleSheet(R"(
        QDialog {
            background: #F6F6F6;
        }
        QFrame#frameMainCard, QFrame#frameActionsCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLabel#labelTitle, QLabel#labelActionsTitle {
            color: #1C1D21;
        }
        QLabel#labelSubtitle, QLabel#labelSelectedBusiness, QLabel#labelEmptyState {
            color: #8181A5;
            font-size: 14px;
        }
        QLineEdit#lineEdit_search {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            padding: 10px 14px;
            color: #1C1D21;
            font-size: 15px;
        }
        QLineEdit#lineEdit_search:focus {
            border: 1px solid #5E81F4;
        }
        QListWidget#listWidget_business {
            background: transparent;
            border: none;
            outline: none;
            padding: 4px;
        }
        QListWidget#listWidget_business::viewport {
            background: transparent;
            border: none;
        }
        QListWidget#listWidget_business::item {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 16px;
            padding: 16px;
            margin: 4px 0px;
            color: #1C1D21;
        }
        QListWidget#listWidget_business::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QListWidget#listWidget_business::item:hover {
            background: #F8FAFF;
            border: 1px solid #D7DDF8;
        }
        QPushButton#pushButton_add, QPushButton#pushButton_open {
            background: #5E81F4;
            color: #FFFFFF;
            border: none;
            border-radius: 14px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton#pushButton_add:hover, QPushButton#pushButton_open:hover {
            background: #4C6FE0;
        }
        QPushButton#pushButton_delete {
            background: #FFF1F3;
            color: #FF808B;
            border: 1px solid #FFD8DE;
            border-radius: 14px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton#pushButton_delete:hover {
            background: #FFE6EA;
        }
        QPushButton:disabled {
            background: #F5F5FA;
            color: #B6B6CE;
            border: 1px solid #ECECF2;
        }
    )");
}

void BusinessList::loadBusinesses(int userid)
{
    ui->listWidget_business->clear();

    QSqlQuery query = DatabaseManager::instance().getBusinesses(userid);
    while (query.next())
    {
        const QString name = query.value("name").toString();
        const int id = query.value("id").toInt();
        const QString createdAt = formatBusinessDate(query.value("created_at").toString());

        QStringList lines;
        lines << name;
        lines << QString("Создано: %1").arg(createdAt.isEmpty() ? "-" : createdAt);
        lines << QString("ID предприятия: %1").arg(id);

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, id);
        item->setSizeHint(QSize(item->sizeHint().width(), 92));
        ui->listWidget_business->addItem(item);
    }

    ui->labelEmptyState->setVisible(ui->listWidget_business->count() == 0);
    if (ui->listWidget_business->count() > 0)
        ui->listWidget_business->setCurrentRow(0);

    updateSelectionState();
}

void BusinessList::updateSelectionState()
{
    QListWidgetItem *item = ui->listWidget_business->currentItem();
    const bool hasSelection = item != nullptr;

    ui->pushButton_open->setEnabled(hasSelection);
    ui->pushButton_delete->setEnabled(hasSelection);
    ui->labelSelectedBusiness->setText(
        hasSelection
            ? QString("Выбрано:\n%1").arg(item->text().section('\n', 0, 0))
            : QString("Предприятие не выбрано"));
}

void BusinessList::on_pushButton_add_clicked()
{
    AddBusinessDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = dialog.businessName();
    if (name.isEmpty())
        return;

    if (!DatabaseManager::instance().createBusiness(currentUserId, name))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать предприятие.");
        return;
    }

    loadBusinesses(currentUserId);
}

void BusinessList::on_pushButton_open_clicked()
{
    QListWidgetItem *item = ui->listWidget_business->currentItem();
    if (!item)
        return;

    const int businessId = item->data(Qt::UserRole).toInt();

    auto *window = new BusinessMainWindow(currentUserId, businessId, nullptr);
    window->show();
    close();
}

void BusinessList::on_pushButton_delete_clicked()
{
    QListWidgetItem *item = ui->listWidget_business->currentItem();
    if (!item)
        return;

    const QString businessName = item->text().section('\n', 0, 0);
    const auto reply = QMessageBox::question(
        this,
        "Удаление предприятия",
        QString("Удалить предприятие \"%1\"?").arg(businessName));

    if (reply != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deleteBusiness(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить предприятие.");
        return;
    }

    loadBusinesses(currentUserId);
}

void BusinessList::on_listWidget_business_itemDoubleClicked(QListWidgetItem *item)
{
    if (!item)
        return;

    const int businessId = item->data(Qt::UserRole).toInt();
    auto *window = new BusinessMainWindow(currentUserId, businessId, nullptr);
    window->show();
    close();
}
