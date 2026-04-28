#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"

#include "activitylogdialog.h"
#include "addemployeedialog.h"
#include "addshiftdialog.h"
#include "businesslist.h"
#include "employeecardwindow.h"
#include "login.h"
#include "positioneditdialog.h"
#include "shifttemplatedialog.h"
#include "statisticschartwidget.h"

#include <QColor>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDoubleValidator>
#include <QAbstractButton>
#include <QFormLayout>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QEventLoop>
#include <QDateEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPalette>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QSet>
#include <QSize>
#include <QSizePolicy>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QTime>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <QEvent>

namespace
{
struct CalendarShiftVisual
{
    QString timeRange;
    QString positionsText;
    bool complete = false;
};

QString findAssetPath(const QString &fileName)
{
    const QStringList candidates = {
        QDir::currentPath() + "/assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/../assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/../../assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/../../../assets/" + fileName,
        "C:/Users/Dmitrii/Documents/VKR_2/assets/" + fileName
    };

    for (const QString &candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
    }

    return QString();
}

QString buildCalendarCellHtml(int day,
                              const QList<CalendarShiftVisual> &shiftLines,
                              bool showQuestionIcon,
                              const QString &questionIconPath)
{
    QString html =
        "<div style='font-family:Segoe UI;color:#1C1D21;'>";

    html += "<table width='100%' cellspacing='0' cellpadding='0' style='margin-bottom:4px;'>"
            "<tr>"
            "<td align='left' valign='top'>";
    html += QString("<span style='font-size:15px;font-weight:700;'>%1</span>").arg(day);
    html += "</td><td align='right' valign='top'>";

    Q_UNUSED(questionIconPath);

    if (showQuestionIcon)
        html += "<span style='font-size:15px;font-weight:700;'>?</span>";

    html += "</td></tr></table>";

    for (const CalendarShiftVisual &line : shiftLines)
    {
        html += QString(
                    "<div style='margin-bottom:5px;'>"
                    "<span style='font-size:12px;font-weight:700;color:#1C1D21;line-height:1.25;"
                    "background:%2;border:1px solid %1;border-radius:10px;"
                    "padding:4px 7px;'>%3 | %4</span>"
                    "</div>")
                    .arg(line.complete ? "#7CCF9B" : "#FF808B",
                         line.complete ? "#E7FAEF" : "#FFE8EC",
                         line.timeRange.toHtmlEscaped(),
                         line.positionsText.toHtmlEscaped());
    }

    html += "</div>";
    return html;
}

void applyButtonIcon(QAbstractButton *button, const QString &fileName, const QSize &size = QSize(18, 18))
{
    if (!button)
        return;

    const QString path = findAssetPath(fileName);
    if (path.isEmpty())
        return;

    button->setIcon(QIcon(path));
    button->setIconSize(size);
}

QFrame *createKpiCard(const QString &title, QLabel **valueLabel, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName("shiftKpiCard");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(4);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("shiftKpiTitleLabel");

    auto *value = new QLabel("0", card);
    value->setObjectName("shiftKpiValueLabel");

    layout->addWidget(titleLabel);
    layout->addWidget(value);
    layout->addStretch();

    if (valueLabel)
        *valueLabel = value;

    card->setMinimumHeight(78);
    card->setMaximumHeight(86);

    return card;
}

double parsePaymentNumber(const QString& text)
{
    bool ok = false;
    const double value = QString(text).trimmed().replace(',', '.').toDouble(&ok);
    return ok ? value : 0.0;
}

QString formatPaymentDate(const QString& value)
{
    if (value.trimmed().isEmpty())
        return "-";

    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");

    return dateTime.isValid()
        ? dateTime.toString("dd.MM.yyyy HH:mm")
        : value;
}

bool paymentNeedsRevenue(const ShiftPaymentInfo& payment)
{
    return payment.paymentType.contains("Процент", Qt::CaseInsensitive)
        || !payment.percentRate.trimmed().isEmpty();
}

double calculatePaymentAmount(const ShiftPaymentInfo& payment)
{
    if (payment.paymentType == "Почасовая")
    {
        const QTime start = QTime::fromString(payment.timeRange.section(" - ", 0, 0), "HH:mm");
        const QTime end = QTime::fromString(payment.timeRange.section(" - ", 1, 1), "HH:mm");
        const double hours = start.isValid() && end.isValid() ? start.secsTo(end) / 3600.0 : 0.0;
        return hours * parsePaymentNumber(payment.hourlyRate);
    }

    if (payment.paymentType == "Фиксированная ставка")
        return parsePaymentNumber(payment.fixedRate);

    if (payment.paymentType == "Ставка + процент")
        return parsePaymentNumber(payment.fixedRate)
            + parsePaymentNumber(payment.revenueAmount) * parsePaymentNumber(payment.percentRate) / 100.0;

    if (paymentNeedsRevenue(payment))
    {
        const double percentPart = parsePaymentNumber(payment.revenueAmount)
            * parsePaymentNumber(payment.percentRate) / 100.0;
        if (!payment.fixedRate.trimmed().isEmpty())
            return parsePaymentNumber(payment.fixedRate) + percentPart;
        return percentPart;
    }

    return 0.0;
}
}

BusinessMainWindow::BusinessMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    setupNavigation();
    setupShiftsSection();
    setupStaffSection();
    setupPaymentsSection();
    setupCommunicationSection();
    setupStatisticsSection();
    setupSettingsSection();
    applyWindowStyles();
    showMaximized();
    showSection(0, "Смены");
}

BusinessMainWindow::BusinessMainWindow(int currentUserId, int businessId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    this->currentUserId = currentUserId;
    currentBusinessId = businessId;

    setupNavigation();
    setupShiftsSection();
    setupStaffSection();
    setupPaymentsSection();
    setupCommunicationSection();
    setupStatisticsSection();
    setupSettingsSection();
    applyWindowStyles();
    showMaximized();

    ui->labelBusinessTitle->setText(DatabaseManager::instance().getBusinessName(businessId));
    showSection(0, "Смены");
}

BusinessMainWindow::~BusinessMainWindow()
{
    delete ui;
}

bool BusinessMainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (shiftMonthTable && watched == shiftMonthTable->viewport() && event->type() == QEvent::Leave)
    {
        if (shiftMonthTooltipFrame)
            shiftMonthTooltipFrame->hide();
    }

    if (watched && watched->property("shiftTooltipWidget").toBool())
    {
        const QString tooltipText = watched->property("shiftTooltipText").toString();

        if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove)
        {
            if (!tooltipText.trimmed().isEmpty() && shiftMonthTooltipFrame && shiftMonthTooltipLabel)
            {
                QWidget *widget = qobject_cast<QWidget *>(watched);
                if (widget)
                {
                    shiftMonthTooltipLabel->setText(tooltipText.toHtmlEscaped().replace("\n", "<br>"));
                    shiftMonthTooltipFrame->adjustSize();
                    const QPoint globalPos = widget->mapToGlobal(QPoint(widget->width() - 6, 6));
                    shiftMonthTooltipFrame->move(globalPos + QPoint(12, 4));
                    shiftMonthTooltipFrame->show();
                    shiftMonthTooltipFrame->raise();
                }
            }
        }
        else if (event->type() == QEvent::Leave)
        {
            if (shiftMonthTooltipFrame)
                shiftMonthTooltipFrame->hide();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void BusinessMainWindow::applyWindowStyles()
{
    if (!sectionSubtitleLabel)
    {
        sectionSubtitleLabel = new QLabel(this);
        sectionSubtitleLabel->setObjectName("labelSectionSubtitle");
        sectionSubtitleLabel->setWordWrap(true);
        ui->contentLayout->insertWidget(1, sectionSubtitleLabel);
    }

    const QString chevronPath = findAssetPath("chevron-down.svg").replace("\\", "/");
    setStyleSheet(QString(R"(
        QMainWindow, QWidget#centralwidget {
            background: #F6F6F6;
        }
        QFrame#sidebarFrame {
            background: #FFFFFF;
            border: none;
        }
        QFrame#contentFrame {
            background: transparent;
        }
        QLabel#labelBusinessTitle {
            color: #1C1D21;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#labelCurrentSection {
            color: #1C1D21;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#labelSectionSubtitle {
            color: #8181A5;
            font-size: 14px;
            padding-bottom: 4px;
        }
        QPushButton#pushButtonShifts,
        QPushButton#pushButtonStaff,
        QPushButton#pushButtonPayments,
        QPushButton#pushButtonCommunication,
        QPushButton#pushButtonSettings {
            background: transparent;
            color: #8181A5;
            border: none;
            border-radius: 16px;
            text-align: left;
            padding: 14px 16px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton#pushButtonShifts:hover,
        QPushButton#pushButtonStaff:hover,
        QPushButton#pushButtonPayments:hover,
        QPushButton#pushButtonCommunication:hover,
        QPushButton#pushButtonSettings:hover {
            background: #F3F5FC;
            color: #1C1D21;
        }
        QPushButton#pushButtonShifts:checked,
        QPushButton#pushButtonStaff:checked,
        QPushButton#pushButtonPayments:checked,
        QPushButton#pushButtonCommunication:checked,
        QPushButton#pushButtonSettings:checked,
        QPushButton#statisticsNavButton:checked {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QPushButton#statisticsNavButton {
            background: transparent;
            color: #8181A5;
            border: none;
            border-radius: 16px;
            text-align: left;
            padding: 14px 16px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton#statisticsNavButton:hover {
            background: #F3F5FC;
            color: #1C1D21;
        }
        QPushButton#pushButtonCreateShift,
        QPushButton#pushButtonEditShift,
        QPushButton#pushButtonDeleteShift,
        QPushButton#shiftTemplatesButton,
        QPushButton#pushButtonShiftArchiveToggle {
            min-height: 42px;
            border-radius: 14px;
            padding: 0 16px;
            font-size: 14px;
            font-weight: 600;
            border: none;
        }
        QPushButton#pushButtonCreateShift,
        QPushButton#shiftTemplatesButton {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton#pushButtonCreateShift:hover,
        QPushButton#shiftTemplatesButton:hover {
            background: #4C6FE0;
        }
        QPushButton#pushButtonEditShift,
        QPushButton#pushButtonShiftArchiveToggle {
            background: #E9EDFB;
            color: #5E81F4;
        }
        QPushButton#pushButtonDeleteShift {
            background: #FFF1F3;
            color: #FF808B;
        }
        QPushButton#pushButtonShiftMonthView,
        QPushButton#pushButtonShiftDayView,
        QPushButton#pushButtonShiftListView {
            min-height: 40px;
            border: none;
            border-radius: 14px;
            padding: 0 18px;
            background: #F3F5FC;
            color: #8181A5;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#pushButtonShiftMonthView:checked,
        QPushButton#pushButtonShiftDayView:checked,
        QPushButton#pushButtonShiftListView:checked {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QPushButton#pushButtonShiftPrevious,
        QPushButton#pushButtonShiftNext,
        QPushButton#pushButtonShiftToday,
        QPushButton#shiftDayPreviousButton,
        QPushButton#shiftDayNextButton {
            min-height: 38px;
            border: none;
            border-radius: 12px;
            padding: 0 14px;
            background: #FFFFFF;
            color: #8181A5;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#pushButtonShiftToday {
            background: #F3F5FC;
            color: #5E81F4;
        }
        QLabel#labelShiftCurrentPeriod,
        QLabel#labelShiftContentTitle,
        QLabel#shiftLegendLabel {
            color: #1C1D21;
        }
        QLabel#labelShiftCurrentPeriod {
            font-size: 20px;
            font-weight: 700;
        }
        QLabel#labelShiftContentTitle {
            font-size: 18px;
            font-weight: 700;
        }
        QLabel#shiftLegendLabel {
            font-size: 13px;
            color: #8181A5;
            padding: 4px 0 2px 0;
        }
        QFrame#shiftKpiCard, QFrame#shiftDayCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 20px;
        }
        QFrame#paymentsLeftCard,
        QFrame#paymentsRightCard,
        QFrame#communicationFormCard,
        QFrame#communicationHistoryCard,
        QFrame#communicationResponsesCard,
        QFrame#statisticsFilterCard,
        QFrame#settingsMainCard,
        QFrame#settingsBotCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLabel#sectionTitleLabel {
            color: #1C1D21;
            font-size: 16px;
            font-weight: 700;
        }
        QLabel#paymentsSummaryLabel {
            color: #8181A5;
            font-size: 14px;
            line-height: 1.4;
        }
        QLabel#sectionInfoLabel,
        QLabel#settingsDescriptionLabel,
        QLabel#vkConnectionStatusLabel {
            color: #8181A5;
            font-size: 13px;
        }
        QLabel#shiftKpiTitleLabel {
            color: #8181A5;
            font-size: 12px;
        }
        QLabel#shiftKpiValueLabel {
            color: #1C1D21;
            font-size: 22px;
            font-weight: 700;
        }
        QTableWidget {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 20px;
            gridline-color: #F1F3F9;
            padding: 6px;
        }
        QHeaderView::section {
            background: #F8FAFF;
            color: #8181A5;
            border: none;
            padding: 8px;
            font-weight: 600;
        }
        QListWidget#listWidgetShiftList {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 20px;
            padding: 8px;
        }
        QToolButton#shiftListFilterButton,
        QToolButton#shiftListSortButton {
            min-width: 38px;
            max-width: 38px;
            min-height: 38px;
            max-height: 38px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0;
        }
        QToolButton#shiftListFilterButton:hover,
        QToolButton#shiftListSortButton:hover {
            background: #F8FAFF;
            border: 1px solid #D7DDF8;
        }
        QPushButton#pushButtonStaffEmployeesView,
        QPushButton#pushButtonStaffPositionsView {
            min-height: 40px;
            border: none;
            border-radius: 14px;
            padding: 0 18px;
            background: #F3F5FC;
            color: #8181A5;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#pushButtonStaffEmployeesView:checked,
        QPushButton#pushButtonStaffPositionsView:checked {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QLabel#labelStaffContentTitle,
        QLabel#labelEmployeeCount,
        QLabel#labelPositionCount,
        QLabel#labelSortTitle {
            color: #1C1D21;
        }
        QLabel#labelStaffContentTitle {
            font-size: 18px;
            font-weight: 700;
        }
        QLabel#labelEmployeeCount,
        QLabel#labelPositionCount,
        QLabel#labelSortTitle {
            font-size: 13px;
            color: #8181A5;
            font-weight: 600;
        }
        QToolButton#staffSortButton {
            min-width: 38px;
            max-width: 38px;
            min-height: 38px;
            max-height: 38px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0;
        }
        QToolButton#staffSortButton:hover {
            background: #F8FAFF;
            border: 1px solid #D7DDF8;
        }
        QComboBox#comboBoxEmployeeSort {
            min-height: 38px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0 12px;
            color: #1C1D21;
            font-size: 13px;
        }
        QComboBox,
        QDateEdit,
        QLineEdit,
        QTextEdit,
        QGroupBox,
        QTabWidget::pane {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 16px;
            color: #1C1D21;
        }
        QComboBox,
        QDateEdit,
        QLineEdit {
            min-height: 40px;
            padding: 0 14px;
            font-size: 14px;
        }
        QTextEdit {
            padding: 12px 14px;
            font-size: 14px;
        }
        QComboBox::drop-down,
        QDateEdit::drop-down {
            width: 32px;
            border: none;
            subcontrol-origin: padding;
            subcontrol-position: top right;
        }
        QComboBox::down-arrow,
        QDateEdit::down-arrow {
            image: url(%1);
            width: 12px;
            height: 12px;
        }
        QTabWidget::pane {
            margin-top: 10px;
            padding: 12px;
        }
        QTabBar::tab {
            min-height: 36px;
            padding: 0 16px;
            border-radius: 12px;
            background: #F3F5FC;
            color: #8181A5;
            font-size: 13px;
            font-weight: 600;
            margin-right: 8px;
        }
        QTabBar::tab:selected {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QGroupBox {
            margin-top: 12px;
            padding: 18px 16px 16px 16px;
            font-size: 14px;
            font-weight: 700;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #1C1D21;
        }
        QPushButton#primaryActionButton,
        QPushButton#settingsPrimaryButton,
        QPushButton#communicationPrimaryButton {
            min-height: 42px;
            border: none;
            border-radius: 14px;
            padding: 0 18px;
            background: #5E81F4;
            color: #FFFFFF;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#secondaryActionButton,
        QPushButton#settingsSecondaryButton,
        QPushButton#communicationSecondaryButton {
            min-height: 42px;
            border: none;
            border-radius: 14px;
            padding: 0 18px;
            background: #EEF2FF;
            color: #5E81F4;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#ghostActionButton,
        QPushButton#settingsGhostButton {
            min-height: 42px;
            border: none;
            border-radius: 14px;
            padding: 0 18px;
            background: #F3F4F8;
            color: #8181A5;
            font-size: 14px;
            font-weight: 600;
        }
        QListWidget#paymentsEmployeeListWidget::item,
        QListWidget#paymentsShiftListWidget::item,
        QListWidget#notificationsHistoryListWidget::item,
        QListWidget#shiftResponsesListWidget::item {
            background: #F9FAFF;
            border: 1px solid #E6EAF8;
            border-radius: 16px;
            padding: 14px 16px;
            margin: 4px 0;
        }
        QListWidget#paymentsEmployeeListWidget::item:selected,
        QListWidget#paymentsShiftListWidget::item:selected,
        QListWidget#notificationsHistoryListWidget::item:selected,
        QListWidget#shiftResponsesListWidget::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QListWidget#paymentsEmployeeListWidget,
        QListWidget#paymentsShiftListWidget,
        QListWidget#notificationsHistoryListWidget,
        QListWidget#shiftResponsesListWidget {
            background: transparent;
            border: none;
            outline: 0;
            padding: 4px;
        }
        QListWidget#paymentsEmployeeListWidget::viewport,
        QListWidget#paymentsShiftListWidget::viewport,
        QListWidget#notificationsHistoryListWidget::viewport,
        QListWidget#shiftResponsesListWidget::viewport {
            background: transparent;
            border: none;
        }
        QPushButton#pushButtonAddEmployee,
        QPushButton#pushButtonAddPosition,
        QPushButton#pushButtonEditPosition,
        QPushButton#pushButtonDeletePosition {
            min-height: 40px;
            border: none;
            border-radius: 14px;
            padding: 0 16px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#pushButtonAddEmployee,
        QPushButton#pushButtonAddPosition,
        QPushButton#pushButtonEditPosition {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton#pushButtonDeletePosition {
            background: #FFF1F3;
            color: #FF808B;
            border: 1px solid #FFD8DE;
        }
        QListWidget#listWidgetEmployees,
        QListWidget#listWidgetPositions {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 20px;
            padding: 8px;
        }
        QListWidget#listWidgetEmployees::viewport,
        QListWidget#listWidgetPositions::viewport {
            background: transparent;
            border: none;
        }
        QListWidget#listWidgetEmployees::item,
        QListWidget#listWidgetPositions::item {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            padding: 12px;
            margin: 4px 0;
            color: #1C1D21;
        }
        QListWidget#listWidgetEmployees::item:selected,
        QListWidget#listWidgetPositions::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QMenu {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 14px;
            border-radius: 8px;
            color: #1C1D21;
        }
        QMenu::item:selected {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QListWidget#listWidgetShiftList::viewport {
            background: transparent;
            border: none;
        }
        QListWidget#listWidgetShiftList::item {
            border: 1px solid #ECECF2;
            border-radius: 14px;
            margin: 4px 0;
            padding: 12px;
            background: #FFFFFF;
        }
        QListWidget#listWidgetShiftList::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QLabel#labelShiftListCount,
        QLabel#shiftDayCounterLabel {
            color: #8181A5;
            font-size: 14px;
            font-weight: 600;
        }
    )").arg(chevronPath));

    applyNavigationIcons();
}

