// QmlEnums.h
// 把 models 层的枚举以 QML_FOREIGN_NAMESPACE 方式暴露给 QML 模块 FengSui.Ui。
//
// 背景：models 层的枚举定义在 FengSui 命名空间（Q_NAMESPACE + Q_ENUM_NS），
// 但 qt_add_qml_module 的静态类型注册要求可导入的枚举通过 QML_ELEMENT/QML_FOREIGN
// 声明并纳入模块 SOURCES。为避免改动 models 命名空间本身（会波及 core/storage），
// 这里在 UI 层新建一个仅供 QML 使用的外部命名空间包装，QML 中通过
//   import FengSui.Ui
//   ... Enums.Delivered / Enums.Completed ...
// 访问，语义与 models 枚举一致。

#pragma once

#include "models/ModelEnums.h"
#include <QtQmlIntegration>

namespace FengSuiQmlEnums {

// 外部命名空间包装：把 FengSui 命名空间的枚举反射转发到 QML 类型名 "Enums"。
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(FengSui)
QML_NAMED_ELEMENT(Enums)

} // namespace FengSuiQmlEnums
