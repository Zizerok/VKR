#include "addbusinessdialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AddBusinessDialog::AddBusinessDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Создание предприятия");
    setModal(true);
    setFixedSize(460, 260);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(24, 24, 24, 24);

    auto *cardFrame = new QFrame(this);
    auto *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    auto *titleLabel = new QLabel("Создание предприятия", cardFrame);
    titleLabel->setObjectName("titleLabel");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *subtitleLabel = new QLabel("Введите название предприятия для начала работы.", cardFrame);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setWordWrap(true);

    nameEdit = new QLineEdit(cardFrame);
    nameEdit->setPlaceholderText("Например: Кофейня на Ленина");
    nameEdit->setMinimumHeight(42);

    statusLabel = new QLabel(cardFrame);
    statusLabel->clear();
    statusLabel->setWordWrap(true);

    auto *buttonsLayout = new QHBoxLayout();
    auto *createButton = new QPushButton("Создать", cardFrame);
    auto *cancelButton = new QPushButton("Отмена", cardFrame);
    createButton->setObjectName("createButton");
    cancelButton->setObjectName("cancelButton");
    createButton->setDefault(true);
    buttonsLayout->addWidget(createButton);
    buttonsLayout->addWidget(cancelButton);

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(nameEdit);
    cardLayout->addWidget(statusLabel);
    cardLayout->addStretch();
    cardLayout->addLayout(buttonsLayout);

    outerLayout->addWidget(cardFrame);

    setStyleSheet(R"(
        QDialog {
            background: #F6F6F6;
        }
        QFrame {
            background: #FFFFFF;
            border: none;
            border-radius: 20px;
        }
        QLabel {
            color: #8181A5;
            font-size: 14px;
            background: transparent;
            border: none;
        }
        QLabel#titleLabel, QLabel#subtitleLabel {
            color: #1C1D21;
        }
        QLineEdit {
            background: transparent;
            border: none;
            border-bottom: 1px solid #ECECF2;
            padding: 6px 2px 10px 2px;
            color: #1C1D21;
            font-size: 16px;
        }
        QLineEdit:focus {
            border-bottom: 2px solid #5E81F4;
        }
        QPushButton {
            min-height: 46px;
            border-radius: 14px;
            font-size: 15px;
            font-weight: 600;
            border: none;
        }
        QPushButton#createButton {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton#cancelButton {
            background: #E9EDFB;
            color: #5E81F4;
        }
    )");

    statusLabel->setStyleSheet("color: #FF808B; background: transparent; border: none;");

    connect(createButton, &QPushButton::clicked, this, &AddBusinessDialog::onCreateClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(nameEdit, &QLineEdit::textChanged, this, [this]() {
        statusLabel->clear();
    });

    nameEdit->setFocus();
}

QString AddBusinessDialog::businessName() const
{
    return nameEdit ? nameEdit->text().trimmed() : QString();
}

void AddBusinessDialog::onCreateClicked()
{
    if (businessName().isEmpty())
    {
        statusLabel->setText("Введите название предприятия.");
        return;
    }

    accept();
}