void BusinessMainWindow::applyNavigationIcons()
{
    applyButtonIcon(ui->pushButtonShifts, "free-icon-switch-job-10299045.png");
    applyButtonIcon(ui->pushButtonStaff, "free-icon-user-1077114.png");
    applyButtonIcon(ui->pushButtonPayments, "free-icon-paycheck-2476429.png");
    applyButtonIcon(ui->pushButtonCommunication, "free-icon-bullhorn-8704449.png");
    applyButtonIcon(ui->pushButtonSettings, "free-icon-gear-484613.png");
    applyButtonIcon(statisticsNavButton, "free-icon-statistics-4563043.png");

    applyButtonIcon(ui->pushButtonShiftPrevious, "free-icon-arrowhead-thin-outline-to-the-left-32542.png", QSize(14, 14));
    applyButtonIcon(ui->pushButtonShiftNext, "free-icon-right-arrow-271228.png", QSize(14, 14));
    applyButtonIcon(shiftDayPreviousButton, "free-icon-arrowhead-thin-outline-to-the-left-32542.png", QSize(14, 14));
    applyButtonIcon(shiftDayNextButton, "free-icon-right-arrow-271228.png", QSize(14, 14));

    ui->pushButtonShiftPrevious->setText("");
    ui->pushButtonShiftNext->setText("");
    if (shiftDayPreviousButton)
        shiftDayPreviousButton->setText("");
    if (shiftDayNextButton)
        shiftDayNextButton->setText("");
}

QMessageBox::StandardButton BusinessMainWindow::showStyledQuestion(const QString& title, const QString& text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::No);
    messageBox.button(QMessageBox::Yes)->setText("Да");
    messageBox.button(QMessageBox::No)->setText("Нет");
    messageBox.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6F6;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
        QMessageBox QLabel#qt_msgbox_label {
            font-size: 18px;
            font-weight: 700;
        }
        QMessageBox QLabel#qt_msgbox_informativelabel {
            color: #8181A5;
            font-size: 13px;
            min-width: 280px;
        }
        QMessageBox QPushButton {
            min-width: 110px;
            min-height: 40px;
            border-radius: 12px;
            border: none;
            padding: 0 14px;
            font-size: 14px;
            font-weight: 600;
        }
        QMessageBox QPushButton[text="Да"] {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QMessageBox QPushButton[text="Нет"] {
            background: #E9EDFB;
            color: #5E81F4;
        }
    )");
    return static_cast<QMessageBox::StandardButton>(messageBox.exec());
}

void BusinessMainWindow::showStyledInformation(const QString& title, const QString& text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Information);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setText("Понятно");
    messageBox.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6F6;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
        QMessageBox QLabel#qt_msgbox_label {
            font-size: 18px;
            font-weight: 700;
        }
        QMessageBox QLabel#qt_msgbox_informativelabel {
            color: #8181A5;
            font-size: 13px;
            min-width: 280px;
        }
        QMessageBox QPushButton {
            min-width: 120px;
            min-height: 40px;
            border-radius: 12px;
            border: none;
            padding: 0 14px;
            font-size: 14px;
            font-weight: 600;
            background: #5E81F4;
            color: #FFFFFF;
        }
    )");
    messageBox.exec();
}

void BusinessMainWindow::showStyledWarning(const QString& title, const QString& text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setText("Понятно");
    messageBox.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6F6;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
        QMessageBox QLabel#qt_msgbox_label {
            font-size: 18px;
            font-weight: 700;
        }
        QMessageBox QLabel#qt_msgbox_informativelabel {
            color: #8181A5;
            font-size: 13px;
            min-width: 280px;
        }
        QMessageBox QPushButton {
            min-width: 120px;
            min-height: 40px;
            border-radius: 12px;
            border: none;
            padding: 0 14px;
            font-size: 14px;
            font-weight: 600;
            background: #F4B85E;
            color: #FFFFFF;
        }
    )");
    messageBox.exec();
}

void BusinessMainWindow::showStyledError(const QString& title, const QString& text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setText("Понятно");
    messageBox.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6F6;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
        }
        QMessageBox QLabel#qt_msgbox_label {
            font-size: 18px;
            font-weight: 700;
        }
        QMessageBox QLabel#qt_msgbox_informativelabel {
            color: #8181A5;
            font-size: 13px;
            min-width: 280px;
        }
        QMessageBox QPushButton {
            min-width: 120px;
            min-height: 40px;
            border-radius: 12px;
            border: none;
            padding: 0 14px;
            font-size: 14px;
            font-weight: 600;
            background: #FF808B;
            color: #FFFFFF;
        }
    )");
    messageBox.exec();
}

void BusinessMainWindow::setupNavigation()
{
    connect(ui->pushButtonShifts, &QPushButton::clicked, this, [this]() {
        showSection(0, "Смены");
    });
    connect(ui->pushButtonStaff, &QPushButton::clicked, this, [this]() {
        showSection(1, "Персонал");
    });
    connect(ui->pushButtonPayments, &QPushButton::clicked, this, [this]() {
        showSection(2, "Выплаты");
    });
    connect(ui->pushButtonCommunication, &QPushButton::clicked, this, [this]() {
        showSection(3, "Коммуникация");
    });
    connect(ui->pushButtonSettings, &QPushButton::clicked, this, [this]() {
        showSection(5, "Настройки");
    });
}

void BusinessMainWindow::showSection(int index, const QString& sectionTitle)
{
    ui->stackedWidgetSections->setCurrentIndex(index);
    ui->labelCurrentSection->setText(sectionTitle);
    if (sectionSubtitleLabel)
    {
        QString subtitle;
        if (index == 0)
            subtitle = "Планирование, просмотр и управление рабочими сменами.";
        else if (index == 1)
            subtitle = "Сотрудники, должности и кадровая информация предприятия.";
        else if (index == 2)
            subtitle = "Начисления, выплаты и расчёты по сменам.";
        else if (index == 3)
            subtitle = "Уведомления, отклики и работа с VK-ботом.";
        else if (index == 4)
            subtitle = "Ключевые показатели и аналитика по работе предприятия.";
        else if (index == 5)
            subtitle = "Параметры предприятия, связи с VK и служебные действия.";

        sectionSubtitleLabel->setText(subtitle);
    }

    if (index == 0)
        ui->pushButtonShifts->setChecked(true);
    else if (index == 1)
        ui->pushButtonStaff->setChecked(true);
    else if (index == 2)
        ui->pushButtonPayments->setChecked(true);
    else if (index == 3)
        ui->pushButtonCommunication->setChecked(true);
    else if (index == 4)
        statisticsNavButton->setChecked(true);
    else if (index == 5)
        ui->pushButtonSettings->setChecked(true);

    if (staffSortButton)
        staffSortButton->setVisible(index == 1 && ui->stackedWidgetStaffContent->currentIndex() == 0);
}

void BusinessMainWindow::setupShiftsSection()
{
    setupShiftMonthCalendar();
    setupShiftDayView();
    setupShiftListControls();

    shiftTemplatesButton = new QPushButton("Шаблоны смен", this);
    shiftTemplatesButton->setObjectName("shiftTemplatesButton");
    ui->shiftHeaderLayout->insertWidget(3, shiftTemplatesButton);
    setupShiftDashboardCards();

    connect(ui->pushButtonCreateShift, &QPushButton::clicked, this, &BusinessMainWindow::onCreateShiftClicked);
    connect(ui->pushButtonEditShift, &QPushButton::clicked, this, &BusinessMainWindow::onEditShiftClicked);
    connect(ui->pushButtonDeleteShift, &QPushButton::clicked, this, &BusinessMainWindow::onDeleteShiftClicked);
    connect(ui->pushButtonShiftArchiveToggle, &QPushButton::clicked, this, &BusinessMainWindow::onToggleShiftArchiveClicked);
    connect(shiftTemplatesButton, &QPushButton::clicked, this, &BusinessMainWindow::onManageShiftTemplatesClicked);

    connect(ui->pushButtonShiftMonthView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(0, "Календарь смен на месяц");
    });
    connect(ui->pushButtonShiftDayView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(1, "Смены на выбранный день");
    });
    connect(ui->pushButtonShiftListView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(2, "Список смен");
    });

    connect(ui->pushButtonShiftPrevious, &QPushButton::clicked, this, &BusinessMainWindow::onPreviousShiftPeriodClicked);
    connect(ui->pushButtonShiftNext, &QPushButton::clicked, this, &BusinessMainWindow::onNextShiftPeriodClicked);
    connect(ui->pushButtonShiftToday, &QPushButton::clicked, this, &BusinessMainWindow::onTodayShiftPeriodClicked);

    showShiftsSubsection(0, "Календарь смен на месяц");
}

void BusinessMainWindow::setupShiftDashboardCards()
{
    auto *kpiContainer = new QWidget(this);
    auto *kpiLayout = new QHBoxLayout(kpiContainer);
    kpiLayout->setContentsMargins(0, 0, 0, 0);
    kpiLayout->setSpacing(10);
    kpiLayout->addWidget(createKpiCard("Смен в периоде", &shiftKpiShiftsCountLabel, kpiContainer));
    kpiLayout->addWidget(createKpiCard("Свободных позиций", &shiftKpiOpenPositionsLabel, kpiContainer));
    kpiLayout->addWidget(createKpiCard("Назначено сотрудников", &shiftKpiAssignedEmployeesLabel, kpiContainer));
    kpiLayout->addWidget(createKpiCard("Нужно закрыть", &shiftKpiNeedsAttentionLabel, kpiContainer));
    kpiContainer->setMaximumHeight(92);

    shiftLegendLabel = new QLabel(this);
    shiftLegendLabel->setObjectName("shiftLegendLabel");
    shiftLegendLabel->hide();

    ui->verticalLayoutShifts->insertWidget(2, kpiContainer);
    ui->verticalLayoutShifts->insertWidget(3, shiftLegendLabel);
}

void BusinessMainWindow::refreshShiftDashboard()
{
    if (!shiftKpiShiftsCountLabel)
        return;

    if (currentBusinessId < 0)
    {
        shiftKpiShiftsCountLabel->setText("0");
        shiftKpiOpenPositionsLabel->setText("0");
        shiftKpiAssignedEmployeesLabel->setText("0");
        shiftKpiNeedsAttentionLabel->setText("0");
        return;
    }

    QDate fromDate;
    QDate toDate;

    switch (ui->stackedWidgetShiftContent->currentIndex())
    {
    case 0:
        fromDate = QDate(currentShiftDate.year(), currentShiftDate.month(), 1);
        toDate = fromDate.addMonths(1).addDays(-1);
        break;
    case 1:
        fromDate = currentShiftDate;
        toDate = currentShiftDate;
        break;
    default:
        fromDate = showingShiftArchive ? QDate(2000, 1, 1) : QDate::currentDate();
        toDate = showingShiftArchive ? QDate::currentDate() : QDate::currentDate().addDays(30);
        break;
    }

    int shiftCount = 0;
    int openPositions = 0;
    int assignedEmployees = 0;
    int needsAttention = 0;

    QSqlQuery query = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, fromDate, toDate);
    while (query.next())
    {
        ++shiftCount;
        const int shiftId = query.value("id").toInt();
        const QList<ShiftAssignedEmployeeData> assignments = DatabaseManager::instance().getShiftAssignments(shiftId);
        const QList<ShiftOpenPositionData> shiftOpenPositions = DatabaseManager::instance().getShiftOpenPositions(shiftId);

        assignedEmployees += assignments.size();

        int shiftOpenCount = 0;
        for (const ShiftOpenPositionData &openPosition : shiftOpenPositions)
            shiftOpenCount += openPosition.employeeCount;

        openPositions += shiftOpenCount;
        if (shiftOpenCount > 0)
            ++needsAttention;
    }

    shiftKpiShiftsCountLabel->setText(QString::number(shiftCount));
    shiftKpiOpenPositionsLabel->setText(QString::number(openPositions));
    shiftKpiAssignedEmployeesLabel->setText(QString::number(assignedEmployees));
    shiftKpiNeedsAttentionLabel->setText(QString::number(needsAttention));
}

void BusinessMainWindow::setupShiftMonthCalendar()
{
    shiftMonthTable = new QTableWidget(5, 7, this);
    shiftMonthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shiftMonthTable->setSelectionMode(QAbstractItemView::NoSelection);
    shiftMonthTable->setFocusPolicy(Qt::NoFocus);
    shiftMonthTable->setWordWrap(true);
    shiftMonthTable->setMouseTracking(true);
    shiftMonthTable->viewport()->setMouseTracking(true);
    shiftMonthTable->viewport()->installEventFilter(this);
    shiftMonthTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    shiftMonthTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    shiftMonthTable->verticalHeader()->setVisible(false);
    shiftMonthTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    shiftMonthTable->setHorizontalHeaderLabels({"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"});

    for (int row = 0; row < shiftMonthTable->rowCount(); ++row)
        shiftMonthTable->setRowHeight(row, 110);

    shiftMonthTooltipFrame = new QFrame(this, Qt::ToolTip | Qt::FramelessWindowHint);
    shiftMonthTooltipFrame->setObjectName("shiftMonthTooltipFrame");
    auto *tooltipLayout = new QVBoxLayout(shiftMonthTooltipFrame);
    tooltipLayout->setContentsMargins(10, 8, 10, 8);
    shiftMonthTooltipLabel = new QLabel(shiftMonthTooltipFrame);
    shiftMonthTooltipLabel->setWordWrap(true);
    shiftMonthTooltipLabel->setTextFormat(Qt::RichText);
    tooltipLayout->addWidget(shiftMonthTooltipLabel);
    shiftMonthTooltipFrame->setStyleSheet(
        "QFrame#shiftMonthTooltipFrame {"
        "background: #FFFFFF;"
        "border: 1px solid #DDE3F4;"
        "border-radius: 12px;"
        "}"
        "QFrame#shiftMonthTooltipFrame QLabel {"
        "color: #1C1D21;"
        "font-size: 12px;"
        "background: transparent;"
        "border: none;"
        "}");
    shiftMonthTooltipFrame->hide();

    ui->labelShiftMonthPlaceholder->hide();
    ui->verticalLayoutShiftMonth->addWidget(shiftMonthTable);
}

