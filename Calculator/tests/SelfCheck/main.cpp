/**
 * @file main.cpp
 * @brief 计算器自测程序
 *
 * 通过程序方式「点击」按钮或「按下」键盘对所有计算路径做回归验证，
 * 覆盖：基本运算、连续运算、除零、退格、清除、重复小数点、运算符连按、
 *       计算完成后继续输入、鼠标键盘混合输入等典型场景。
 *
 * 用法：CalculatorSelfCheck        （退出码 0 表示全部通过）
 */

#include "mainwindow.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

#include <iostream>

namespace {

struct TestCase
{
    QString       name;        // 用例名称
    QStringList   actions;     // 操作序列
    QString       expected;    // 期望的显示屏内容
};

// 需要同时兼容鼠标按钮名和键盘标记，约定：
//   普通字符串            → 点击对应 objectName 的按钮
//   Key:<Qt::Key 枚举名>  → 向窗口发送该按键事件
bool isKeyAction(const QString &action)
{
    return action.startsWith(QStringLiteral("Key:"));
}

/** 把一个简短的按键名解析成 Qt::Key */
Qt::Key parseKey(const QString &text)
{
    static const QHash<QString, Qt::Key> table = {
        {QStringLiteral("0"), Qt::Key_0},
        {QStringLiteral("1"), Qt::Key_1},
        {QStringLiteral("2"), Qt::Key_2},
        {QStringLiteral("3"), Qt::Key_3},
        {QStringLiteral("4"), Qt::Key_4},
        {QStringLiteral("5"), Qt::Key_5},
        {QStringLiteral("6"), Qt::Key_6},
        {QStringLiteral("7"), Qt::Key_7},
        {QStringLiteral("8"), Qt::Key_8},
        {QStringLiteral("9"), Qt::Key_9},
        {QStringLiteral("Plus"), Qt::Key_Plus},
        {QStringLiteral("Minus"), Qt::Key_Minus},
        {QStringLiteral("Asterisk"), Qt::Key_Asterisk},
        {QStringLiteral("Slash"), Qt::Key_Slash},
        {QStringLiteral("Equal"), Qt::Key_Equal},
        {QStringLiteral("Enter"), Qt::Key_Enter},
        {QStringLiteral("Return"), Qt::Key_Return},
        {QStringLiteral("Period"), Qt::Key_Period},
        {QStringLiteral("Backspace"), Qt::Key_Backspace},
        {QStringLiteral("Escape"), Qt::Key_Escape},
        {QStringLiteral("Delete"), Qt::Key_Delete},
        {QStringLiteral("Percent"), Qt::Key_Percent},
    };
    return table.value(text, Qt::Key_unknown);
}

const QString kClearButton = QStringLiteral("pushButton_Clear");
const QString kBackspaceButton = QStringLiteral("pushButton_Backspace");
const QString kEqualButton = QStringLiteral("pushButton_Equal");
const QString kPointButton = QStringLiteral("pushButton_Point");
const QString kPercentButton = QStringLiteral("pushButton_Percent");
const QString kSignButton = QStringLiteral("pushButton_Sign");

QString digitButton(int n)
{
    return QStringLiteral("pushButton_%1").arg(n);
}

QString opButton(const QString &op)
{
    if (op == QStringLiteral("+")) return QStringLiteral("pushButton_Add");
    if (op == QStringLiteral("-")) return QStringLiteral("pushButton_Subtract");
    if (op == QStringLiteral("*")) return QStringLiteral("pushButton_Multiply");
    return QStringLiteral("pushButton_Divide");
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const std::vector<TestCase> cases = {
        {QStringLiteral("基本加法"),            {digitButton(1), opButton("+"), digitButton(2), kEqualButton}, QStringLiteral("3")},
        {QStringLiteral("两位数加法"),          {digitButton(1), digitButton(2), opButton("+"), digitButton(8), kEqualButton}, QStringLiteral("20")},
        {QStringLiteral("减法"),                {digitButton(9), opButton("-"), digitButton(3), kEqualButton}, QStringLiteral("6")},
        {QStringLiteral("乘法"),                {digitButton(6), opButton("*"), digitButton(7), kEqualButton}, QStringLiteral("42")},
        {QStringLiteral("除法"),                {digitButton(8), opButton("/"), digitButton(2), kEqualButton}, QStringLiteral("4")},
        {QStringLiteral("小数参与运算"),        {digitButton(1), kPointButton, digitButton(5), opButton("+"), digitButton(2), kPointButton, digitButton(5), kEqualButton}, QStringLiteral("4")},
        {QStringLiteral("重复小数点被忽略"),    {digitButton(1), kPointButton, kPointButton, digitButton(5)}, QStringLiteral("1.5")},
        {QStringLiteral("先按小数点自动补零"),  {kPointButton, digitButton(5)}, QStringLiteral("0.5")},
        {QStringLiteral("前导零被去掉"),        {digitButton(0), digitButton(0), digitButton(5)}, QStringLiteral("5")},
        {QStringLiteral("退格删除一位"),        {digitButton(1), digitButton(2), digitButton(3), kBackspaceButton}, QStringLiteral("12")},
        {QStringLiteral("退格删空回落到零"),    {digitButton(5), kBackspaceButton}, QStringLiteral("0")},
        {QStringLiteral("小数退格后再输入"),    {digitButton(1), kPointButton, digitButton(5), kBackspaceButton, digitButton(2)}, QStringLiteral("1.2")},
        {QStringLiteral("清除后重新计算"),      {digitButton(9), digitButton(9), kClearButton, digitButton(3), opButton("+"), digitButton(4), kEqualButton}, QStringLiteral("7")},
        {QStringLiteral("连续运算"),            {digitButton(2), opButton("+"), digitButton(3), opButton("+"), digitButton(4), kEqualButton}, QStringLiteral("9")},
        {QStringLiteral("连按运算符取最后一个"),{digitButton(5), opButton("+"), opButton("*"), digitButton(2), kEqualButton}, QStringLiteral("10")},
        {QStringLiteral("除零提示"),            {digitButton(8), opButton("/"), digitButton(0), kEqualButton}, QStringLiteral("除数不能为零")},
        {QStringLiteral("除零后输入被忽略"),    {digitButton(8), opButton("/"), digitButton(0), kEqualButton, digitButton(5)}, QStringLiteral("除数不能为零")},
        {QStringLiteral("除零后清除可恢复"),    {digitButton(8), opButton("/"), digitButton(0), kEqualButton, kClearButton, digitButton(6), opButton("+"), digitButton(1), kEqualButton}, QStringLiteral("7")},
        {QStringLiteral("算完后直接输数字"),    {digitButton(2), opButton("+"), digitButton(3), kEqualButton, digitButton(7)}, QStringLiteral("7")},
        {QStringLiteral("算完后接着按运算符"),  {digitButton(2), opButton("+"), digitButton(3), kEqualButton, opButton("+"), digitButton(5), kEqualButton}, QStringLiteral("10")},
        {QStringLiteral("算完后直接退格"),      {digitButton(2), opButton("+"), digitButton(3), kEqualButton, kBackspaceButton}, QStringLiteral("5")},
        {QStringLiteral("连按等号保持结果"),    {digitButton(2), opButton("+"), digitButton(3), kEqualButton, kEqualButton}, QStringLiteral("5")},
        {QStringLiteral("正负号切换"),          {digitButton(5), kSignButton}, QStringLiteral("-5")},
        {QStringLiteral("负数参与运算"),        {digitButton(5), kSignButton, opButton("+"), digitButton(3), kEqualButton}, QStringLiteral("-2")},
        {QStringLiteral("百分号"),              {digitButton(5), digitButton(0), kPercentButton}, QStringLiteral("0.5")},
        {QStringLiteral("无限小数保留12位"),    {digitButton(1), opButton("/"), digitButton(3), kEqualButton}, QStringLiteral("0.333333333333")},
        {QStringLiteral("未输数字先按运算符"),  {kClearButton, opButton("+"), digitButton(5), kEqualButton}, QStringLiteral("5")},
        {QStringLiteral("键盘完成加法"),        {QStringLiteral("Key:1"), QStringLiteral("Key:2"), QStringLiteral("Key:Plus"), QStringLiteral("Key:3"), QStringLiteral("Key:Enter")}, QStringLiteral("15")},
        {QStringLiteral("键盘退格与Esc清除"),   {QStringLiteral("Key:9"), QStringLiteral("Key:9"), QStringLiteral("Key:Backspace"), QStringLiteral("Key:9"), QStringLiteral("Key:Escape")}, QStringLiteral("0")},
        {QStringLiteral("键盘小键盘数字与回车"),{QStringLiteral("Key:7"), QStringLiteral("Key:8"), QStringLiteral("Key:Slash"), QStringLiteral("Key:2"), QStringLiteral("Key:Return")}, QStringLiteral("39")},
        {QStringLiteral("鼠标键盘混合输入"),    {digitButton(1), digitButton(0), QStringLiteral("Key:Minus"), digitButton(4), QStringLiteral("Key:Equal")}, QStringLiteral("6")},
        {QStringLiteral("键盘乘除符号"),        {QStringLiteral("Key:6"), QStringLiteral("Key:Asterisk"), QStringLiteral("Key:7"), QStringLiteral("Key:Equal")}, QStringLiteral("42")},
    };

    int passed = 0;
    int failed = 0;

    for (const TestCase &testCase : cases) {
        MainWindow window;
        for (const QString &action : testCase.actions) {
            if (isKeyAction(action)) {
                const Qt::Key key = parseKey(action.mid(4));
                QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
                QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
                QApplication::sendEvent(&window, &press);
                QApplication::sendEvent(&window, &release);
            } else {
                QPushButton *button = window.findChild<QPushButton *>(action);
                if (button == nullptr) {
                    std::cout << "[ERROR] 找不到按钮: " << action.toStdString() << std::endl;
                    ++failed;
                    continue;
                }
                button->click();
            }
        }

        const QLineEdit *display = window.findChild<QLineEdit *>(QStringLiteral("lineEditDisplay"));
        const QString actual = display->text();

        if (actual == testCase.expected) {
            ++passed;
            std::cout << "[PASS] " << testCase.name.toStdString() << std::endl;
        } else {
            ++failed;
            std::cout << "[FAIL] " << testCase.name.toStdString()
                      << "  期望: " << testCase.expected.toStdString()
                      << "  实际: " << actual.toStdString() << std::endl;
        }
    }

    std::cout << "\n通过 " << passed << " / " << (passed + failed) << " 项" << std::endl;
    return failed == 0 ? 0 : 1;
}
