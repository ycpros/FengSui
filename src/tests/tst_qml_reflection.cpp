// tst_qml_reflection.cpp
// 阶段 1 数据桥接底座回归测试：验证 models 层 struct 的 Q_GADGET 属性反射，
// 以及命名空间级枚举（Q_ENUM_NS）可经元对象解析。
// 注：QML 端到端（import FengSui.Ui + Enums）依赖模块注册，由主程序运行时验证。

#include <QtTest>
#include <QMetaProperty>
#include <QQmlEngine>
#include <QQmlComponent>

#include "models/Message.h"
#include "models/PeerInfo.h"
#include "models/TransferTask.h"
#include "models/ModelEnums.h"

using namespace FengSui;

class TestQmlReflection : public QObject {
    Q_OBJECT

private slots:
    // Q_GADGET 属性反射：能通过元对象按名读到字段值
    void gadgetPropertyReflection()
    {
        Message msg;
        msg.content = QStringLiteral("你好");
        msg.status = MessageStatus::Delivered;

        const QMetaObject& mo = Message::staticMetaObject;
        const int idx = mo.indexOfProperty("content");
        QVERIFY(idx >= 0);
        const QVariant v = mo.property(idx).readOnGadget(&msg);
        QCOMPARE(v.toString(), QStringLiteral("你好"));

        // 枚举属性也应可读
        const int sIdx = mo.indexOfProperty("status");
        QVERIFY(sIdx >= 0);
        QCOMPARE(mo.property(sIdx).readOnGadget(&msg).toInt(),
                 static_cast<int>(MessageStatus::Delivered));
    }

    // PeerInfo 的 Q_GADGET 反射
    void peerInfoReflection()
    {
        PeerInfo peer;
        peer.displayName = QStringLiteral("Lily-PC");
        peer.online = true;

        const QMetaObject& mo = PeerInfo::staticMetaObject;
        QCOMPARE(mo.property(mo.indexOfProperty("displayName"))
                     .readOnGadget(&peer).toString(),
                 QStringLiteral("Lily-PC"));
        QCOMPARE(mo.property(mo.indexOfProperty("online"))
                     .readOnGadget(&peer).toBool(),
                 true);
    }

    // 命名空间级枚举反射：Q_ENUM_NS 使枚举可经元对象解析
    void namespaceEnumReflection()
    {
        const QMetaObject& mo = FengSui::staticMetaObject;
        const int ei = mo.indexOfEnumerator("TransferStatus");
        QVERIFY(ei >= 0);
        const QMetaEnum me = mo.enumerator(ei);
        QCOMPARE(me.keyToValue("Completed"),
                 static_cast<int>(TransferStatus::Completed));
    }

    // QML 端到端：把 FengSui 命名空间枚举以 "Enums" 注册后，QML 能访问枚举值。
    // 这复现主程序 qt_add_qml_module 生成的注册（qmlRegisterNamespaceAndRevisions），
    // 确保 QML 中 import 后 Enums.Delivered / Enums.Completed 解析正确。
    void qmlNamespaceEnumEndToEnd()
    {
        qmlRegisterNamespaceAndRevisions(&FengSui::staticMetaObject,
                                         "FengSui.Test", 1);
        // 在该 URI 下把命名空间元对象暴露为可导入类型名 Enums
        qmlRegisterUncreatableMetaObject(FengSui::staticMetaObject,
                                         "FengSui.Test", 1, 0, "Enums",
                                         QStringLiteral("仅枚举"));

        QQmlEngine engine;
        QQmlComponent component(&engine);
        // 用作用域访问 Enums.<枚举名>.<键>，避免不同枚举同名键（如 Failed）冲突。
        component.setData(R"(
            import QtQml
            import FengSui.Test
            QtObject {
                property int delivered: Enums.MessageStatus.Delivered
                property int completed: Enums.TransferStatus.Completed
                property int msgFailed: Enums.MessageStatus.Failed
                property int xferFailed: Enums.TransferStatus.Failed
            }
        )", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("delivered").toInt(),
                 static_cast<int>(MessageStatus::Delivered));
        QCOMPARE(obj->property("completed").toInt(),
                 static_cast<int>(TransferStatus::Completed));
        // 作用域访问能正确区分两个同名 Failed
        QCOMPARE(obj->property("msgFailed").toInt(),
                 static_cast<int>(MessageStatus::Failed));
        QCOMPARE(obj->property("xferFailed").toInt(),
                 static_cast<int>(TransferStatus::Failed));
    }
};

QTEST_MAIN(TestQmlReflection)
#include "tst_qml_reflection.moc"