void BusinessMainWindow::setupShiftDayView()
{
    auto *navigationLayout = new QHBoxLayout();
    shiftDayPreviousButton = new QPushButton("<", this);
    shiftDayNextButton = new QPushButton(">", this);
    shiftDayCounterLabel = new QLabel("Смена 0 из 0", this);
    shiftDayPreviousButton->setObjectName("shiftDayPreviousButton");
    shiftDayNextButton->setObjectName("shiftDayNextButton");
    shiftDayCounterLabel->setObjectName("shiftDayCounterLabel");
    shiftDayCounterLabel->setAlignment(Qt::AlignCenter);

    navigationLayout->addWidget(shiftDayPreviousButton);
    navigationLayout->addWidget(shiftDayCounterLabel, 1);
    navigationLayout->addWidget(shiftDayNextButton);

    auto *cardFrame = new QFrame(this);
    cardFrame->setObjectName("shiftDayCard");
    auto *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(12);

    auto createSectionLabel = [this](const QString& text) {
        auto *label = new QLabel(text, this);
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
        return label;
    };

    shiftDayTimeLabel = new QLabel("-", this);
    shiftDayStatusLabel = new QLabel("-", this);
    shiftDayAssignedLabel = new QLabel("-", this);
    shiftDayOpenPositionsLabel = new QLabel("-", this);
    shiftDayCommentLabel = new QLabel("-", this);

    for (QLabel *label : {shiftDayTimeLabel, shiftDayStatusLabel, shiftDayAssignedLabel,
                          shiftDayOpenPositionsLabel, shiftDayCommentLabel})
    {
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }

    cardLayout->addWidget(createSectionLabel("Время"));
    cardLayout->addWidget(shiftDayTimeLabel);
    cardLayout->addWidget(createSectionLabel("Статус"));
    cardLayout->addWidget(shiftDayStatusLabel);
    cardLayout->addWidget(createSectionLabel("Назначенные сотрудники"));
    cardLayout->addWidget(shiftDayAssignedLabel);
    cardLayout->addWidget(createSectionLabel("Свободные позиции"));
    cardLayout->addWidget(shiftDayOpenPositionsLabel);
    cardLayout->addWidget(createSectionLabel("Комментарий"));
    cardLayout->addWidget(shiftDayCommentLabel);
    cardLayout->addStretch();

    ui->labelShiftDayPlaceholder->hide();
    ui->verticalLayoutShiftDay->addLayout(navigationLayout);
    ui->verticalLayoutShiftDay->addWidget(cardFrame);

    connect(shiftDayPreviousButton, &QPushButton::clicked, this, [this]() {
        if (currentDayShiftIds.isEmpty() || currentDayShiftIndex <= 0)
            return;
        --currentDayShiftIndex;
        showCurrentDayShift();
    });
    connect(shiftDayNextButton, &QPushButton::clicked, this, [this]() {
        if (currentDayShiftIds.isEmpty() || currentDayShiftIndex >= currentDayShiftIds.size() - 1)
            return;
        ++currentDayShiftIndex;
        showCurrentDayShift();
    });
}

void BusinessMainWindow::setupShiftListControls()
{
    shiftListControlsWidget = new QWidget(this);
    auto *controlsLayout = new QHBoxLayout(shiftListControlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);

    controlsLayout->addStretch();

    shiftListFilterButton = new QToolButton(shiftListControlsWidget);
    shiftListFilterButton->setObjectName("shiftListFilterButton");
    shiftListFilterButton->setPopupMode(QToolButton::InstantPopup);
    shiftListFilterButton->setToolTip("Фильтр смен");
    applyButtonIcon(shiftListFilterButton, "free-icon-filter-2676824.png", QSize(16, 16));

    auto *filterMenu = new QMenu(shiftListFilterButton);
    const QStringList filterItems = {
        "Все смены",
        "Только с открытыми позициями",
        "Только укомплектованные"
    };
    for (int i = 0; i < filterItems.size(); ++i)
    {
        QAction *action = filterMenu->addAction(filterItems.at(i));
        action->setCheckable(true);
        action->setChecked(i == shiftListFilterMode);
        connect(action, &QAction::triggered, this, [this, filterMenu, i]() {
            shiftListFilterMode = i;
            for (QAction *menuAction : filterMenu->actions())
                menuAction->setChecked(false);
            filterMenu->actions().at(i)->setChecked(true);
            loadShiftList();
        });
    }
    shiftListFilterButton->setMenu(filterMenu);

    shiftListSortButton = new QToolButton(shiftListControlsWidget);
    shiftListSortButton->setObjectName("shiftListSortButton");
    shiftListSortButton->setPopupMode(QToolButton::InstantPopup);
    shiftListSortButton->setToolTip("Сортировка смен");
    applyButtonIcon(shiftListSortButton, "free-icon-filter-3839020.png", QSize(16, 16));

    auto *sortMenu = new QMenu(shiftListSortButton);
    const QStringList sortItems = {
        "Сначала новые",
        "Сначала старые",
        "По времени начала"
    };
    for (int i = 0; i < sortItems.size(); ++i)
    {
        QAction *action = sortMenu->addAction(sortItems.at(i));
        action->setCheckable(true);
        action->setChecked(i == shiftListSortMode);
        connect(action, &QAction::triggered, this, [this, sortMenu, i]() {
            shiftListSortMode = i;
            for (QAction *menuAction : sortMenu->actions())
                menuAction->setChecked(false);
            sortMenu->actions().at(i)->setChecked(true);
            loadShiftList();
        });
    }
    shiftListSortButton->setMenu(sortMenu);

    controlsLayout->addWidget(shiftListFilterButton);
    controlsLayout->addWidget(shiftListSortButton);

    ui->shiftContentHeaderLayout->insertWidget(2, shiftListControlsWidget);
    shiftListControlsWidget->hide();
}

void BusinessMainWindow::showShiftsSubsection(int index, const QString& title)
{
    ui->stackedWidgetShiftContent->setCurrentIndex(index);

    if (index != 2)
        showingShiftArchive = false;

    if (index == 0)
        ui->pushButtonShiftMonthView->setChecked(true);
    else if (index == 1)
        ui->pushButtonShiftDayView->setChecked(true);
    else if (index == 2)
        ui->pushButtonShiftListView->setChecked(true);

    ui->pushButtonShiftPrevious->setEnabled(index != 2);
    ui->pushButtonShiftNext->setEnabled(index != 2);
    ui->pushButtonShiftArchiveToggle->setVisible(index == 2);

    if (index == 0)
        loadShiftMonthCalendar();
    else if (index == 1)
        loadShiftDayView();
    else if (index == 2)
        loadShiftList();

    ui->labelShiftContentTitle->setText(index == 2
        ? (showingShiftArchive ? "Архив смен" : "Список смен")
        : title);
    if (shiftLegendLabel)
        shiftLegendLabel->hide();
    if (shiftListControlsWidget)
        shiftListControlsWidget->setVisible(index == 2);

    updateShiftListMode();
    updateShiftPeriodLabel();
    refreshShiftDashboard();
}

void BusinessMainWindow::updateShiftPeriodLabel()
{
    const int shiftViewIndex = ui->stackedWidgetShiftContent->currentIndex();

    if (shiftViewIndex == 0)
        ui->labelShiftCurrentPeriod->setText(currentShiftDate.toString("MMMM yyyy"));
    else if (shiftViewIndex == 1)
        ui->labelShiftCurrentPeriod->setText(currentShiftDate.toString("dd MMMM yyyy"));
    else
        ui->labelShiftCurrentPeriod->setText(showingShiftArchive ? "Все смены" : "Актуальные смены");
}

void BusinessMainWindow::loadShiftMonthCalendar()
{
    if (!shiftMonthTable)
        return;

    const QDate firstDay(currentShiftDate.year(), currentShiftDate.month(), 1);
    const QDate lastDay = firstDay.addMonths(1).addDays(-1);
    const int startColumn = firstDay.dayOfWeek() - 1;
    const int totalCells = startColumn + lastDay.day();
    const int requiredRows = (totalCells + 6) / 7;

    shiftMonthTable->setRowCount(requiredRows);
    for (int row = 0; row < requiredRows; ++row)
        shiftMonthTable->setRowHeight(row, 110);

    for (int row = 0; row < shiftMonthTable->rowCount(); ++row)
    {
        for (int column = 0; column < shiftMonthTable->columnCount(); ++column)
        {
            QWidget *existingWidget = shiftMonthTable->cellWidget(row, column);
            if (existingWidget)
            {
                shiftMonthTable->removeCellWidget(row, column);
                existingWidget->deleteLater();
            }
        }
    }

    shiftMonthTable->clearContents();

    if (currentBusinessId < 0)
        return;

    const QString questionIconPath = findAssetPath("free-icon-question-1828940.png");

    QMap<QDate, QList<CalendarShiftVisual>> shiftsByDate;
    QSqlQuery query = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, firstDay, lastDay);
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString timeRange = QString("%1-%2")
                                      .arg(query.value("start_time").toString(),
                                           query.value("end_time").toString());

        QSet<QString> positions;
        for (const ShiftAssignedEmployeeData& assignment : DatabaseManager::instance().getShiftAssignments(shiftId))
            positions.insert(assignment.positionName);
        const QList<ShiftOpenPositionData> openPositions = DatabaseManager::instance().getShiftOpenPositions(shiftId);
        for (const ShiftOpenPositionData& openPosition : openPositions)
            positions.insert(openPosition.positionName);
        const bool shiftComplete = openPositions.isEmpty();

        QStringList sortedPositions = positions.values();
        std::sort(sortedPositions.begin(), sortedPositions.end());
        CalendarShiftVisual visual;
        visual.complete = shiftComplete;
        visual.timeRange = timeRange;
        visual.positionsText = sortedPositions.isEmpty() ? QString("Без ролей") : sortedPositions.join(", ");
        shiftsByDate[shiftDate].append(visual);
    }

    for (int day = 1; day <= lastDay.day(); ++day)
    {
        const QDate cellDate(firstDay.year(), firstDay.month(), day);
        const int cellIndex = startColumn + day - 1;
        const int row = cellIndex / 7;
        const int column = cellIndex % 7;

        QStringList lines;
        lines << QString::number(day);
        const QList<CalendarShiftVisual> shiftLines = shiftsByDate.value(cellDate);
        for (const CalendarShiftVisual &line : shiftLines)
            lines << QString("%1 | %2").arg(line.timeRange, line.positionsText);

        const QString fullText = lines.join("\n");
        QList<CalendarShiftVisual> visibleShiftLines = shiftLines;
        bool tooltipOnly = false;

        if (shiftLines.size() > 1)
        {
            visibleShiftLines = {shiftLines.first()};
            tooltipOnly = true;
        }
        else if (fullText.length() > 70)
        {
            visibleShiftLines = shiftLines.isEmpty() ? QList<CalendarShiftVisual>() : QList<CalendarShiftVisual>{shiftLines.first()};
            tooltipOnly = true;
        }

        auto *item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        item->setFlags(Qt::ItemIsEnabled);
        item->setToolTip(QString());

        const bool isToday = (cellDate == QDate::currentDate());
        if (isToday)
        {
            item->setBackground(QColor("#FFFFFF"));
            item->setData(Qt::UserRole, QColor("#F4BE5E"));
        }

        shiftMonthTable->setItem(row, column, item);

        auto *cellLabel = new QLabel(shiftMonthTable);
        cellLabel->setTextFormat(Qt::RichText);
        cellLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        cellLabel->setWordWrap(true);
        cellLabel->setMargin(4);
        cellLabel->setText(buildCalendarCellHtml(day, visibleShiftLines, tooltipOnly, questionIconPath));
        cellLabel->setStyleSheet(isToday
            ? "background: transparent; border: 2px solid #F4BE5E; border-radius: 12px;"
            : "background: transparent; border: none;");
        cellLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        cellLabel->setProperty("shiftTooltipWidget", tooltipOnly);
        cellLabel->setProperty("shiftTooltipText", tooltipOnly ? fullText : QString());
        cellLabel->setMouseTracking(true);
        cellLabel->installEventFilter(this);
        shiftMonthTable->setCellWidget(row, column, cellLabel);
    }

    refreshShiftDashboard();
}

void BusinessMainWindow::loadShiftDayView()
{
    currentDayShiftIds.clear();
    currentDayShiftIndex = 0;

    if (currentBusinessId >= 0)
    {
        QSqlQuery query = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, currentShiftDate, currentShiftDate);
        while (query.next())
            currentDayShiftIds.append(query.value("id").toInt());
    }

    showCurrentDayShift();
    refreshShiftDashboard();
}

void BusinessMainWindow::showCurrentDayShift()
{
    const int totalShifts = currentDayShiftIds.size();

    shiftDayPreviousButton->setEnabled(totalShifts > 1 && currentDayShiftIndex > 0);
    shiftDayNextButton->setEnabled(totalShifts > 1 && currentDayShiftIndex < totalShifts - 1);

    if (totalShifts == 0)
    {
        shiftDayCounterLabel->setText("Смен нет");
        shiftDayTimeLabel->setText(QString("На %1 смен пока нет.").arg(currentShiftDate.toString("dd.MM.yyyy")));
        shiftDayStatusLabel->setText("-");
        shiftDayAssignedLabel->setText("-");
        shiftDayOpenPositionsLabel->setText("-");
        shiftDayCommentLabel->setText("-");
        return;
    }

    shiftDayCounterLabel->setText(QString("Смена %1 из %2").arg(currentDayShiftIndex + 1).arg(totalShifts));

    const int shiftId = currentDayShiftIds.at(currentDayShiftIndex);
    QSqlQuery query = DatabaseManager::instance().getShiftById(shiftId);
    if (!query.next())
        return;

    shiftDayTimeLabel->setText(QString("%1 - %2")
                                   .arg(query.value("start_time").toString(),
                                        query.value("end_time").toString()));
    shiftDayStatusLabel->setText(query.value("status").toString());
    shiftDayAssignedLabel->setText(DatabaseManager::instance().getShiftAssignedSummary(shiftId).join("\n"));
    if (shiftDayAssignedLabel->text().isEmpty())
        shiftDayAssignedLabel->setText("-");

    shiftDayOpenPositionsLabel->setText(DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId).join("\n"));
    if (shiftDayOpenPositionsLabel->text().isEmpty())
        shiftDayOpenPositionsLabel->setText("-");

    const QString comment = query.value("comment").toString().trimmed();
    shiftDayCommentLabel->setText(comment.isEmpty() ? "-" : comment);
}

void BusinessMainWindow::updateShiftListMode()
{
    if (ui->stackedWidgetShiftContent->currentIndex() != 2)
    {
        ui->pushButtonShiftArchiveToggle->setVisible(false);
        return;
    }

    ui->pushButtonShiftArchiveToggle->setVisible(true);
    ui->pushButtonShiftArchiveToggle->setText(
        showingShiftArchive ? "Вернуться к списку смен" : "Архив смен");
}

void BusinessMainWindow::loadShiftList()
{
    ui->listWidgetShiftList->clear();
    ui->listWidgetShiftList->setAlternatingRowColors(false);
    ui->listWidgetShiftList->setFocusPolicy(Qt::NoFocus);

    if (currentBusinessId < 0)
    {
        ui->labelShiftListCount->setText(showingShiftArchive ? "Смен в архиве: 0" : "Актуальных смен: 0");
        return;
    }

    QSqlQuery query = showingShiftArchive
        ? DatabaseManager::instance().getAllShifts(currentBusinessId)
        : DatabaseManager::instance().getShiftsForList(currentBusinessId, QDate::currentDate());

    struct ShiftListItemData
    {
        int shiftId = -1;
        QDate shiftDate;
        QString startTime;
        QString displayText;
        bool hasOpenPositions = false;
    };

    QList<ShiftListItemData> items;
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString timeRange = QString("%1 - %2")
                                      .arg(query.value("start_time").toString(),
                                           query.value("end_time").toString());

        QStringList lines;
        lines << QString("%1 | %2 | %3")
                     .arg(shiftDate.toString("dd.MM.yyyy"), timeRange, query.value("status").toString());
        const QString assignedSummary = DatabaseManager::instance().getShiftAssignedSummary(shiftId).join("; ");
        const QString openPositionsSummary = DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId).join("; ");
        lines << QString("Назначены: %1").arg(assignedSummary.isEmpty() ? "-" : assignedSummary);
        lines << QString("Свободные позиции: %1").arg(openPositionsSummary.isEmpty() ? "-" : openPositionsSummary);

        const QString comment = query.value("comment").toString().trimmed();
        if (!comment.isEmpty())
            lines << QString("Комментарий: %1").arg(comment);

        ShiftListItemData itemData;
        itemData.shiftId = shiftId;
        itemData.shiftDate = shiftDate;
        itemData.startTime = query.value("start_time").toString();
        itemData.displayText = lines.join("\n");
        itemData.hasOpenPositions = !openPositionsSummary.isEmpty();
        items.append(itemData);
    }

    if (shiftListFilterButton)
    {
        const int filterIndex = shiftListFilterMode;
        QList<ShiftListItemData> filteredItems;
        for (const ShiftListItemData &itemData : std::as_const(items))
        {
            const bool keep = (filterIndex == 0)
                || (filterIndex == 1 && itemData.hasOpenPositions)
                || (filterIndex == 2 && !itemData.hasOpenPositions);
            if (keep)
                filteredItems.append(itemData);
        }
        items = filteredItems;
    }

    if (shiftListSortButton)
    {
        const int sortIndex = shiftListSortMode;
        std::sort(items.begin(), items.end(), [sortIndex](const ShiftListItemData &left, const ShiftListItemData &right) {
            if (sortIndex == 1)
            {
                if (left.shiftDate != right.shiftDate)
                    return left.shiftDate < right.shiftDate;
                return left.startTime < right.startTime;
            }
            if (sortIndex == 2)
            {
                if (left.startTime != right.startTime)
                    return left.startTime < right.startTime;
                return left.shiftDate > right.shiftDate;
            }

            if (left.shiftDate != right.shiftDate)
                return left.shiftDate > right.shiftDate;
            return left.startTime > right.startTime;
        });
    }

    for (const ShiftListItemData &itemData : std::as_const(items))
    {
        auto *item = new QListWidgetItem(itemData.displayText);
        item->setData(Qt::UserRole, itemData.shiftId);
        ui->listWidgetShiftList->addItem(item);
    }

    ui->labelShiftListCount->setText(
        showingShiftArchive
            ? QString("Смен в архиве: %1").arg(items.size())
            : QString("Актуальных смен: %1").arg(items.size()));

    refreshShiftDashboard();
}

