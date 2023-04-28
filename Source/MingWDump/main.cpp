#include "mainwindow.h"
#include <QApplication>

#include <QDir>
#include <qdebug.h>
#include <QDateTime>
#include <QFile>
#include <qglobal.h>
#include <QVector>
#include <QList>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#endif


#ifdef Q_OS_WIN
static LONG WINAPI exceptionCallback(struct _EXCEPTION_POINTERS* exceptionInfo)
{
    QCoreApplication *app = QApplication::instance();

    QString savePath = app->applicationDirPath() + "dump/";
    qDebug()<<"save path :"<<savePath;
    QDir dir(savePath);
    if (!dir.exists() && !dir.mkpath(savePath)) {
        app->exit(E_UNEXPECTED);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    savePath.append("assit_");
    savePath.append(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"));
    savePath.append(".dmp");

    HANDLE dump = CreateFileW(savePath.toStdWString().c_str(), GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == dump) {
        app->exit(E_UNEXPECTED);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    MINIDUMP_EXCEPTION_INFORMATION miniDumpExceptionInfo;
    miniDumpExceptionInfo.ExceptionPointers = exceptionInfo;
    miniDumpExceptionInfo.ThreadId = GetCurrentThreadId();
    miniDumpExceptionInfo.ClientPointers = TRUE;
    DWORD idProcess = GetCurrentProcessId();
    MiniDumpWriteDump(GetCurrentProcess(), idProcess, dump,
        MiniDumpNormal, &miniDumpExceptionInfo, NULL, NULL);

    CloseHandle(dump);

    app->exit(E_UNEXPECTED);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif


//! 空指针
void crashNullPtr()
{
    //空指针测试
    volatile int* p_a = (int*)(NULL); *p_a = 1;
}

//! 悬浮指针
void crashFloatPtr()
{
    int a = 5;
    volatile int* p_a = &a;
    delete p_a;
    *p_a = 10;
    qDebug()<<"*p_a :"<< *p_a;
}

//! 野指针
void crashWildPtr()
{
    volatile int* p_a;
    *p_a = 5;
    qDebug()<<"*p_a :"<< *p_a;

}

//! array越界
void ArrayExceed()
{
    int arr[2] = {0, 1};

    for(int i = 2; i < 10; ++i)
    {
        arr[i] = i + 1;
        qDebug()<<"arr[i]"<< arr[i];
    }
    arr[1000] = 11111;
    qDebug()<<"arr[10000]"<< arr[10000];
}

//! Vector越界
void VectorExceed()
{
    QVector<int> vec = {0, 1};
    int value;
    for(int i = 2; i < 10; ++i)
    {
        value = vec.at(i);
        qDebug()<<"arr.at(i)"<< value;
    }
    qDebug()<<"arr[10000]"<< vec.at(10000);
}

//! Vector越界
void VectorExceed2()
{
    QVector<int> vec = {0, 1};
    for(int i = 2; i < 10; ++i)
    {
        vec[i] = i + 1;
        qDebug()<<"arr[i]"<< vec[i];
    }
    vec[1000] = 11111;
    qDebug()<<"arr[10000]"<< vec[10000];
}

//! list越界
void ListExceed()
{
    QList<int> list = {0, 1};
    list[2] = 2;
}

//! Map
//void MapExceed()
//{
//    QMap<int, QString> map;
//    map.insert(0, "frist");
//    map.insert(1, "second");
//    map.remove(3);
//}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(exceptionCallback);
#endif

//    crashNullPtr();
//    crashFloatPtr();
//    crashWildPtr();
//    ArrayExceed();
    VectorExceed();
//    VectorExceed2();
//    ListExceed();

    return a.exec();
}

