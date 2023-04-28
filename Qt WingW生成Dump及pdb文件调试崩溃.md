# Qt-mingw 生成dump文件并使用vs调试定位错误代码

## 简介

> 最近因为维护的项目出现崩溃，客户现场的log无法定位，因此需要使用dump文件调试于是查找资料添加了一个崩溃生成dump文件功能

-----

## 环境

>Qt5.12.9-MingW版本/Qt5.5.1-MingW版本，Visual Studio 2017/Visual Studio 2013，Windows系统

----

## 配置pro文件，在exe文件加入调试信息

```c++
QMAKE_CFLAGS_RELEASE += -g
QMAKE_CXXFLAGS_RELEASE += -g
#禁止优化
QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_LFLAGS_RELEASE = -mthreads -W
#生成dump文件需要
LIBS += -lDbgHelp
#加上下面两行，否则用vs调试时，提示“未找到xxx.exe"
QMAKE_CXXFLAGS += -g
QMAKE_CFLAGS += -g
```

### 核心代码

```c++
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
//! 异常处理回调（崩溃时自动调用）
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

    // 创建dump文件
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
    
    // 将崩溃信息写入dump文件中
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
    //! 注册异常奔溃回调
    SetUnhandledExceptionFilter(exceptionCallback);
#endif
//测试接口
//    crashNullPtr();
//    crashFloatPtr();
//    crashWildPtr();
//    ArrayExceed();
    VectorExceed();
//    VectorExceed2();
//    ListExceed();

    return a.exec();
}


```

## 生成pdb文件

>注：WingW编译器并不会像MSVC一样自动生成pdb文件需要使用第三方工具生成。这里使用github开源工具**cv2pdb.exe**生成。

### 下载工具

[下载链接](https://github.com/rainers/cv2pdb)

![1682242797578](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682242797578.png)

下载下来使用vs2017编译源码的到cv2pdb.exe文件。

### 生成方法

>使用命令行(cmd)
>
>1. 打开命令行，cd到cv2pdb.exe文件路径下
>2. 执行指令 “cv2pdb.exe  xxx.exe”即可在xxx.exe同级路径下生成xxx.pdb文件

![1682243028125](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682243028125.png)

### 出现错误：

> **qBreakpadTest.pdb: cannot load PDB helper DLL**

#### 原因

**错误原因：电脑没有安装Visual Studio环境**

> 安装Visual Studio即可解决

------

## 调试

---

### 使用Visual Studio IDE调试（推荐）

#### 加载pdb文件

> 使用Visual Studio打开需要调试的pdb文件(将exe文件和pdb放到同一目录下，然后设置符合路径)

---

#### 设置调试pdb目录位置

![1682243563126](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682243563126.png)

![1682243641185](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682243641185.png)

---

#### 设置源码目录

在解决方案-属性，打开弹窗设置

![1682243771250](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682243771250.png)

#### 调试

> 点击 “使用 仅限本机 进行调试”

![1682243822104](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682243822104.png)

![img](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/kk_image.jpg)

---

### 使用Windows调试器windbg.exe调试

> [下载链接](https://developer.microsoft.com/zh-cn/windows/downloads/windows-sdk/)
>
> 此链接是下载Windows SDK安装包的，Windows SDK安装包含windbg.exe。

#### 调试步骤

> 1. 双击打开windbg.exe，选择相应版本
>
> ![1682405865975](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682405908877.png)
>
> 2. 依次完成如下配置
> 打开File 需要完成 Symbol File Path 、Source File Path、Image File Path 的配置
>
>   1. Symbol File Path ：这里就是加載pdb文件的路徑
>
>   2. Source File Path: 加载程序代码 存放的路径（注意切回發佈版本的代碼）
>
>   3. Image File Path: 加載exe 存放的路徑
>
>   4. 選擇Open Crash Dump 導入生成的dmp文件
>
>   5. 输入命令 !analyze -v ，等待几秒后会打印出错误信息
>
> ![1682407236891](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682407236891.png)
>
> 
>
>   ![1682407088643](https://cdn.jsdelivr.net/gh/ouyujia/blog-img/img/1682407088643.png)
>

1、Symbol File Path ：這裏就是加載pdb文件的路徑，我們直接使用

D:/Project/DumTools/
1
2、 Source File Path: 加载程序代码 存放的路径（注意切回發佈版本的代碼）

D:/Project/Demo/
1
3、Image File Path: 加載exe 存放的路徑

D:/Project/DumTools/
1
4、選擇Open Crash Dump 導入生成的dmp文件

## 注意

> * 用于崩溃调试的dump、pdb、源码需要是同一个版本，一般为发布版本（即客户版本）
> * 用于崩溃调试的dump、pdb、源码需要区分debug和release否则无法调试

## 参考资料

>1. [Qt-mingw 生成dump文件并使用vs调试定位错误代码_qt生成dump文件](https://blog.csdn.net/LiHong333/article/details/129798322?spm=1001.2014.3001.5502)
>2. [Qt如何在Release编译下怎么调试？怎么生成pdb文件](https://libaineu2004.blog.csdn.net/article/details/108310384?spm=1001.2014.3001.5502)
>3. [ Window端Qt Create dmp的生成与解析](https://blog.csdn.net/HeroGuo_JP/article/details/105383752)