void BusinessMainWindow::setupStaffSection()
{
    ui->comboBoxEmployeeSort->addItem("ФИО: А-Я");
    ui->comboBoxEmployeeSort->addItem("ФИО: Я-А");
    ui->comboBoxEmployeeSort->hide();
    ui->labelSortTitle->hide();
    ui->listWidgetEmployees->setAlternatingRowColors(false);
    ui->listWidgetEmployees->setFocusPolicy(Qt::NoFocus);
    ui->listWidgetPositions->setAlternatingRowColors(false);
    ui->listWidgetPositions->setFocusPolicy(Qt::NoFocus);

    if (!staffSortButton)
    {
        staffSortButton = new QToolButton(ui->comboBoxEmployeeSort->parentWidget());
        staffSortButton->setObjectName("staffSortButton");
        staffSortButton->setPopupMode(QToolButton::InstantPopup);
        staffSortButton->setToolTip("Сортировка сотрудников");
        const QString filterIconPath = findAssetPath("free-icon-filter-2676824.png");
        if (!filterIconPath.isEmpty())
            staffSortButton->setIcon(QIcon(filterIconPath));
        staffSortButton->setIconSize(QSize(16, 16));

        auto *sortMenu = new QMenu(staffSortButton);
        auto *sortAscAction = sortMenu->addAction("ФИО: А-Я");
        auto *sortDescAction = sortMenu->addAction("ФИО: Я-А");
        connect(sortAscAction, &QAction::triggered, this, [this]() {
            ui->comboBoxEmployeeSort->setCurrentIndex(0);
        });
        connect(sortDescAction, &QAction::triggered, this, [this]() {
            ui->comboBoxEmployeeSort->setCurrentIndex(1);
        });
        staffSortButton->setMenu(sortMenu);

        if (auto *layout = qobject_cast<QHBoxLayout*>(ui->comboBoxEmployeeSort->parentWidget()->layout()))
            layout->insertWidget(1, staffSortButton);
    }
    staffSortButton->hide();

    connect(ui->pushButtonStaffEmployeesView, &QPushButton::clicked, this, [this]() {
        showStaffSubsection(0, "Список сотрудников");
    });
    connect(ui->pushButtonStaffPositionsView, &QPushButton::clicked, this, [this]() {
        showStaffSubsection(1, "Управление должностями");
    });
    connect(ui->pushButtonAddEmployee, &QPushButton::clicked, this, &BusinessMainWindow::onAddEmployeeClicked);
    connect(ui->listWidgetEmployees, &QListWidget::itemDoubleClicked, this, &BusinessMainWindow::onEmployeeItemDoubleClicked);
    connect(ui->comboBoxEmployeeSort, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        loadEmployees();
    });
    connect(ui->pushButtonAddPosition, &QPushButton::clicked, this, &BusinessMainWindow::onAddPositionClicked);
    connect(ui->pushButtonEditPosition, &QPushButton::clicked, this, &BusinessMainWindow::onEditPositionClicked);
    connect(ui->pushButtonDeletePosition, &QPushButton::clicked, this, &BusinessMainWindow::onDeletePositionClicked);

    loadEmployees();
    loadPositions();
    showStaffSubsection(0, "Список сотрудников");
}

void BusinessMainWindow::loadEmployees()
{
    ui->listWidgetEmployees->clear();

    if (currentBusinessId < 0)
    {
        ui->labelEmployeeCount->setText("Сотрудников: 0");
        return;
    }

    QSqlQuery query = DatabaseManager::instance().getEmployees(currentBusinessId, ui->comboBoxEmployeeSort->currentIndex() == 0);
    int count = 0;
    while (query.next())
    {
        const QString fullName = query.value("full_name").toString();
        const QString position = query.value("position").toString().trimmed();
        const QString phone = query.value("phone").toString().trimmed();
        QStringList lines;
        lines << fullName;
        lines << QString("Должность: %1").arg(position.isEmpty() ? "-" : position);
        if (!phone.isEmpty())
            lines << QString("Телефон: %1").arg(phone);

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, query.value("id").toInt());
        ui->listWidgetEmployees->addItem(item);
        ++count;
    }
    ui->labelEmployeeCount->setText(QString("Сотрудников: %1").arg(count));
}

void BusinessMainWindow::loadPositions()
{
    ui->listWidgetPositions->clear();

    if (currentBusinessId < 0)
    {
        ui->labelPositionCount->setText("Должностей: 0");
        return;
    }

    QSqlQuery query = DatabaseManager::instance().getPositions(currentBusinessId, true);
    int count = 0;
    while (query.next())
    {
        const int positionId = query.value("id").toInt();
        const QString name = query.value("name").toString();
        const QVariant salaryValue = query.value("salary");
        const QString salaryText = salaryValue.isNull()
            ? "-"
            : QString::number(salaryValue.toDouble(), 'f', 0);
        const QStringList coveredPositionNames = DatabaseManager::instance().getCoveredPositionNames(positionId);

        QString text = QString("%1\nОклад: %2").arg(name, salaryText);
        if (!coveredPositionNames.isEmpty())
            text += QString("\nМожет заменить: %1").arg(coveredPositionNames.join(", "));

        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, positionId);
        ui->listWidgetPositions->addItem(item);
        ++count;
    }
    ui->labelPositionCount->setText(QString("Должностей: %1").arg(count));
}

void BusinessMainWindow::showStaffSubsection(int index, const QString& title)
{
    ui->stackedWidgetStaffContent->setCurrentIndex(index);
    ui->labelStaffContentTitle->setText(title);
    if (staffSortButton)
        staffSortButton->setVisible(index == 0 && ui->stackedWidgetSections->currentIndex() == 1);

    if (index == 0)
        ui->pushButtonStaffEmployeesView->setChecked(true);
    else if (index == 1)
        ui->pushButtonStaffPositionsView->setChecked(true);
}

void BusinessMainWindow::setupPaymentsSection()
{
    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    paymentsEmployeeListWidget = new QListWidget(this);
    paymentsEmployeeListWidget->setMinimumWidth(260);
    paymentsEmployeeListWidget->setAlternatingRowColors(false);
    paymentsEmployeeListWidget->setFocusPolicy(Qt::NoFocus);
    paymentsEmployeeListWidget->setObjectName("paymentsEmployeeListWidget");

    auto *leftPane = new QFrame(this);
    leftPane->setObjectName("paymentsLeftCard");
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);
    auto *employeesTitle = new QLabel("Сотрудники", leftPane);
    employeesTitle->setObjectName("sectionTitleLabel");
    auto *employeesInfo = new QLabel("Выберите сотрудника, чтобы посмотреть начисления и оплату по сменам.", leftPane);
    employeesInfo->setObjectName("sectionInfoLabel");
    employeesInfo->setWordWrap(true);
    leftLayout->addWidget(employeesTitle);
    leftLayout->addWidget(employeesInfo);
    leftLayout->addWidget(paymentsEmployeeListWidget, 1);

    auto *rightPane = new QFrame(this);
    rightPane->setObjectName("paymentsRightCard");
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    auto *detailsTitle = new QLabel("Выплаты и начисления", rightPane);
    detailsTitle->setObjectName("sectionTitleLabel");
    paymentsSummaryLabel = new QLabel("Выберите сотрудника для просмотра выплат", this);
    paymentsSummaryLabel->setWordWrap(true);
    paymentsSummaryLabel->setObjectName("paymentsSummaryLabel");

    auto *filterLayout = new QHBoxLayout();
    auto *filterLabel = new QLabel("Фильтр:", this);
    paymentsStatusFilterComboBox = new QComboBox(this);
    paymentsStatusFilterComboBox->addItems({"Не оплачено", "Оплачено", "Все"});
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(paymentsStatusFilterComboBox);
    filterLayout->addStretch();

    auto *revenueLayout = new QHBoxLayout();
    auto *revenueLabel = new QLabel("Выручка:", this);
    paymentsRevenueEdit = new QLineEdit(this);
    paymentsRevenueEdit->setPlaceholderText("Введите выручку");
    auto *revenueValidator = new QDoubleValidator(0.0, 1000000000.0, 2, this);
    revenueValidator->setNotation(QDoubleValidator::StandardNotation);
    paymentsRevenueEdit->setValidator(revenueValidator);
    paymentsSaveRevenueButton = new QPushButton("Сохранить выручку", this);
    paymentsSaveRevenueButton->setObjectName("primaryActionButton");
    revenueLayout->addWidget(revenueLabel);
    revenueLayout->addWidget(paymentsRevenueEdit, 1);
    revenueLayout->addWidget(paymentsSaveRevenueButton);

    paymentsShiftListWidget = new QListWidget(this);
    paymentsShiftListWidget->setAlternatingRowColors(false);
    paymentsShiftListWidget->setFocusPolicy(Qt::NoFocus);
    paymentsShiftListWidget->setObjectName("paymentsShiftListWidget");

    auto *buttonsLayout = new QHBoxLayout();
    paymentsMarkPaidButton = new QPushButton("Отметить смену оплаченной", this);
    paymentsMarkPaidButton->setObjectName("secondaryActionButton");
    paymentsMarkAllPaidButton = new QPushButton("Оплатить все", this);
    paymentsMarkAllPaidButton->setObjectName("primaryActionButton");
    buttonsLayout->addWidget(paymentsMarkPaidButton);
    buttonsLayout->addWidget(paymentsMarkAllPaidButton);

    rightLayout->addWidget(detailsTitle);
    rightLayout->addWidget(paymentsSummaryLabel);
    rightLayout->addLayout(filterLayout);
    rightLayout->addLayout(revenueLayout);
    rightLayout->addWidget(paymentsShiftListWidget, 1);
    rightLayout->addLayout(buttonsLayout);

    contentLayout->addWidget(leftPane);
    contentLayout->addWidget(rightPane, 1);

    ui->labelPaymentsPlaceholder->hide();
    ui->verticalLayoutPayments->addLayout(contentLayout);

    connect(paymentsEmployeeListWidget, &QListWidget::currentRowChanged,
            this, &BusinessMainWindow::onPaymentEmployeeSelectionChanged);
    connect(paymentsShiftListWidget, &QListWidget::currentRowChanged,
            this, &BusinessMainWindow::onPaymentShiftSelectionChanged);
    connect(paymentsStatusFilterComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &BusinessMainWindow::onPaymentFilterChanged);
    connect(paymentsSaveRevenueButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onSavePaymentRevenueClicked);
    connect(paymentsMarkPaidButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onMarkPaymentPaidClicked);
    connect(paymentsMarkAllPaidButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onMarkAllPaymentsPaidClicked);

    updatePaymentRevenueEditor();
    loadPaymentsEmployees();
}

void BusinessMainWindow::setupCommunicationSection()
{
    ui->labelCommunicationPlaceholder->hide();

    communicationTabWidget = new QTabWidget(this);

    auto *messagesPage = new QWidget(this);
    auto *messagesLayout = new QHBoxLayout(messagesPage);
    messagesLayout->setSpacing(16);

    auto *messageFormFrame = new QFrame(messagesPage);
    messageFormFrame->setObjectName("communicationFormCard");
    auto *messageFormLayout = new QVBoxLayout(messageFormFrame);
    messageFormLayout->setContentsMargins(16, 16, 16, 16);
    messageFormLayout->setSpacing(12);

    auto *messageTitle = new QLabel("Новое сообщение", messageFormFrame);
    QFont titleFont = messageTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(13);
    messageTitle->setFont(titleFont);
    messageTitle->setObjectName("sectionTitleLabel");

    auto *messageForm = new QFormLayout();
    messageTypeComboBox = new QComboBox(messageFormFrame);
    messageTypeComboBox->addItems({"Общее объявление", "Новая смена", "Изменение смены", "Отмена смены", "Выплата"});
    messageShiftComboBox = new QComboBox(messageFormFrame);
    messageRecipientTypeComboBox = new QComboBox(messageFormFrame);
    messageRecipientTypeComboBox->addItems({"Все сотрудники", "Конкретный сотрудник", "По должности"});
    messageEmployeeComboBox = new QComboBox(messageFormFrame);
    messagePositionComboBox = new QComboBox(messageFormFrame);
    messageTextEdit = new QTextEdit(messageFormFrame);
    messageTextEdit->setMinimumHeight(140);
    messageTextEdit->setPlaceholderText("Введите текст уведомления для сотрудников");

    messageForm->addRow("Тип сообщения:", messageTypeComboBox);
    messageForm->addRow("Смена:", messageShiftComboBox);
    messageForm->addRow("Получатели:", messageRecipientTypeComboBox);
    messageForm->addRow("Сотрудник:", messageEmployeeComboBox);
    messageForm->addRow("Должность:", messagePositionComboBox);
    messageForm->addRow("Текст:", messageTextEdit);

    auto *messageButtonsLayout = new QHBoxLayout();
    sendMessageButton = new QPushButton("Отправить", messageFormFrame);
    sendMessageButton->setObjectName("communicationPrimaryButton");
    clearMessageButton = new QPushButton("Очистить", messageFormFrame);
    clearMessageButton->setObjectName("communicationSecondaryButton");
    messageButtonsLayout->addWidget(sendMessageButton);
    messageButtonsLayout->addWidget(clearMessageButton);

    messageFormLayout->addWidget(messageTitle);
    messageFormLayout->addLayout(messageForm);
    messageFormLayout->addLayout(messageButtonsLayout);
    messageFormLayout->addStretch();

    auto *historyFrame = new QFrame(messagesPage);
    historyFrame->setObjectName("communicationHistoryCard");
    auto *historyLayout = new QVBoxLayout(historyFrame);
    historyLayout->setContentsMargins(16, 16, 16, 16);
    historyLayout->setSpacing(12);
    auto *historyTitle = new QLabel("История уведомлений", historyFrame);
    historyTitle->setFont(titleFont);
    historyTitle->setObjectName("sectionTitleLabel");
    notificationsHistoryListWidget = new QListWidget(historyFrame);
    notificationsHistoryListWidget->setObjectName("notificationsHistoryListWidget");
    notificationsHistoryListWidget->setAlternatingRowColors(false);
    notificationsHistoryListWidget->setFocusPolicy(Qt::NoFocus);
    historyLayout->addWidget(historyTitle);
    historyLayout->addWidget(notificationsHistoryListWidget, 1);

    messagesLayout->addWidget(messageFormFrame, 1);
    messagesLayout->addWidget(historyFrame, 1);

    auto *responsesPage = new QWidget(this);
    auto *responsesLayout = new QVBoxLayout(responsesPage);
    responsesLayout->setSpacing(12);
    auto *responsesFrame = new QFrame(responsesPage);
    responsesFrame->setObjectName("communicationResponsesCard");
    auto *responsesFrameLayout = new QVBoxLayout(responsesFrame);
    responsesFrameLayout->setContentsMargins(16, 16, 16, 16);
    responsesFrameLayout->setSpacing(12);
    auto *responsesHeaderLayout = new QHBoxLayout();
    auto *responsesTitle = new QLabel("Отклики на свободные позиции", responsesPage);
    responsesTitle->setFont(titleFont);
    responsesTitle->setObjectName("sectionTitleLabel");
    auto *refreshResponsesButton = new QPushButton("Обновить", responsesPage);
    refreshResponsesButton->setObjectName("communicationSecondaryButton");
    responsesHeaderLayout->addWidget(responsesTitle);
    responsesHeaderLayout->addStretch();
    responsesHeaderLayout->addWidget(refreshResponsesButton);
    shiftResponsesListWidget = new QListWidget(responsesPage);
    shiftResponsesListWidget->setObjectName("shiftResponsesListWidget");
    shiftResponsesListWidget->setAlternatingRowColors(false);
    shiftResponsesListWidget->setFocusPolicy(Qt::NoFocus);
    responsesFrameLayout->addLayout(responsesHeaderLayout);
    responsesFrameLayout->addWidget(shiftResponsesListWidget, 1);
    responsesLayout->addWidget(responsesFrame, 1);

    communicationTabWidget->addTab(messagesPage, "Сообщения");
    communicationTabWidget->addTab(responsesPage, "Отклики на смены");
    ui->verticalLayoutCommunication->addWidget(communicationTabWidget);

    connect(messageRecipientTypeComboBox, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateMessageRecipientControls(); });
    connect(messageTypeComboBox, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateMessageTypeControls(); });
    connect(messageShiftComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (messageTypeComboBox && messageTypeComboBox->currentText() == "Новая смена")
                    messageTextEdit->setPlainText(buildShiftNotificationText(messageShiftComboBox->currentData().toInt()));
            });
    connect(sendMessageButton, &QPushButton::clicked, this, &BusinessMainWindow::onSendMessageClicked);
    connect(clearMessageButton, &QPushButton::clicked, this, &BusinessMainWindow::onClearMessageClicked);
    connect(refreshResponsesButton, &QPushButton::clicked, this, &BusinessMainWindow::onRefreshShiftResponsesClicked);

    loadMessageRecipients();
    loadUnnotifiedShiftOptions();
    updateMessageTypeControls();
    updateMessageRecipientControls();
    loadNotificationsHistory();
    loadShiftResponses();
}

