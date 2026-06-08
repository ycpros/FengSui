// Logger.h
// 应用日志系统，基于 qInstallMessageHandler 实现。
// 支持同时输出到文件和控制台，按日志级别过滤文件输出。

#pragma once

#include <QObject>
#include <QFile>
#include <QMutex>

namespace FengSui {

// 日志系统单例。
// 在 Application::initialize() 中初始化，全局可用 qDebug/qInfo/qWarning/qCritical 输出。
// 文件日志保存在 {AppData}/FengSui/logs/ 目录下，按日期滚动。
class Logger : public QObject {
    Q_OBJECT

public:
    // 获取单例实例
    static Logger& instance();

    // 初始化日志系统：安装消息处理器、创建日志目录、打开当天日志文件。
    // 可多次调用，重复调用会先关闭旧文件再重新初始化。
    void initialize();

    // 关闭日志文件，恢复默认消息处理器。应用退出前调用。
    void shutdown();

    // 返回日志文件路径，用于诊断导出
    QString logFilePath() const;

private:
    Logger();
    ~Logger() override;
    Q_DISABLE_COPY(Logger)

    // Qt 消息处理器回调（静态函数，通过 instance() 分发到成员方法）
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    // 实例级消息处理
    void handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    // 打开当天的日志文件
    void openLogFile();

    QFile m_logFile;
    QMutex m_mutex;           // 多线程安全
    bool m_initialized = false;
};

} // namespace FengSui
