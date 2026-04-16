#include "employeecardwindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

EmployeeCardWindow::EmployeeCardWindow(const QString& fullName, QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Карточка сотрудника");
    resize(700, 500);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *nameLabel = new QLabel(fullName, central);
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(18);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    auto *placeholderLabel = new QLabel(
        "Здесь будет карточка сотрудника. Пока оставлена белая страница-заглушка для дальнейшей разработки.",
        central);
    placeholderLabel->setWordWrap(true);

    layout->addWidget(nameLabel);
    layout->addWidget(placeholderLabel);
    layout->addStretch();

    central->setStyleSheet("background-color: white;");
    setCentralWidget(central);
}
