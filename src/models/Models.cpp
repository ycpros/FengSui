// Models.cpp
// 汇总编译 models 层所有 Q_GADGET / Q_NAMESPACE 头的元对象代码。
//
// 这些数据结构定义在纯头文件中（Q_GADGET/Q_ENUM_NS），本身不产生编译单元。
// moc 生成的 staticMetaObject 需要一个 .cpp 承载并链接。凡是引用这些类型元对象
// 的目标（主程序、各测试）都应链接本文件，避免 "无法解析的外部符号 staticMetaObject"。
//
// 通过 include 头文件触发 AUTOMOC 为其生成元对象；无需在此写任何代码。

#include "models/ModelEnums.h"
#include "models/PeerInfo.h"
#include "models/Message.h"
#include "models/Conversation.h"
#include "models/TransferTask.h"
#include "models/SharedFolder.h"
#include "models/AccessGrant.h"
#include "models/DownloadLog.h"
