// UiStyle.cpp
// Centralized QSS snippets used by the main GUI pages.

#include "ui/UiStyle.h"

namespace FengSui {
namespace UiStyle {

QString appStyleSheet()
{
    return QStringLiteral(
        "QMainWindow { background: #f4f6f8; }"
        "QWidget { color: #1f2933; font-size: 13px; }"
        "QLabel#pageTitle { font-size: 20px; font-weight: 700; color: #101828; }"
        "QLabel#pageSubtitle { color: #667085; font-size: 12px; }"
        "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox {"
        "  background: #ffffff; border: 1px solid #d8dee8; border-radius: 6px;"
        "  padding: 6px 8px; selection-background-color: #1f8a8a;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QComboBox:focus {"
        "  border: 1px solid #1f8a8a;"
        "}"
        "QPushButton {"
        "  border: 1px solid #cfd7e3; border-radius: 6px; padding: 7px 12px;"
        "  background: #ffffff; color: #243447;"
        "}"
        "QPushButton:hover { background: #f4f7fb; }"
        "QPushButton:disabled { color: #98a2b3; background: #eef1f5; border-color: #e1e6ee; }"
        "QPushButton[primary=\"true\"] { background: #147d7e; border-color: #147d7e; color: #ffffff; }"
        "QPushButton[primary=\"true\"]:hover { background: #0f6f70; }"
        "QTabWidget::pane { border: 1px solid #d8dee8; border-radius: 8px; background: #ffffff; top: -1px; }"
        "QTabBar::tab {"
        "  background: #eef2f6; border: 1px solid #d8dee8; border-bottom: none;"
        "  padding: 8px 16px; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px;"
        "}"
        "QTabBar::tab:selected { background: #ffffff; color: #0f6f70; font-weight: 600; }"
        "QTableWidget {"
        "  background: #ffffff; border: 1px solid #d8dee8; border-radius: 8px;"
        "  gridline-color: #eef1f5; alternate-background-color: #fafbfc;"
        "}"
        "QHeaderView::section {"
        "  background: #f5f7fa; border: none; border-bottom: 1px solid #d8dee8;"
        "  padding: 8px; font-weight: 600; color: #475467;"
        "}"
        "QListWidget { background: #ffffff; border: 1px solid #d8dee8; border-radius: 8px; }"
        "QListWidget::item { border-bottom: 1px solid #eef1f5; }"
        "QListWidget::item:selected { background: #e7f5f4; color: #0f5f60; }"
        "QProgressBar {"
        "  border: 1px solid #d8dee8; border-radius: 5px; height: 10px;"
        "  background: #eef2f6; text-align: center;"
        "}"
        "QProgressBar::chunk { background: #1f8a8a; border-radius: 4px; }"
        "QCheckBox { spacing: 8px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    );
}

QString pageTitleStyle()
{
    return QStringLiteral("font-size: 20px; font-weight: 700; color: #101828;");
}

QString sectionTitleStyle()
{
    return QStringLiteral("font-size: 14px; font-weight: 700; color: #243447;");
}

QString mutedTextStyle()
{
    return QStringLiteral("font-size: 12px; color: #667085;");
}

QString primaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton { background: #147d7e; color: #ffffff; border: 1px solid #147d7e;"
        "  border-radius: 6px; padding: 7px 14px; font-weight: 600; }"
        "QPushButton:hover { background: #0f6f70; }"
        "QPushButton:disabled { background: #c7d0dc; border-color: #c7d0dc; color: #ffffff; }"
    );
}

QString secondaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton { background: #ffffff; color: #243447; border: 1px solid #cfd7e3;"
        "  border-radius: 6px; padding: 7px 12px; }"
        "QPushButton:hover { background: #f4f7fb; }"
    );
}

QString pillStyle(const QString& background, const QString& foreground)
{
    return QStringLiteral(
               "background: %1; color: %2; border-radius: 10px; padding: 3px 8px;"
               "font-size: 12px; font-weight: 600;")
        .arg(background, foreground);
}

} // namespace UiStyle
} // namespace FengSui