void BusinessMainWindow::loadMessageRecipients()
{
    if (!messageEmployeeComboBox || !messagePositionComboBox)
        return;

    messageEmployeeComboBox->clear();
    messagePositionComboBox->clear();

    if (currentBusinessId < 0)
        return;

    QSqlQuery employees = DatabaseManager::instance().getEmployees(currentBusinessId, true);
    while (employees.next())
        messageEmployeeComboBox->addItem(employees.value("full_name").toString(), employees.value("id"));

    const QStringList positions = DatabaseManager::instance().getPositionNames(currentBusinessId);
    for (const QString& position : positions)
        messagePositionComboBox->addItem(position);
}

void BusinessMainWindow::loadUnnotifiedShiftOptions()
{
    if (!messageShiftComboBox)
        return;

    messageShiftComboBox->clear();
    if (currentBusinessId < 0)
        return;

    QSqlQuery query = DatabaseManager::instance().getShiftsWithoutNewShiftNotification(currentBusinessId);
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString label = QString("%1 | %2-%3 | %4")
                                  .arg(shiftDate.toString("dd.MM.yyyy"),
                                       query.value("start_time").toString(),
                                       query.value("end_time").toString(),
                                       query.value("status").toString());
        messageShiftComboBox->addItem(label, shiftId);
    }
}

void BusinessMainWindow::updateMessageTypeControls()
{
    if (!messageTypeComboBox || !messageShiftComboBox)
        return;

    const bool isNewShiftMessage = messageTypeComboBox->currentText() == "Новая смена";
    messageShiftComboBox->setEnabled(isNewShiftMessage);
    messageShiftComboBox->setVisible(isNewShiftMessage);

    const bool shouldSendViaBackend = true;
    if (shouldSendViaBackend)
    {
        loadUnnotifiedShiftOptions();
        messageRecipientTypeComboBox->setCurrentIndex(0);
        messageTextEdit->setPlainText(buildShiftNotificationText(messageShiftComboBox->currentData().toInt()));
    }
    else if (messageTextEdit && messageTextEdit->toPlainText().startsWith("Открыта новая смена"))
    {
        messageTextEdit->clear();
    }
}

void BusinessMainWindow::updateMessageRecipientControls()
{
    if (!messageRecipientTypeComboBox)
        return;

    const QString recipientType = messageRecipientTypeComboBox->currentText();
    if (messageEmployeeComboBox)
        messageEmployeeComboBox->setEnabled(recipientType == "Конкретный сотрудник");
    if (messagePositionComboBox)
        messagePositionComboBox->setEnabled(recipientType == "По должности");
}

QString BusinessMainWindow::buildShiftNotificationText(int shiftId) const
{
    if (shiftId <= 0)
        return "";

    QSqlQuery query = DatabaseManager::instance().getShiftById(shiftId);
    if (!query.next())
        return "";

    const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
    QStringList lines;
    lines << "Открыта новая смена.";
    lines << QString("Дата: %1").arg(shiftDate.toString("dd.MM.yyyy"));
    lines << QString("Время: %1-%2")
                 .arg(query.value("start_time").toString(),
                      query.value("end_time").toString());

    const QStringList assigned = DatabaseManager::instance().getShiftAssignedSummary(shiftId);
    const QStringList openPositions = DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId);

    if (!assigned.isEmpty())
        lines << QString("Уже назначены: %1").arg(assigned.join("; "));
    if (!openPositions.isEmpty())
        lines << QString("Свободные позиции: %1").arg(openPositions.join("; "));

    const QString comment = query.value("comment").toString().trimmed();
    if (!comment.isEmpty())
        lines << QString("Комментарий: %1").arg(comment);

    lines << QString("Для отклика нажмите кнопку в VK-боте или отправьте: Хочу смену %1").arg(shiftId);
    return lines.join("\n");
}

void BusinessMainWindow::loadNotificationsHistory()
{
    if (!notificationsHistoryListWidget)
        return;

    notificationsHistoryListWidget->clear();
    if (currentBusinessId < 0)
        return;

    const QList<NotificationInfo> notifications = DatabaseManager::instance().getNotifications(currentBusinessId);
    for (const NotificationInfo& notification : notifications)
    {
        QStringList lines;
        lines << QString("%1 | %2 | %3")
                     .arg(formatPaymentDate(notification.createdAt),
                          notification.channel,
                          notification.sendStatus);
        lines << QString("Тип: %1").arg(notification.notificationType);
        lines << QString("Получатели: %1").arg(notification.recipientLabel);
        lines << QString("Текст: %1").arg(notification.messageText);
        if (notification.shiftId > 0)
            lines << QString("Смена ID: %1").arg(notification.shiftId);

        notificationsHistoryListWidget->addItem(new QListWidgetItem(lines.join("\n")));
    }
}

void BusinessMainWindow::loadShiftResponses()
{
    if (!shiftResponsesListWidget)
        return;

    shiftResponsesListWidget->clear();
    if (currentBusinessId < 0)
        return;

    const QList<ShiftResponseInfo> responses = DatabaseManager::instance().getShiftResponses(currentBusinessId);
    if (responses.isEmpty())
    {
        shiftResponsesListWidget->addItem("Откликов пока нет. Позже сюда будут попадать ответы сотрудников из VK-бота.");
        return;
    }

    for (const ShiftResponseInfo& response : responses)
    {
        QStringList lines;
        lines << QString("%1 | Смена ID: %2 | %3")
                     .arg(formatPaymentDate(response.createdAt))
                     .arg(response.shiftId)
                     .arg(response.positionName);
        lines << QString("Сотрудник: %1").arg(response.employeeName.isEmpty() ? "Не найден" : response.employeeName);
        lines << QString("VK ID: %1").arg(response.vkId.isEmpty() ? "-" : response.vkId);
        lines << QString("Статус: %1").arg(response.responseStatus);
        if (!response.responseMessage.trimmed().isEmpty())
            lines << QString("Комментарий: %1").arg(response.responseMessage);

        shiftResponsesListWidget->addItem(new QListWidgetItem(lines.join("\n")));
    }
}

