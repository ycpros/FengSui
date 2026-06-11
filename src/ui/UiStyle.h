// UiStyle.h
// Shared Qt Widgets styling helpers for the FengSui desktop shell.

#pragma once

#include <QString>

namespace FengSui {
namespace UiStyle {

QString appStyleSheet();
QString pageTitleStyle();
QString sectionTitleStyle();
QString mutedTextStyle();
QString primaryButtonStyle();
QString secondaryButtonStyle();
QString pillStyle(const QString& background, const QString& foreground);

} // namespace UiStyle
} // namespace FengSui
