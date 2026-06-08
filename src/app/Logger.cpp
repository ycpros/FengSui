// Logger.cpp
// 日志系统实现：文件写入 + 控制台输出，按日期滚动。

#include "app/Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <iostream>

namespace FengSui {

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::Logger() = default;

Logger::~Logger()
{
    shutdown();
}

void Logger::initialize()
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        // 重新初始化：先关闭旧文件
        shutdown();
    }

    // 创建日志目录
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);

    // 安装自定义消息处理器
    qInstallMessageHandler(messageHandler);

    openLogFile();
    m_initialized = true;
}

void Logger::shutdown()
{
    QMutexLocker locker(&m_mutex);

    // 恢复默认处理器
    qInstallMessageHandler(nullptr);

    if (m_logFile.isOpen()) {
        m_logFile.close();
    }

    m_initialized = false;
}

QString Logger::logFilePath() const
{
    return m_logFile.fileName();
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    instance().handleMessage(type, context, msg);
}

void Logger::handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QMutexLocker locker(&m_mutex);

    // 构建带时间戳和级别的日志行
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr;
    switch (type) {
        case QtDebugMsg:    levelStr = "DEBUG"; break;
        case QtInfoMsg:     levelStr = "INFO";  break;
        case QtWarningMsg:  levelStr = "WARN";  break;
        case QtCriticalMsg: levelStr = "ERROR"; break;
        case QtFatalMsg:    levelStr = "FATAL"; break;
    }

    // 文件相对路径（截取 src/ 之后的部分，便于定位）
    QString file = QString::fromUtf8(context.file ? context.file : "");
    int srcIndex = file.indexOf("src/");
    QString shortFile = (srcIndex >= 0) ? file.mid(srcIndex) : file;

    // 日志格式：[时间] [级别] [文件:行号] 消息内容
    QString logLine = QString("[%1] [%2] [%3:%4] %5")
                          .arg(timestamp, levelStr, shortFile)
                          .arg(context.line)
                          .arg(msg);

    // 输出到控制台（stderr for warnings and above）
    if (type >= QtWarningMsg) {
        std::cerr << logLine.toUtf8().constData() << std::endl;
    } else {
        std::cout << logLine.toUtf8().constData() << std::endl;
    }

    // 输出到日志文件
    if (m_logFile.isOpen()) {
        QTextStream stream(&m_logFile);
        stream << logLine << "\n";
        stream.flush();
    }
}

void Logger::openLogFile()
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString filePath = logDir + "/fengsui_" + dateStr + ".log";

    m_logFile.setFileName(filePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Failed to open log file: " << filePath.toUtf8().constData() << std::endl;
    }
}

} // namespace FengSui