void BusinessMainWindow::setupStatisticsSection()
{
    statisticsNavButton = new QPushButton("Статистика", this);
    statisticsNavButton->setObjectName("statisticsNavButton");
    statisticsNavButton->setCheckable(true);
    statisticsNavButton->setAutoExclusive(true);
    ui->sidebarLayout->insertWidget(6, statisticsNavButton);
    connect(statisticsNavButton, &QPushButton::clicked, this, [this]() {
        showSection(4, "Статистика");
        loadStatisticsSection();
    });

    statisticsPage = new QWidget(this);
    auto *pageLayout = new QVBoxLayout(statisticsPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(statisticsPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scrollArea);
    container->setMinimumWidth(1180);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(18);

    auto *filterFrame = new QFrame(container);
    filterFrame->setObjectName("statisticsFilterCard");
    auto *filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(16, 16, 16, 16);
    filterLayout->setSpacing(12);
    auto *periodLabel = new QLabel("Период:", filterFrame);
    periodLabel->setObjectName("sectionInfoLabel");
    statisticsStartDateEdit = new QDateEdit(QDate::currentDate().addMonths(-1), filterFrame);
    statisticsStartDateEdit->setCalendarPopup(true);
    statisticsStartDateEdit->setDisplayFormat("dd.MM.yyyy");
    statisticsStartDateEdit->setMinimumWidth(150);
    statisticsEndDateEdit = new QDateEdit(QDate::currentDate(), filterFrame);
    statisticsEndDateEdit->setCalendarPopup(true);
    statisticsEndDateEdit->setDisplayFormat("dd.MM.yyyy");
    statisticsEndDateEdit->setMinimumWidth(150);
    auto *refreshButton = new QPushButton("Обновить статистику", filterFrame);
    refreshButton->setObjectName("primaryActionButton");

    filterLayout->addWidget(periodLabel);
    filterLayout->addWidget(statisticsStartDateEdit);
    filterLayout->addWidget(new QLabel("по", filterFrame));
    filterLayout->addWidget(statisticsEndDateEdit);
    filterLayout->addWidget(refreshButton);
    filterLayout->addStretch();

    auto createKpiCard = [container](const QString& title, QLabel *&valueLabel) {
        auto *frame = new QFrame(container);
        frame->setObjectName("shiftKpiCard");
        auto *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(6);
        auto *titleLabel = new QLabel(title, frame);
        titleLabel->setObjectName("shiftKpiTitleLabel");
        valueLabel = new QLabel("-", frame);
        valueLabel->setObjectName("shiftKpiValueLabel");
        valueLabel->setWordWrap(true);
        layout->addWidget(titleLabel);
        layout->addWidget(valueLabel);
        layout->addStretch();
        return frame;
    };

    auto *kpiLayout = new QHBoxLayout();
    kpiLayout->setSpacing(12);
    kpiLayout->addWidget(createKpiCard("Смен за период", statisticsKpiShiftsLabel), 1);
    kpiLayout->addWidget(createKpiCard("Активных сотрудников", statisticsKpiEmployeesLabel), 1);
    kpiLayout->addWidget(createKpiCard("Начислено выплат", statisticsKpiPaymentsLabel), 1);
    kpiLayout->addWidget(createKpiCard("VK-отклики", statisticsKpiVkLabel), 1);

    auto createInfoBlock = [container](const QString& title, QLabel *&contentLabel) {
        auto *group = new QGroupBox(title, container);
        group->setMinimumHeight(210);
        auto *layout = new QVBoxLayout(group);
        layout->setContentsMargins(18, 18, 18, 18);
        contentLabel = new QLabel("-", group);
        contentLabel->setStyleSheet("font-size: 14px; color: #1C1D21; line-height: 1.5;");
        contentLabel->setWordWrap(true);
        contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(contentLabel);
        return group;
    };

    auto *insightsLayout = new QHBoxLayout();
    insightsLayout->setSpacing(12);
    insightsLayout->addWidget(createInfoBlock("Сравнение с прошлым периодом", statisticsComparisonLabel), 1);
    insightsLayout->addWidget(createInfoBlock("Проблемные места", statisticsProblemsLabel), 1);
    insightsLayout->addWidget(createInfoBlock("Эффективность VK-откликов", statisticsVkEfficiencyLabel), 1);

    auto createBlock = [container](const QString& title,
                                   QLabel *&summaryLabel,
                                   StatisticsChartWidget *&firstChartView,
                                   StatisticsChartWidget *&secondChartView) {
        auto *group = new QGroupBox(title, container);
        auto *layout = new QVBoxLayout(group);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(14);
        summaryLabel = new QLabel("-", group);
        summaryLabel->setStyleSheet("font-size: 14px; color: #1C1D21; line-height: 1.5;");
        summaryLabel->setWordWrap(true);
        summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        auto *chartsLayout = new QHBoxLayout();
        firstChartView = new StatisticsChartWidget(group);
        secondChartView = new StatisticsChartWidget(group);
        chartsLayout->addWidget(firstChartView, 1);
        chartsLayout->addWidget(secondChartView, 1);

        layout->addWidget(summaryLabel);
        layout->addLayout(chartsLayout);
        return group;
    };

    containerLayout->addWidget(filterFrame);
    containerLayout->addLayout(kpiLayout);
    containerLayout->addLayout(insightsLayout);
    containerLayout->addWidget(createBlock("Статистика по сменам",
                                           shiftStatisticsSummaryLabel,
                                           shiftStatusChartView,
                                           shiftPositionsChartView));
    containerLayout->addWidget(createBlock("Статистика по сотрудникам",
                                           employeeStatisticsSummaryLabel,
                                           employeePositionsChartView,
                                           employeeActivityChartView));
    containerLayout->addWidget(createBlock("Статистика по выплатам",
                                           paymentStatisticsSummaryLabel,
                                           paymentStatusChartView,
                                           paymentTopEmployeesChartView));
    containerLayout->addStretch();

    scrollArea->setWidget(container);
    pageLayout->addWidget(scrollArea);
    ui->stackedWidgetSections->insertWidget(4, statisticsPage);

    connect(refreshButton, &QPushButton::clicked, this, &BusinessMainWindow::onRefreshStatisticsClicked);
}

void BusinessMainWindow::loadStatisticsSection()
{
    if (currentBusinessId < 0
        || !statisticsStartDateEdit
        || !statisticsEndDateEdit
        || !statisticsKpiShiftsLabel
        || !statisticsKpiEmployeesLabel
        || !statisticsKpiPaymentsLabel
        || !statisticsKpiVkLabel
        || !statisticsComparisonLabel
        || !statisticsProblemsLabel
        || !statisticsVkEfficiencyLabel
        || !shiftStatusChartView
        || !shiftPositionsChartView
        || !employeePositionsChartView
        || !employeeActivityChartView
        || !paymentStatusChartView
        || !paymentTopEmployeesChartView)
    {
        return;
    }

    const QDate startDate = statisticsStartDateEdit->date();
    const QDate endDate = statisticsEndDateEdit->date();
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate)
    {
        showStyledWarning("Статистика", "Проверьте выбранный период.");
        return;
    }

    auto formatSignedPercent = [](double current, double previous) {
        if (qFuzzyCompare(previous + 1.0, 1.0))
        {
            if (qFuzzyCompare(current + 1.0, 1.0))
                return QString("0%");
            return QString("+100%");
        }

        const double percent = ((current - previous) / previous) * 100.0;
        return QString("%1%2%")
            .arg(percent >= 0.0 ? "+" : "")
            .arg(QString::number(percent, 'f', 1));
    };

    auto topFromIntMap = [](const QMap<QString, int>& map, int limit) {
        QList<QPair<QString, int>> items;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            items.append(qMakePair(it.key().trimmed().isEmpty() ? QString("Без должности") : it.key(), it.value()));
        std::sort(items.begin(), items.end(), [](const auto& left, const auto& right) {
            if (left.second == right.second)
                return left.first < right.first;
            return left.second > right.second;
        });
        if (items.size() > limit)
            items = items.mid(0, limit);
        return items;
    };

    auto topFromEmployeeIntMap = [](const QMap<int, int>& map, const QMap<int, QString>& employeeNames, int limit) {
        QList<QPair<QString, int>> items;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            items.append(qMakePair(employeeNames.value(it.key(), QString("Сотрудник %1").arg(it.key())), it.value()));
        std::sort(items.begin(), items.end(), [](const auto& left, const auto& right) {
            if (left.second == right.second)
                return left.first < right.first;
            return left.second > right.second;
        });
        if (items.size() > limit)
            items = items.mid(0, limit);
        return items;
    };

    auto topFromEmployeeDoubleMap = [](const QMap<int, double>& map, const QMap<int, QString>& employeeNames, int limit) {
        QList<QPair<QString, double>> items;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            items.append(qMakePair(employeeNames.value(it.key(), QString("Сотрудник %1").arg(it.key())), it.value()));
        std::sort(items.begin(), items.end(), [](const auto& left, const auto& right) {
            if (qFuzzyCompare(left.second + 1.0, right.second + 1.0))
                return left.first < right.first;
            return left.second > right.second;
        });
        if (items.size() > limit)
            items = items.mid(0, limit);
        return items;
    };

    int totalShifts = 0;
    int completedShifts = 0;
    int cancelledShifts = 0;
    int unfilledShifts = 0;
    QMap<QString, int> shiftPositionCounts;
    QMap<QString, int> openPositionsByName;
    QMap<int, int> openPositionsByShift;
    QSet<int> shiftsWithOpenPositions;
    QSet<int> uniqueShiftIds;

    QSqlQuery shiftsQuery = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, startDate, endDate);
    while (shiftsQuery.next())
    {
        const int shiftId = shiftsQuery.value("id").toInt();
        const QString status = shiftsQuery.value("status").toString();
        ++totalShifts;
        uniqueShiftIds.insert(shiftId);

        if (status.compare("Выполнена", Qt::CaseInsensitive) == 0)
            ++completedShifts;
        if (status.compare("Отменена", Qt::CaseInsensitive) == 0)
            ++cancelledShifts;

        const QList<ShiftAssignedEmployeeData> assignments = DatabaseManager::instance().getShiftAssignments(shiftId);
        for (const ShiftAssignedEmployeeData& assignment : assignments)
            shiftPositionCounts[assignment.positionName] += 1;

        const QList<ShiftOpenPositionData> openPositions = DatabaseManager::instance().getShiftOpenPositions(shiftId);
        for (const ShiftOpenPositionData& openPosition : openPositions)
        {
            shiftPositionCounts[openPosition.positionName] += openPosition.employeeCount;
            if (openPosition.employeeCount > 0)
            {
                shiftsWithOpenPositions.insert(shiftId);
                openPositionsByName[openPosition.positionName] += openPosition.employeeCount;
                openPositionsByShift[shiftId] += openPosition.employeeCount;
            }
        }
    }
    unfilledShifts = shiftsWithOpenPositions.size();

    int activeEmployees = 0;
    QMap<QString, int> employeesByPosition;
    QSqlQuery employeesQuery = DatabaseManager::instance().getEmployees(currentBusinessId, true);
    while (employeesQuery.next())
    {
        const bool isActive = employeesQuery.value("is_active").toInt() == 1;
        if (isActive)
            ++activeEmployees;
        employeesByPosition[employeesQuery.value("position").toString()] += 1;
    }

    QMap<int, int> shiftsPerEmployee;
    QMap<int, QString> employeeNames;
    QMap<int, double> paymentTotalsByEmployee;
    QMap<int, int> responseCountsByEmployee;
    int totalResponses = 0;
    int acceptedResponses = 0;
    int declinedResponses = 0;
    QSet<int> vkRespondedShiftIds;
    double totalAccrued = 0.0;
    double totalPaid = 0.0;

    QSqlQuery assignmentsQuery(DatabaseManager::instance().database());
    assignmentsQuery.prepare(
        "SELECT sa.id, sa.employee_id, e.full_name, s.id AS shift_id, s.shift_date, s.start_time, s.end_time, "
        "sa.position_name, sa.payment_type, sa.hourly_rate, sa.fixed_rate, sa.percent_rate, "
        "COALESCE(sa.revenue_amount, '') AS revenue_amount, COALESCE(sa.paid_at, '') AS paid_at, "
        "COALESCE(sa.is_paid, 0) AS is_paid "
        "FROM shift_assignments sa "
        "JOIN shifts s ON s.id = sa.shift_id "
        "JOIN employees e ON e.id = sa.employee_id "
        "WHERE s.business_id = :business_id "
        "AND s.shift_date >= :start_date "
        "AND s.shift_date <= :end_date "
        "ORDER BY s.shift_date ASC, e.full_name ASC");
    assignmentsQuery.bindValue(":business_id", currentBusinessId);
    assignmentsQuery.bindValue(":start_date", startDate.toString(Qt::ISODate));
    assignmentsQuery.bindValue(":end_date", endDate.toString(Qt::ISODate));
    assignmentsQuery.exec();

    while (assignmentsQuery.next())
    {
        ShiftPaymentInfo payment;
        payment.assignmentId = assignmentsQuery.value("id").toInt();
        payment.shiftId = assignmentsQuery.value("shift_id").toInt();
        payment.employeeId = assignmentsQuery.value("employee_id").toInt();
        payment.employeeName = assignmentsQuery.value("full_name").toString();
        payment.shiftDate = assignmentsQuery.value("shift_date").toString();
        payment.timeRange = QString("%1 - %2")
                                .arg(assignmentsQuery.value("start_time").toString(),
                                     assignmentsQuery.value("end_time").toString());
        payment.positionName = assignmentsQuery.value("position_name").toString();
        payment.paymentType = assignmentsQuery.value("payment_type").toString();
        payment.hourlyRate = assignmentsQuery.value("hourly_rate").toString();
        payment.fixedRate = assignmentsQuery.value("fixed_rate").toString();
        payment.percentRate = assignmentsQuery.value("percent_rate").toString();
        payment.revenueAmount = assignmentsQuery.value("revenue_amount").toString();
        payment.paidAt = assignmentsQuery.value("paid_at").toString();
        payment.isPaid = assignmentsQuery.value("is_paid").toInt() == 1;

        const double amount = calculatePaymentAmount(payment);
        totalAccrued += amount;
        if (payment.isPaid)
            totalPaid += amount;

        shiftsPerEmployee[payment.employeeId] += 1;
        employeeNames[payment.employeeId] = payment.employeeName;
        paymentTotalsByEmployee[payment.employeeId] += amount;
    }

    QSqlQuery responsesQuery(DatabaseManager::instance().database());
    responsesQuery.prepare(
        "SELECT employee_id, COUNT(*) AS response_count "
        "FROM shift_responses "
        "WHERE business_id = :business_id "
        "AND created_at::date >= :start_date "
        "AND created_at::date <= :end_date "
        "AND employee_id IS NOT NULL "
        "GROUP BY employee_id");
    responsesQuery.bindValue(":business_id", currentBusinessId);
    responsesQuery.bindValue(":start_date", startDate.toString(Qt::ISODate));
    responsesQuery.bindValue(":end_date", endDate.toString(Qt::ISODate));
    responsesQuery.exec();

    while (responsesQuery.next())
        responseCountsByEmployee[responsesQuery.value("employee_id").toInt()] = responsesQuery.value("response_count").toInt();

    QSqlQuery responseStatusQuery(DatabaseManager::instance().database());
    responseStatusQuery.prepare(
        "SELECT shift_id, response_status "
        "FROM shift_responses "
        "WHERE business_id = :business_id "
        "AND created_at::date >= :start_date "
        "AND created_at::date <= :end_date");
    responseStatusQuery.bindValue(":business_id", currentBusinessId);
    responseStatusQuery.bindValue(":start_date", startDate.toString(Qt::ISODate));
    responseStatusQuery.bindValue(":end_date", endDate.toString(Qt::ISODate));
    responseStatusQuery.exec();

    while (responseStatusQuery.next())
    {
        ++totalResponses;
        const int shiftId = responseStatusQuery.value("shift_id").toInt();
        const QString status = responseStatusQuery.value("response_status").toString().trimmed().toLower();
        if (shiftId > 0)
            vkRespondedShiftIds.insert(shiftId);

        if (status.contains("зан") || status.contains("accepted") || status.contains("назнач"))
            ++acceptedResponses;
        else if (status.contains("отмен") || status.contains("cancel") || status.contains("declin"))
            ++declinedResponses;
    }

    const QList<QPair<QString, int>> topPositions = topFromIntMap(shiftPositionCounts, 6);
    const QList<QPair<QString, int>> positionDistribution = topFromIntMap(employeesByPosition, 6);
    const QList<QPair<QString, int>> topEmployeesByShifts = topFromEmployeeIntMap(shiftsPerEmployee, employeeNames, 6);
    const QList<QPair<QString, int>> topResponders = topFromEmployeeIntMap(responseCountsByEmployee, employeeNames, 3);
    const QList<QPair<QString, double>> topEmployeesByPayments = topFromEmployeeDoubleMap(paymentTotalsByEmployee, employeeNames, 6);
    const QList<QPair<QString, int>> topOpenPositions = topFromIntMap(openPositionsByName, 3);

    QString respondersText = "Нет откликов через VK";
    if (!topResponders.isEmpty())
    {
        QStringList responderLines;
        for (const auto& responder : topResponders)
            responderLines << QString("%1 (%2)").arg(responder.first).arg(responder.second);
        respondersText = responderLines.join(", ");
    }

    const QDate previousEndDate = startDate.addDays(-1);
    const int periodDays = startDate.daysTo(endDate) + 1;
    const QDate previousStartDate = previousEndDate.addDays(-(periodDays - 1));

    int previousTotalShifts = 0;
    int previousActiveEmployees = 0;
    double previousTotalAccrued = 0.0;
    int previousResponses = 0;

    if (previousStartDate.isValid() && previousEndDate.isValid())
    {
        QSqlQuery previousShiftsQuery = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, previousStartDate, previousEndDate);
        while (previousShiftsQuery.next())
            ++previousTotalShifts;

        QSqlQuery previousEmployeesQuery = DatabaseManager::instance().getEmployees(currentBusinessId, true);
        while (previousEmployeesQuery.next())
        {
            if (previousEmployeesQuery.value("is_active").toInt() == 1)
                ++previousActiveEmployees;
        }

        QSqlQuery previousAssignmentsQuery(DatabaseManager::instance().database());
        previousAssignmentsQuery.prepare(
            "SELECT sa.id, sa.employee_id, e.full_name, s.id AS shift_id, s.shift_date, s.start_time, s.end_time, "
            "sa.position_name, sa.payment_type, sa.hourly_rate, sa.fixed_rate, sa.percent_rate, "
            "COALESCE(sa.revenue_amount, '') AS revenue_amount, COALESCE(sa.paid_at, '') AS paid_at, "
            "COALESCE(sa.is_paid, 0) AS is_paid "
            "FROM shift_assignments sa "
            "JOIN shifts s ON s.id = sa.shift_id "
            "JOIN employees e ON e.id = sa.employee_id "
            "WHERE s.business_id = :business_id "
            "AND s.shift_date >= :start_date "
            "AND s.shift_date <= :end_date");
        previousAssignmentsQuery.bindValue(":business_id", currentBusinessId);
        previousAssignmentsQuery.bindValue(":start_date", previousStartDate.toString(Qt::ISODate));
        previousAssignmentsQuery.bindValue(":end_date", previousEndDate.toString(Qt::ISODate));
        previousAssignmentsQuery.exec();

        while (previousAssignmentsQuery.next())
        {
            ShiftPaymentInfo payment;
            payment.assignmentId = previousAssignmentsQuery.value("id").toInt();
            payment.shiftId = previousAssignmentsQuery.value("shift_id").toInt();
            payment.employeeId = previousAssignmentsQuery.value("employee_id").toInt();
            payment.employeeName = previousAssignmentsQuery.value("full_name").toString();
            payment.shiftDate = previousAssignmentsQuery.value("shift_date").toString();
            payment.timeRange = QString("%1 - %2")
                                    .arg(previousAssignmentsQuery.value("start_time").toString(),
                                         previousAssignmentsQuery.value("end_time").toString());
            payment.positionName = previousAssignmentsQuery.value("position_name").toString();
            payment.paymentType = previousAssignmentsQuery.value("payment_type").toString();
            payment.hourlyRate = previousAssignmentsQuery.value("hourly_rate").toString();
            payment.fixedRate = previousAssignmentsQuery.value("fixed_rate").toString();
            payment.percentRate = previousAssignmentsQuery.value("percent_rate").toString();
            payment.revenueAmount = previousAssignmentsQuery.value("revenue_amount").toString();
            payment.paidAt = previousAssignmentsQuery.value("paid_at").toString();
            payment.isPaid = previousAssignmentsQuery.value("is_paid").toInt() == 1;
            previousTotalAccrued += calculatePaymentAmount(payment);
        }

        QSqlQuery previousResponsesQuery(DatabaseManager::instance().database());
        previousResponsesQuery.prepare(
            "SELECT COUNT(*) AS response_count "
            "FROM shift_responses "
            "WHERE business_id = :business_id "
            "AND created_at::date >= :start_date "
            "AND created_at::date <= :end_date");
        previousResponsesQuery.bindValue(":business_id", currentBusinessId);
        previousResponsesQuery.bindValue(":start_date", previousStartDate.toString(Qt::ISODate));
        previousResponsesQuery.bindValue(":end_date", previousEndDate.toString(Qt::ISODate));
        previousResponsesQuery.exec();
        if (previousResponsesQuery.next())
            previousResponses = previousResponsesQuery.value("response_count").toInt();
    }

    statisticsKpiShiftsLabel->setText(
        QString("%1\n%2 к прошлому периоду")
            .arg(totalShifts)
            .arg(formatSignedPercent(totalShifts, previousTotalShifts)));
    statisticsKpiEmployeesLabel->setText(
        QString("%1\n%2 к прошлому периоду")
            .arg(activeEmployees)
            .arg(formatSignedPercent(activeEmployees, previousActiveEmployees)));
    statisticsKpiPaymentsLabel->setText(
        QString("%1\n%2 к прошлому периоду")
            .arg(QString::number(totalAccrued, 'f', 0))
            .arg(formatSignedPercent(totalAccrued, previousTotalAccrued)));
    statisticsKpiVkLabel->setText(
        QString("%1\n%2 к прошлому периоду")
            .arg(totalResponses)
            .arg(formatSignedPercent(totalResponses, previousResponses)));

    statisticsComparisonLabel->setText(
        QString("Прошлый период: %1 - %2\n"
                "Смен: %3 -> %4 (%5)\n"
                "Начисления: %6 -> %7 (%8)\n"
                "VK-отклики: %9 -> %10 (%11)")
            .arg(previousStartDate.toString("dd.MM.yyyy"))
            .arg(previousEndDate.toString("dd.MM.yyyy"))
            .arg(previousTotalShifts)
            .arg(totalShifts)
            .arg(formatSignedPercent(totalShifts, previousTotalShifts))
            .arg(QString::number(previousTotalAccrued, 'f', 0))
            .arg(QString::number(totalAccrued, 'f', 0))
            .arg(formatSignedPercent(totalAccrued, previousTotalAccrued))
            .arg(previousResponses)
            .arg(totalResponses)
            .arg(formatSignedPercent(totalResponses, previousResponses)));

    QStringList problemLines;
    if (unfilledShifts > 0)
        problemLines << QString("Неукомплектованных смен: %1").arg(unfilledShifts);
    if (!topOpenPositions.isEmpty())
    {
        problemLines << "Самые дефицитные должности:";
        for (const auto& item : topOpenPositions)
            problemLines << QString("%1 — %2 своб. мест").arg(item.first).arg(item.second);
    }
    if (!topEmployeesByShifts.isEmpty())
    {
        problemLines << "Сотрудники с наибольшей нагрузкой:";
        const int overloadLimit = (topEmployeesByShifts.size() < 3)
            ? static_cast<int>(topEmployeesByShifts.size())
            : 3;
        for (int index = 0; index < overloadLimit; ++index)
            problemLines << QString("%1 — %2 смен").arg(topEmployeesByShifts.at(index).first).arg(topEmployeesByShifts.at(index).second);
    }
    if (problemLines.isEmpty())
        problemLines << "Явных проблем за выбранный период не обнаружено.";
    statisticsProblemsLabel->setText(problemLines.join("\n"));

    int respondedProblemShifts = 0;
    for (auto it = openPositionsByShift.constBegin(); it != openPositionsByShift.constEnd(); ++it)
    {
        if (vkRespondedShiftIds.contains(it.key()))
            ++respondedProblemShifts;
    }

    const int shiftsWithDemand = openPositionsByShift.size();
    const double responsePerOpenShift = shiftsWithDemand == 0 ? 0.0 : static_cast<double>(totalResponses) / shiftsWithDemand;
    const double closureRate = shiftsWithDemand == 0 ? 0.0 : (static_cast<double>(respondedProblemShifts) / shiftsWithDemand) * 100.0;
    const double acceptedRate = totalResponses == 0 ? 0.0 : (static_cast<double>(acceptedResponses) / totalResponses) * 100.0;

    statisticsVkEfficiencyLabel->setText(
        QString("Всего откликов: %1\n"
                "Подтверждено системой: %2\n"
                "Отмен/отказов: %3\n"
                "Среднее откликов на смену с набором: %4\n"
                "Проблемных смен с откликами: %5%\n"
                "Доля успешных откликов: %6%")
            .arg(totalResponses)
            .arg(acceptedResponses)
            .arg(declinedResponses)
            .arg(QString::number(responsePerOpenShift, 'f', 1))
            .arg(QString::number(closureRate, 'f', 1))
            .arg(QString::number(acceptedRate, 'f', 1)));

    shiftStatisticsSummaryLabel->setText(
        QString("Смен создано: %1\nВыполнено: %2\nОтменено: %3\nНеукомплектовано: %4")
            .arg(totalShifts)
            .arg(completedShifts)
            .arg(cancelledShifts)
            .arg(unfilledShifts));

    employeeStatisticsSummaryLabel->setText(
        QString("Активных сотрудников: %1\nСотрудников по должностям: %2\nТоп по откликам VK: %3")
            .arg(activeEmployees)
            .arg(employeesByPosition.size())
            .arg(respondersText));

    const double unpaidAmount = std::max(0.0, totalAccrued - totalPaid);
    const double averagePerShift = uniqueShiftIds.isEmpty() ? 0.0 : totalAccrued / uniqueShiftIds.size();
    paymentStatisticsSummaryLabel->setText(
        QString("Начислено: %1\nВыплачено: %2\nНе выплачено: %3\nСредняя выплата за смену: %4")
            .arg(QString::number(totalAccrued, 'f', 2))
            .arg(QString::number(totalPaid, 'f', 2))
            .arg(QString::number(unpaidAmount, 'f', 2))
            .arg(QString::number(averagePerShift, 'f', 2)));

    shiftStatusChartView->setPieData(
        "Состояние смен",
        {
            {"Выполнено", static_cast<double>(completedShifts)},
            {"Отменено", static_cast<double>(cancelledShifts)},
            {"Неукомплектовано", static_cast<double>(unfilledShifts)},
            {"Остальные", static_cast<double>(std::max(0, totalShifts - completedShifts - cancelledShifts))}
        },
        {QColor(34, 197, 94), QColor(239, 68, 68), QColor(245, 158, 11), QColor(59, 130, 246)});

    {
        QStringList categories;
        QList<double> values;
        for (const auto& item : topPositions)
        {
            categories << item.first;
            values << item.second;
        }
        shiftPositionsChartView->setBarData("Популярные должности в сменах", categories, values, QColor(37, 99, 235));
    }

    {
        QList<QPair<QString, double>> pieValues;
        for (const auto& item : positionDistribution)
            pieValues.append(qMakePair(item.first, static_cast<double>(item.second)));
        employeePositionsChartView->setPieData(
            "Сотрудники по должностям",
            pieValues,
            {QColor(99, 102, 241), QColor(16, 185, 129), QColor(249, 115, 22), QColor(236, 72, 153), QColor(20, 184, 166), QColor(234, 179, 8)});
    }

    {
        QStringList categories;
        QList<double> values;
        for (const auto& item : topEmployeesByShifts)
        {
            categories << item.first;
            values << item.second;
        }
        employeeActivityChartView->setBarData("Кто чаще выходит в смены", categories, values, QColor(124, 58, 237));
    }

    paymentStatusChartView->setPieData(
        "Выплаты за период",
        {
            {"Выплачено", totalPaid},
            {"Не выплачено", unpaidAmount}
        },
        {QColor(34, 197, 94), QColor(239, 68, 68)});

    {
        QStringList categories;
        QList<double> values;
        for (const auto& item : topEmployeesByPayments)
        {
            categories << item.first;
            values << item.second;
        }
        paymentTopEmployeesChartView->setBarData("Топ сотрудников по сумме выплат", categories, values, QColor(14, 165, 233));
    }
}

void BusinessMainWindow::setupSettingsSection()
{
    ui->labelSettingsPlaceholder->hide();

    {
        auto *settingsFrame = new QFrame(this);
        settingsFrame->setObjectName("settingsMainCard");
        auto *settingsLayout = new QVBoxLayout(settingsFrame);
        settingsLayout->setContentsMargins(20, 20, 20, 20);
        settingsLayout->setSpacing(16);

        auto *titleLabel = new QLabel(QString::fromUtf8("Настройки предприятия"), settingsFrame);
        QFont titleFont = titleLabel->font();
        titleFont.setBold(true);
        titleFont.setPointSize(13);
        titleLabel->setFont(titleFont);
        titleLabel->setObjectName("sectionTitleLabel");

        auto *descriptionLabel = new QLabel(
            QString::fromUtf8("VK-бот теперь является единым сервисом системы. Администратор предприятия не настраивает токены и API, а только включает уведомления и дает сотрудникам ссылку на бота."),
            settingsFrame);
        descriptionLabel->setObjectName("settingsDescriptionLabel");
        descriptionLabel->setWordWrap(true);

        auto *botFrame = new QFrame(settingsFrame);
        botFrame->setObjectName("settingsBotCard");
        auto *botLayout = new QVBoxLayout(botFrame);
        botLayout->setContentsMargins(16, 16, 16, 16);
        auto *botTitleLabel = new QLabel(QString::fromUtf8("Единый VK-бот системы"), botFrame);
        QFont botTitleFont = botTitleLabel->font();
        botTitleFont.setBold(true);
        botTitleLabel->setFont(botTitleFont);
        botTitleLabel->setObjectName("sectionTitleLabel");
        auto *botInfoLabel = new QLabel(
            QString::fromUtf8("Все предприятия используют один общий VK-бот. Сотрудники пишут боту один раз, после этого система сможет отправлять им уведомления о сменах, объявлениях и выплатах."),
            botFrame);
        botInfoLabel->setObjectName("sectionInfoLabel");
        botInfoLabel->setWordWrap(true);
        botLayout->addWidget(botTitleLabel);
        botLayout->addWidget(botInfoLabel);

        auto *formLayout = new QFormLayout();
        vkEnabledCheckBox = new QCheckBox(QString::fromUtf8("VK-уведомления включены"), settingsFrame);
        vkGroupIdEdit = new QLineEdit(settingsFrame);
        vkGroupIdEdit->setPlaceholderText(QString::fromUtf8("Например: https://vk.com/your_bot_or_group"));
        vkCommunityTokenEdit = nullptr;
        vkBackendUrlEdit = nullptr;

        formLayout->addRow("", vkEnabledCheckBox);
        formLayout->addRow(QString::fromUtf8("Ссылка для сотрудников:"), vkGroupIdEdit);

        auto *buttonsLayout = new QHBoxLayout();
        auto *saveSettingsButton = new QPushButton(QString::fromUtf8("Сохранить настройки"), settingsFrame);
        saveSettingsButton->setObjectName("settingsPrimaryButton");
        auto *checkConnectionButton = new QPushButton(QString::fromUtf8("Проверить backend"), settingsFrame);
        checkConnectionButton->setObjectName("settingsSecondaryButton");
        auto *activityLogButton = new QPushButton(QString::fromUtf8("Журнал действий"), settingsFrame);
        activityLogButton->setObjectName("settingsSecondaryButton");
        buttonsLayout->addWidget(saveSettingsButton);
        buttonsLayout->addWidget(checkConnectionButton);
        buttonsLayout->addWidget(activityLogButton);
        buttonsLayout->addStretch();

        auto *accountActionsLayout = new QHBoxLayout();
        auto *backToBusinessesButton = new QPushButton(QString::fromUtf8("К списку предприятий"), settingsFrame);
        backToBusinessesButton->setObjectName("settingsGhostButton");
        auto *logoutButton = new QPushButton(QString::fromUtf8("Выйти из аккаунта"), settingsFrame);
        logoutButton->setObjectName("settingsGhostButton");
        accountActionsLayout->addWidget(backToBusinessesButton);
        accountActionsLayout->addWidget(logoutButton);
        accountActionsLayout->addStretch();

        vkConnectionStatusLabel = new QLabel(QString::fromUtf8("Статус: подключение не проверялось"), settingsFrame);
        vkConnectionStatusLabel->setObjectName("vkConnectionStatusLabel");
        vkConnectionStatusLabel->setWordWrap(true);

        settingsLayout->addWidget(titleLabel);
        settingsLayout->addWidget(descriptionLabel);
        settingsLayout->addWidget(botFrame);
        settingsLayout->addLayout(formLayout);
        settingsLayout->addLayout(buttonsLayout);
        settingsLayout->addWidget(vkConnectionStatusLabel);
        settingsLayout->addSpacing(18);
        settingsLayout->addLayout(accountActionsLayout);
        settingsLayout->addStretch();

        ui->verticalLayoutSettings->addWidget(settingsFrame);

        connect(saveSettingsButton, &QPushButton::clicked, this, &BusinessMainWindow::onSaveVkSettingsClicked);
        connect(checkConnectionButton, &QPushButton::clicked, this, &BusinessMainWindow::onCheckVkConnectionClicked);
        connect(activityLogButton, &QPushButton::clicked, this, [this]() {
            ActivityLogDialog dialog(currentBusinessId, this);
            dialog.exec();
        });
        connect(backToBusinessesButton, &QPushButton::clicked, this, [this]() {
            auto *businessList = new BusinessList(currentUserId);
            businessList->setAttribute(Qt::WA_DeleteOnClose);
            businessList->show();
            close();
        });
        connect(logoutButton, &QPushButton::clicked, this, [this]() {
            auto *loginWindow = new Login();
            loginWindow->setAttribute(Qt::WA_DeleteOnClose);
            loginWindow->show();
            close();
        });

        loadVkSettings();
        return;
    }

    auto *settingsFrame = new QFrame(this);
    auto *settingsLayout = new QVBoxLayout(settingsFrame);
    settingsLayout->setSpacing(14);

    auto *titleLabel = new QLabel("Настройки VK-интеграции", settingsFrame);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(13);
    titleLabel->setFont(titleFont);

    auto *descriptionLabel = new QLabel(
        "Эти параметры будут использоваться для связи desktop-приложения с VK-ботом сообщества и backend-сервером.",
        settingsFrame);
    descriptionLabel->setWordWrap(true);

    auto *formLayout = new QFormLayout();
    vkGroupIdEdit = new QLineEdit(settingsFrame);
    vkGroupIdEdit->setPlaceholderText("Например: 123456789");
    vkCommunityTokenEdit = new QLineEdit(settingsFrame);
    vkCommunityTokenEdit->setPlaceholderText("Токен сообщества VK");
    vkCommunityTokenEdit->setEchoMode(QLineEdit::Password);
    vkBackendUrlEdit = new QLineEdit(settingsFrame);
    vkBackendUrlEdit->setPlaceholderText("https://example.ru/vk/callback");
    vkEnabledCheckBox = new QCheckBox("Интеграция включена", settingsFrame);

    formLayout->addRow("ID группы VK:", vkGroupIdEdit);
    formLayout->addRow("Токен сообщества:", vkCommunityTokenEdit);
    formLayout->addRow("URL backend-сервера:", vkBackendUrlEdit);
    formLayout->addRow("", vkEnabledCheckBox);

    auto *buttonsLayout = new QHBoxLayout();
    auto *saveSettingsButton = new QPushButton("Сохранить настройки", settingsFrame);
    auto *checkConnectionButton = new QPushButton("Проверить подключение", settingsFrame);
    buttonsLayout->addWidget(saveSettingsButton);
    buttonsLayout->addWidget(checkConnectionButton);
    buttonsLayout->addStretch();

    vkConnectionStatusLabel = new QLabel("Статус: настройки не проверялись", settingsFrame);
    vkConnectionStatusLabel->setWordWrap(true);

    settingsLayout->addWidget(titleLabel);
    settingsLayout->addWidget(descriptionLabel);
    settingsLayout->addLayout(formLayout);
    settingsLayout->addLayout(buttonsLayout);
    settingsLayout->addWidget(vkConnectionStatusLabel);
    settingsLayout->addStretch();

    ui->verticalLayoutSettings->addWidget(settingsFrame);

    connect(saveSettingsButton, &QPushButton::clicked, this, &BusinessMainWindow::onSaveVkSettingsClicked);
    connect(checkConnectionButton, &QPushButton::clicked, this, &BusinessMainWindow::onCheckVkConnectionClicked);

    loadVkSettings();
}

void BusinessMainWindow::loadVkSettings()
{
    if (currentBusinessId < 0 || !vkGroupIdEdit || !vkEnabledCheckBox)
        return;

    const VkSettingsData settings = DatabaseManager::instance().getVkSettings(currentBusinessId);
    vkGroupIdEdit->setText(settings.groupId);
    if (vkCommunityTokenEdit)
        vkCommunityTokenEdit->setText(settings.communityToken);
    if (vkBackendUrlEdit)
        vkBackendUrlEdit->setText(settings.backendUrl);
    vkEnabledCheckBox->setChecked(settings.isEnabled);
}

void BusinessMainWindow::loadPaymentsEmployees()
{
    paymentsEmployeeListWidget->clear();

    const QList<EmployeePaymentSummary> summaries =
        DatabaseManager::instance().getEmployeePaymentSummaries(currentBusinessId);

    for (const EmployeePaymentSummary& summary : summaries)
    {
        auto *item = new QListWidgetItem(
            QString("%1\nК выплате: %2 | Неоплачено смен: %3")
                .arg(summary.employeeName)
                .arg(QString::number(summary.totalAmount, 'f', 2))
                .arg(summary.unpaidAssignments));
        item->setData(Qt::UserRole, summary.employeeId);
        paymentsEmployeeListWidget->addItem(item);
    }

    if (paymentsEmployeeListWidget->count() > 0)
        paymentsEmployeeListWidget->setCurrentRow(0);
    else
        loadEmployeePaymentDetails(-1);
}

void BusinessMainWindow::loadEmployeePaymentDetails(int employeeId)
{
    currentPaymentItems.clear();
    paymentsShiftListWidget->clear();

    if (employeeId < 0)
    {
        paymentsSummaryLabel->setText("Данных по выплатам пока нет.");
        paymentsMarkPaidButton->setEnabled(false);
        paymentsMarkAllPaidButton->setEnabled(false);
        updatePaymentRevenueEditor();
        return;
    }

    currentPaymentItems = DatabaseManager::instance().getEmployeeShiftPayments(currentBusinessId, employeeId);

    double totalUnpaid = 0.0;
    int unpaidCount = 0;
    int visibleCount = 0;
    for (int i = 0; i < currentPaymentItems.size(); ++i)
    {
        const ShiftPaymentInfo& payment = currentPaymentItems.at(i);
        const double amount = calculatePaymentAmount(payment);
        if (!payment.isPaid)
        {
            totalUnpaid += amount;
            ++unpaidCount;
        }

        if (!paymentMatchesCurrentFilter(payment))
            continue;

        QStringList lines;
        lines << QString("%1 | %2 | %3").arg(payment.shiftDate, payment.timeRange, payment.positionName);
        if (paymentNeedsRevenue(payment))
            lines << QString("Выручка: %1").arg(payment.revenueAmount.trimmed().isEmpty() ? "-" : payment.revenueAmount);
        lines << QString("Тип оплаты: %1").arg(payment.paymentType);
        lines << QString("Сумма: %1").arg(QString::number(amount, 'f', 2));
        lines << QString("Статус: %1").arg(payment.isPaid ? "Оплачено" : "Не оплачено");

        if (payment.isPaid)
            lines << QString("Дата оплаты: %1").arg(formatPaymentDate(payment.paidAt));

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, payment.assignmentId);
        item->setData(Qt::UserRole + 1, i);
        paymentsShiftListWidget->addItem(item);
        ++visibleCount;

    }

    const QString employeeName = paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->text().section('\n', 0, 0)
        : "Сотрудник";
    paymentsSummaryLabel->setText(
        QString("%1\nК выплате: %2\nНеоплаченных смен: %3")
            .arg(employeeName)
            .arg(QString::number(totalUnpaid, 'f', 2))
            .arg(QString("%1\nЗаписей в списке: %2").arg(unpaidCount).arg(visibleCount)));

    paymentsMarkPaidButton->setEnabled(paymentsShiftListWidget->count() > 0);
    paymentsMarkAllPaidButton->setEnabled(unpaidCount > 0);
    if (paymentsShiftListWidget->count() > 0)
        paymentsShiftListWidget->setCurrentRow(0);
    updatePaymentRevenueEditor();
}

void BusinessMainWindow::onPreviousShiftPeriodClicked()
{
    const int view = ui->stackedWidgetShiftContent->currentIndex();
    if (view == 2)
        return;

    currentShiftDate = (view == 0) ? currentShiftDate.addMonths(-1) : currentShiftDate.addDays(-1);
    if (view == 0)
        loadShiftMonthCalendar();
    else
        loadShiftDayView();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onNextShiftPeriodClicked()
{
    const int view = ui->stackedWidgetShiftContent->currentIndex();
    if (view == 2)
        return;

    currentShiftDate = (view == 0) ? currentShiftDate.addMonths(1) : currentShiftDate.addDays(1);
    if (view == 0)
        loadShiftMonthCalendar();
    else
        loadShiftDayView();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onTodayShiftPeriodClicked()
{
    if (ui->stackedWidgetShiftContent->currentIndex() == 2)
    {
        showingShiftArchive = false;
        loadShiftList();
        updateShiftListMode();
        updateShiftPeriodLabel();
        ui->labelShiftContentTitle->setText("Список смен");
        return;
    }

    currentShiftDate = QDate::currentDate();
    if (ui->stackedWidgetShiftContent->currentIndex() == 0)
        loadShiftMonthCalendar();
    else
        loadShiftDayView();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onCreateShiftClicked()
{
    AddShiftDialog dialog(currentBusinessId, -1, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadShiftMonthCalendar();
        loadShiftDayView();
        loadShiftList();
        loadPaymentsEmployees();
        loadUnnotifiedShiftOptions();
    }
}

void BusinessMainWindow::onEditShiftClicked()
{
    if (ui->stackedWidgetShiftContent->currentIndex() != 2)
    {
        showStyledInformation("Смены", "Редактирование доступно в режиме списка.");
        return;
    }

    QListWidgetItem *item = ui->listWidgetShiftList->currentItem();
    if (!item)
    {
        showStyledInformation("Смены", "Сначала выберите смену для редактирования.");
        return;
    }

    AddShiftDialog dialog(currentBusinessId, item->data(Qt::UserRole).toInt(), this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadShiftMonthCalendar();
        loadShiftDayView();
        loadShiftList();
        loadPaymentsEmployees();
        loadUnnotifiedShiftOptions();
    }
}

void BusinessMainWindow::onDeleteShiftClicked()
{
    if (ui->stackedWidgetShiftContent->currentIndex() != 2)
    {
        showStyledInformation("Смены", "Удаление доступно в режиме списка.");
        return;
    }

    QListWidgetItem *item = ui->listWidgetShiftList->currentItem();
    if (!item)
    {
        showStyledInformation("Смены", "Сначала выберите смену для удаления.");
        return;
    }

    if (showStyledQuestion("Удаление смены", "Удалить выбранную смену?") != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deleteShift(item->data(Qt::UserRole).toInt()))
    {
        showStyledError("Ошибка", "Не удалось удалить смену.");
        return;
    }

    loadShiftMonthCalendar();
    loadShiftDayView();
    loadShiftList();
    loadPaymentsEmployees();
    loadUnnotifiedShiftOptions();
}

void BusinessMainWindow::onToggleShiftArchiveClicked()
{
    showingShiftArchive = !showingShiftArchive;
    ui->labelShiftContentTitle->setText(showingShiftArchive ? "Архив смен" : "Список смен");
    loadShiftList();
    updateShiftListMode();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onManageShiftTemplatesClicked()
{
    ShiftTemplateDialog dialog(currentBusinessId, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadShiftMonthCalendar();
        loadShiftDayView();
        loadShiftList();
        loadPaymentsEmployees();
        loadUnnotifiedShiftOptions();
    }
}

void BusinessMainWindow::onAddEmployeeClicked()
{
    AddEmployeeDialog dialog(currentBusinessId, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadEmployees();
        loadPaymentsEmployees();
        loadMessageRecipients();
    }
}

void BusinessMainWindow::onEmployeeItemDoubleClicked(QListWidgetItem *item)
{
    auto *employeeCardWindow = new EmployeeCardWindow(item->data(Qt::UserRole).toInt(), this);
    employeeCardWindow->show();
}

void BusinessMainWindow::onAddPositionClicked()
{
    PositionEditDialog dialog(currentBusinessId, -1, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.positionName().isEmpty())
    {
        showStyledWarning("Ошибка", "Заполните наименование должности.");
        return;
    }

    if (!DatabaseManager::instance().createPosition(
            currentBusinessId,
            dialog.positionName(),
            dialog.salaryText(),
            dialog.coveredPositionIds()))
    {
        showStyledError("Ошибка", "Не удалось добавить должность.");
        return;
    }

    loadPositions();
    loadMessageRecipients();
}

void BusinessMainWindow::onEditPositionClicked()
{
    QListWidgetItem *item = ui->listWidgetPositions->currentItem();
    if (!item)
    {
        showStyledInformation("Должности", "Сначала выберите должность для редактирования.");
        return;
    }

    PositionEditDialog dialog(currentBusinessId, item->data(Qt::UserRole).toInt(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.positionName().isEmpty())
    {
        showStyledWarning("Ошибка", "Заполните наименование должности.");
        return;
    }

    if (!DatabaseManager::instance().updatePosition(
            item->data(Qt::UserRole).toInt(),
            dialog.positionName(),
            dialog.salaryText(),
            dialog.coveredPositionIds()))
    {
        showStyledError("Ошибка", "Не удалось изменить должность.");
        return;
    }

    loadPositions();
    loadMessageRecipients();
}

void BusinessMainWindow::onDeletePositionClicked()
{
    QListWidgetItem *item = ui->listWidgetPositions->currentItem();
    if (!item)
    {
        showStyledInformation("Должности", "Сначала выберите должность для удаления.");
        return;
    }

    if (showStyledQuestion(
            "Удаление должности",
            QString("Удалить должность \"%1\"?").arg(item->text())) != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deletePosition(item->data(Qt::UserRole).toInt()))
    {
        showStyledError("Ошибка", "Не удалось удалить должность.");
        return;
    }

    loadPositions();
    loadMessageRecipients();
}

void BusinessMainWindow::onPaymentEmployeeSelectionChanged()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    loadEmployeePaymentDetails(item ? item->data(Qt::UserRole).toInt() : -1);
}

bool BusinessMainWindow::paymentMatchesCurrentFilter(const ShiftPaymentInfo& payment) const
{
    if (!paymentsStatusFilterComboBox)
        return true;

    switch (paymentsStatusFilterComboBox->currentIndex())
    {
    case 0:
        return !payment.isPaid;
    case 1:
        return payment.isPaid;
    default:
        return true;
    }
}

void BusinessMainWindow::updatePaymentRevenueEditor()
{
    if (!paymentsRevenueEdit || !paymentsSaveRevenueButton || !paymentsShiftListWidget)
        return;

    QListWidgetItem *item = paymentsShiftListWidget->currentItem();
    const int row = item ? item->data(Qt::UserRole + 1).toInt() : -1;
    const bool validRow = row >= 0 && row < currentPaymentItems.size();

    if (!validRow)
    {
        paymentsRevenueEdit->clear();
        paymentsRevenueEdit->setEnabled(false);
        paymentsSaveRevenueButton->setEnabled(false);
        if (paymentsMarkPaidButton)
            paymentsMarkPaidButton->setEnabled(false);
        return;
    }

    const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
    const bool needsRevenue = paymentNeedsRevenue(payment);
    paymentsRevenueEdit->setText(payment.revenueAmount);
    paymentsRevenueEdit->setEnabled(needsRevenue);
    paymentsSaveRevenueButton->setEnabled(needsRevenue);
    if (paymentsMarkPaidButton)
        paymentsMarkPaidButton->setEnabled(!payment.isPaid);

    if (!needsRevenue)
        paymentsRevenueEdit->clear();
}

void BusinessMainWindow::onPaymentShiftSelectionChanged()
{
    updatePaymentRevenueEditor();
}

void BusinessMainWindow::onPaymentFilterChanged()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    loadEmployeePaymentDetails(item ? item->data(Qt::UserRole).toInt() : -1);
}

void BusinessMainWindow::onSavePaymentRevenueClicked()
{
    QListWidgetItem *selectedPaymentItem = paymentsShiftListWidget ? paymentsShiftListWidget->currentItem() : nullptr;
    const int row = selectedPaymentItem ? selectedPaymentItem->data(Qt::UserRole + 1).toInt() : -1;
    if (row < 0 || row >= currentPaymentItems.size())
    {
        showStyledInformation("Выплаты", "Сначала выберите смену.");
        return;
    }

    const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
    if (!paymentNeedsRevenue(payment))
    {
        showStyledInformation("Выплаты", "Для этой схемы оплаты выручка не требуется.");
        return;
    }

    if (!DatabaseManager::instance().updateShiftAssignmentRevenue(payment.assignmentId, paymentsRevenueEdit->text()))
    {
        showStyledError("Ошибка", "Не удалось сохранить выручку.");
        return;
    }

    const int employeeId = paymentsEmployeeListWidget && paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->data(Qt::UserRole).toInt()
        : -1;

    loadPaymentsEmployees();

    if (employeeId >= 0)
    {
        for (int i = 0; i < paymentsEmployeeListWidget->count(); ++i)
        {
            QListWidgetItem *employeeItem = paymentsEmployeeListWidget->item(i);
            if (employeeItem && employeeItem->data(Qt::UserRole).toInt() == employeeId)
            {
                paymentsEmployeeListWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void BusinessMainWindow::onMarkPaymentPaidClicked()
{
    QListWidgetItem *item = paymentsShiftListWidget->currentItem();
    if (!item)
    {
        showStyledInformation("Выплаты", "Сначала выберите смену для отметки оплаты.");
        return;
    }

    const int row = item->data(Qt::UserRole + 1).toInt();
    if (row >= 0 && row < currentPaymentItems.size())
    {
        const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
        if (paymentNeedsRevenue(payment) && payment.revenueAmount.trimmed().isEmpty())
        {
            showStyledInformation("Выплаты", "Сначала укажите выручку для этой смены.");
            return;
        }
    }

    if (!DatabaseManager::instance().markShiftAssignmentPaid(item->data(Qt::UserRole).toInt()))
    {
        showStyledError("Ошибка", "Не удалось отметить оплату смены.");
        return;
    }

    const int employeeId = paymentsEmployeeListWidget && paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->data(Qt::UserRole).toInt()
        : -1;

    loadPaymentsEmployees();

    if (employeeId >= 0)
    {
        for (int i = 0; i < paymentsEmployeeListWidget->count(); ++i)
        {
            QListWidgetItem *employeeItem = paymentsEmployeeListWidget->item(i);
            if (employeeItem && employeeItem->data(Qt::UserRole).toInt() == employeeId)
            {
                paymentsEmployeeListWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void BusinessMainWindow::onMarkAllPaymentsPaidClicked()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    if (!item)
    {
        showStyledInformation("Выплаты", "Сначала выберите сотрудника.");
        return;
    }

    const int employeeId = item->data(Qt::UserRole).toInt();
    for (const ShiftPaymentInfo &payment : std::as_const(currentPaymentItems))
    {
        if (!payment.isPaid && paymentNeedsRevenue(payment) && payment.revenueAmount.trimmed().isEmpty())
        {
            showStyledInformation("Выплаты", "Укажите выручку для всех процентных смен перед массовой оплатой.");
            return;
        }
    }

    if (!DatabaseManager::instance().markAllEmployeeAssignmentsPaid(currentBusinessId, employeeId))
    {
        showStyledError("Ошибка", "Не удалось отметить все выплаты как оплаченные.");
        return;
    }

    loadPaymentsEmployees();
    for (int i = 0; i < paymentsEmployeeListWidget->count(); ++i)
    {
        QListWidgetItem *employeeItem = paymentsEmployeeListWidget->item(i);
        if (employeeItem && employeeItem->data(Qt::UserRole).toInt() == employeeId)
        {
            paymentsEmployeeListWidget->setCurrentRow(i);
            break;
        }
    }
}

void BusinessMainWindow::onSendMessageClicked()
{
    if (currentBusinessId < 0)
        return;

    const bool isNewShiftMessage = messageTypeComboBox && messageTypeComboBox->currentText() == "Новая смена";
    const int shiftId = isNewShiftMessage && messageShiftComboBox
        ? messageShiftComboBox->currentData().toInt()
        : -1;

    if (isNewShiftMessage && shiftId <= 0)
    {
        showStyledWarning("Коммуникация", "Нет смен без уведомления. Создайте новую смену или выберите другой тип сообщения.");
        return;
    }

    QString messageText = messageTextEdit ? messageTextEdit->toPlainText().trimmed() : QString();
    if (isNewShiftMessage && messageText.isEmpty())
    {
        messageText = buildShiftNotificationText(shiftId);
        messageTextEdit->setPlainText(messageText);
    }

    if (messageText.isEmpty())
    {
        showStyledWarning("Коммуникация", "Введите текст сообщения.");
        return;
    }

    const QString recipientType = messageRecipientTypeComboBox->currentText();
    QString recipientCode = "all";
    int recipientEmployeeId = -1;
    QString recipientPositionName;
    QString recipientLabel = "Все сотрудники";

    if (recipientType == "Конкретный сотрудник")
    {
        if (messageEmployeeComboBox->currentText().trimmed().isEmpty())
        {
            showStyledWarning("Коммуникация", "Выберите сотрудника.");
            return;
        }
        recipientCode = "employee";
        recipientLabel = messageEmployeeComboBox->currentText();
        recipientEmployeeId = messageEmployeeComboBox->currentData().toInt();
    }
    else if (recipientType == "По должности")
    {
        if (messagePositionComboBox->currentText().trimmed().isEmpty())
        {
            showStyledWarning("Коммуникация", "Выберите должность.");
            return;
        }
        recipientCode = "position";
        recipientLabel = messagePositionComboBox->currentText();
        recipientPositionName = recipientLabel;
    }

    QString sendStatus = "РћР¶РёРґР°РµС‚ VK";
    if (true)
    {
        const QList<int> vkIds = isNewShiftMessage
            ? DatabaseManager::instance().getVkRecipientIdsForShiftOpenPositions(currentBusinessId, shiftId)
            : DatabaseManager::instance().getVkRecipientIds(
                  currentBusinessId,
                  recipientCode,
                  recipientEmployeeId,
                  recipientPositionName
                  );

        const QList<int> assignedVkIdsForShift = isNewShiftMessage
            ? DatabaseManager::instance().getAssignedShiftVkRecipientIds(shiftId)
            : QList<int>();

        if (vkIds.isEmpty() && assignedVkIdsForShift.isEmpty())
        {
            showStyledWarning("Коммуникация", "Нет подходящих сотрудников с VK ID.");
            return;
        }

        QJsonArray userIds;
        for (int vkId : vkIds)
            userIds.append(vkId);

        QJsonArray openPositionsJson;
        const QList<ShiftOpenPositionData> openPositions = isNewShiftMessage
            ? DatabaseManager::instance().getShiftOpenPositions(shiftId)
            : QList<ShiftOpenPositionData>();
        for (const ShiftOpenPositionData& openPosition : openPositions)
        {
            QJsonObject item;
            item["position"] = openPosition.positionName;
            item["count"] = openPosition.employeeCount;
            openPositionsJson.append(item);
        }

        QJsonObject payload;
        if (isNewShiftMessage)
            payload["shift_id"] = shiftId;
        else
            payload["shift_id"] = QJsonValue::Null;
        payload["message"] = messageText;
        payload["user_ids"] = userIds;
        payload["open_positions"] = openPositionsJson;

        VkSettingsData settings = DatabaseManager::instance().getVkSettings(currentBusinessId);
        QString backendUrl = settings.backendUrl.trimmed();
        if (backendUrl.isEmpty())
            backendUrl = "http://127.0.0.1:8000";
        while (backendUrl.endsWith('/'))
            backendUrl.chop(1);

        QNetworkAccessManager manager;
        QNetworkRequest request{QUrl(backendUrl + "/api/send-shift-notification")};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timer.start(15000);
        loop.exec();

        if (timer.isActive())
        {
            timer.stop();
            const QByteArray responseBody = reply->readAll();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (reply->error() != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300)
            {
                sendStatus = "Ошибка VK";
                showStyledWarning(
                    "VK backend",
                    QString("Backend не смог отправить уведомление.\n%1")
                        .arg(QString::fromUtf8(responseBody).isEmpty()
                                 ? reply->errorString()
                                 : QString::fromUtf8(responseBody))
                    );
            }
            else
            {
                const QJsonDocument responseJson = QJsonDocument::fromJson(responseBody);
                const QJsonArray results = responseJson.object().value("results").toArray();
                int sentCount = 0;
                int errorCount = 0;
                for (const QJsonValue& value : results)
                {
                    const QString status = value.toObject().value("status").toString();
                    if (status == "sent")
                        ++sentCount;
                    else
                        ++errorCount;
                }

                if (sentCount > 0 && errorCount == 0)
                    sendStatus = "РћС‚РїСЂР°РІР»РµРЅРѕ VK";
                else if (sentCount > 0)
                    sendStatus = "Р§Р°СЃС‚РёС‡РЅРѕ РѕС‚РїСЂР°РІР»РµРЅРѕ";
                else
                    sendStatus = "Ошибка VK";
            }
        }
        else
        {
            reply->abort();
            sendStatus = "Ошибка VK";
            showStyledWarning("VK backend", "Backend не ответил за 15 секунд. Проверьте, что uvicorn запущен.");
        }

        reply->deleteLater();
        if (vkIds.isEmpty() && !assignedVkIdsForShift.isEmpty())
            sendStatus = QString::fromUtf8("Отправлено назначенным");

        if (isNewShiftMessage)
        {
            const QList<int> assignedVkIds = DatabaseManager::instance().getAssignedShiftVkRecipientIds(shiftId);
            if (!assignedVkIds.isEmpty())
            {
                QJsonArray assignedUserIds;
                for (int vkId : assignedVkIds)
                    assignedUserIds.append(vkId);

                QJsonObject assignedPayload;
                assignedPayload["shift_id"] = shiftId;
                assignedPayload["message"] = QString::fromUtf8("Вы назначены на смену администратором.\n\n") + messageText;
                assignedPayload["user_ids"] = assignedUserIds;
                assignedPayload["open_positions"] = QJsonArray();

                QNetworkAccessManager assignedManager;
                QNetworkRequest assignedRequest{QUrl(backendUrl + "/api/send-shift-notification")};
                assignedRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                QEventLoop assignedLoop;
                QTimer assignedTimer;
                assignedTimer.setSingleShot(true);

                QNetworkReply *assignedReply = assignedManager.post(
                    assignedRequest,
                    QJsonDocument(assignedPayload).toJson(QJsonDocument::Compact)
                    );
                connect(assignedReply, &QNetworkReply::finished, &assignedLoop, &QEventLoop::quit);
                connect(&assignedTimer, &QTimer::timeout, &assignedLoop, &QEventLoop::quit);

                assignedTimer.start(10000);
                assignedLoop.exec();
                if (!assignedTimer.isActive())
                    assignedReply->abort();
                assignedReply->deleteLater();
            }
        }
    }

    if (!DatabaseManager::instance().createNotification(
            currentBusinessId,
            shiftId,
            recipientCode,
            recipientLabel,
            messageTypeComboBox->currentText(),
            messageText,
            "VK",
            "Ожидает VK"))
    {
        showStyledError("Ошибка", "Не удалось сохранить уведомление.");
        return;
    }

    messageTextEdit->clear();
    DatabaseManager::instance().updateLatestNotificationStatus(currentBusinessId, shiftId, sendStatus);

    loadUnnotifiedShiftOptions();
    loadNotificationsHistory();
}

void BusinessMainWindow::onClearMessageClicked()
{
    if (messageTextEdit)
        messageTextEdit->clear();
}

void BusinessMainWindow::onRefreshShiftResponsesClicked()
{
    loadShiftResponses();
}

void BusinessMainWindow::onRefreshStatisticsClicked()
{
    loadStatisticsSection();
}

void BusinessMainWindow::onSaveVkSettingsClicked()
{
    if (currentBusinessId < 0)
        return;

    VkSettingsData settings;
    settings.groupId = vkGroupIdEdit ? vkGroupIdEdit->text() : QString();
    settings.communityToken = vkCommunityTokenEdit ? vkCommunityTokenEdit->text() : QString();
    settings.backendUrl = vkBackendUrlEdit ? vkBackendUrlEdit->text() : QString();
    settings.isEnabled = vkEnabledCheckBox && vkEnabledCheckBox->isChecked();

    if (!DatabaseManager::instance().saveVkSettings(currentBusinessId, settings))
    {
        showStyledError("Ошибка", "Не удалось сохранить настройки VK.");
        return;
    }

    if (vkConnectionStatusLabel)
        vkConnectionStatusLabel->setText("Статус: настройки сохранены, подключение ещё не проверено.");
}

void BusinessMainWindow::onCheckVkConnectionClicked()
{
    if (!vkConnectionStatusLabel)
        return;

    {
        QNetworkAccessManager manager;
        QNetworkRequest request{QUrl("http://127.0.0.1:8000/health")};

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        QNetworkReply *reply = manager.get(request);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timer.start(5000);
        loop.exec();

        if (timer.isActive())
        {
            timer.stop();
            const bool ok = reply->error() == QNetworkReply::NoError
                && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
            vkConnectionStatusLabel->setText(ok
                ? QString::fromUtf8("Статус: backend доступен, VK-уведомления могут отправляться через единый бот.")
                : QString::fromUtf8("Статус: backend ответил с ошибкой. Проверьте, что сервер запущен и /health работает."));
        }
        else
        {
            reply->abort();
            vkConnectionStatusLabel->setText(QString::fromUtf8("Статус: backend не ответил за 5 секунд. Запустите uvicorn."));
        }

        reply->deleteLater();
        return;
    }

    const bool hasGroupId = vkGroupIdEdit && !vkGroupIdEdit->text().trimmed().isEmpty();
    const bool hasToken = vkCommunityTokenEdit && !vkCommunityTokenEdit->text().trimmed().isEmpty();
    const bool hasBackendUrl = vkBackendUrlEdit && !vkBackendUrlEdit->text().trimmed().isEmpty();

    if (hasGroupId && hasToken && hasBackendUrl)
    {
        vkConnectionStatusLabel->setText(
            "Статус: данные заполнены. Реальная проверка подключения будет добавлена после подключения backend-сервера.");
    }
    else
    {
        vkConnectionStatusLabel->setText(
            "Статус: заполните ID группы, токен сообщества и URL backend-сервера.");
    }
}

