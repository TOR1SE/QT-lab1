/**
 * @file main.cpp
 * @brief 演示模式入口：自动按脚本驱动计算器，用于录制测试视频
 *
 * 通过 QTimer 按时间轴依次触发按钮点击与键盘事件，节奏放慢便于观看。
 * 程序真实显示在屏幕上，由外部 ffmpeg -f gdigrab -i title="计算器 - Qt" 录制。
 */
#include "mainwindow.h"

#include <QApplication>
#include <QKeyEvent>
#include <QPushButton>
#include <QTimer>

namespace {

struct Step
{
    int  delayMs;      // 距上一步的间隔
    bool isKey;        // true: 发送键盘事件；false: 点击按钮
    int  key;          // Qt::Key
    const char *buttonName;
};

/** 演示脚本：鼠标带小数运算 → 键盘计算 → 退格清除 → 除零 → 连续运算 → 连按等号 → 重复小数点 */
const std::vector<Step> kScript = {
    // ---- 鼠标：12.5 + 3 = 15.5 ----
    {3000, false, 0, "pushButton_1"},
    {700,  false, 0, "pushButton_2"},
    {700,  false, 0, "pushButton_Point"},
    {700,  false, 0, "pushButton_5"},
    {900,  false, 0, "pushButton_Add"},
    {900,  false, 0, "pushButton_3"},
    {900,  false, 0, "pushButton_Equal"},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 键盘：45.6 − 12.3 = 33.3 ----
    {1500, true, Qt::Key_4, nullptr},
    {600,  true, Qt::Key_5, nullptr},
    {600,  true, Qt::Key_Period, nullptr},
    {600,  true, Qt::Key_6, nullptr},
    {800,  true, Qt::Key_Minus, nullptr},
    {600,  true, Qt::Key_1, nullptr},
    {600,  true, Qt::Key_2, nullptr},
    {600,  true, Qt::Key_Period, nullptr},
    {600,  true, Qt::Key_3, nullptr},
    {800,  true, Qt::Key_Return, nullptr},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 键盘：9 × 6 = 54 ----
    {1200, true, Qt::Key_9, nullptr},
    {700,  true, Qt::Key_Asterisk, nullptr},
    {700,  true, Qt::Key_6, nullptr},
    {700,  true, Qt::Key_Return, nullptr},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 退格：12345 → 123 ----
    {1200, false, 0, "pushButton_1"},
    {500,  false, 0, "pushButton_2"},
    {500,  false, 0, "pushButton_3"},
    {500,  false, 0, "pushButton_4"},
    {500,  false, 0, "pushButton_5"},
    {1200, false, 0, "pushButton_Backspace"},
    {900,  false, 0, "pushButton_Backspace"},
    {3000, true, Qt::Key_Escape, nullptr},
    // ---- 除零 ----
    {1500, false, 0, "pushButton_8"},
    {700,  false, 0, "pushButton_Divide"},
    {700,  false, 0, "pushButton_0"},
    {800,  false, 0, "pushButton_Equal"},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 连续运算：2 + 3 + 4 = 9 ----
    {1500, false, 0, "pushButton_2"},
    {700,  false, 0, "pushButton_Add"},
    {700,  false, 0, "pushButton_3"},
    {900,  false, 0, "pushButton_Add"},
    {700,  false, 0, "pushButton_4"},
    {800,  false, 0, "pushButton_Equal"},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 连按等号：2 + 3 = 5, = 8, = 11 ----
    {1500, false, 0, "pushButton_2"},
    {700,  false, 0, "pushButton_Add"},
    {700,  false, 0, "pushButton_3"},
    {800,  false, 0, "pushButton_Equal"},
    {1200, false, 0, "pushButton_Equal"},
    {1200, false, 0, "pushButton_Equal"},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 重复小数点：7 . . 8 → 7.8 ----
    {1500, false, 0, "pushButton_7"},
    {800,  false, 0, "pushButton_Point"},
    {800,  false, 0, "pushButton_Point"},
    {800,  false, 0, "pushButton_8"},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 科学键：81 √ = 9 ----
    {2000, false, 0, "pushButton_8"},
    {500,  false, 0, "pushButton_1"},
    {800,  false, 0, "pushButton_Sqrt"},
    {2200, false, 0, "pushButton_Clear"},
    // ---- 科学键：9 x² = 81 ----
    {1200, false, 0, "pushButton_9"},
    {800,  false, 0, "pushButton_Square"},
    {2200, false, 0, "pushButton_Clear"},
    // ---- 科学键：8 1/x = 0.125 ----
    {1200, false, 0, "pushButton_8"},
    {800,  false, 0, "pushButton_Inverse"},
    {2200, false, 0, "pushButton_Clear"},
    // ---- CE：12 + 3 CE 4 = 16 ----
    {1200, false, 0, "pushButton_1"},
    {500,  false, 0, "pushButton_2"},
    {700,  false, 0, "pushButton_Add"},
    {500,  false, 0, "pushButton_3"},
    {1000, false, 0, "pushButton_ClearEntry"},
    {700,  false, 0, "pushButton_4"},
    {700,  false, 0, "pushButton_Equal"},
    {2500, false, 0, "pushButton_Clear"},
    // ---- 键盘科学键：36 @ √、12 q 平方 ----
    {1500, true, Qt::Key_3, nullptr},
    {500,  true, Qt::Key_6, nullptr},
    {700,  true, Qt::Key_At, nullptr},
    {2200, false, 0, "pushButton_Clear"},
    {1200, true, Qt::Key_1, nullptr},
    {500,  true, Qt::Key_2, nullptr},
    {700,  true, Qt::Key_Q, nullptr},
    {2500, false, 0, "pushButton_Clear"},
    {2000, false, 0, nullptr},   // 收尾停顿
};

/** 演示节奏系数：整体放慢，便于观众跟随 */
constexpr double kSpeedFactor = 1.5;

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle(QStringLiteral("计算器 - Qt"));
    window.show();

    QTimer *timer = new QTimer(&window);
    timer->setSingleShot(true);
    int index = 0;

    QObject::connect(timer, &QTimer::timeout, [&]() {
        if (index >= static_cast<int>(kScript.size())) {
            QApplication::quit();
            return;
        }
        const Step &step = kScript[index];
        ++index;

        if (step.isKey) {
            QKeyEvent press(QEvent::KeyPress, static_cast<Qt::Key>(step.key), Qt::NoModifier);
            QApplication::sendEvent(&window, &press);
        } else if (step.buttonName != nullptr) {
            QPushButton *button = window.findChild<QPushButton *>(QString::fromLatin1(step.buttonName));
            if (button != nullptr) {
                button->click();
            }
        }

        if (index < static_cast<int>(kScript.size())) {
            timer->start(qRound(kScript[index].delayMs * kSpeedFactor));
        } else {
            QTimer::singleShot(500, &window, []() { QApplication::quit(); });
        }
    });

    timer->start(qRound(kScript.front().delayMs * kSpeedFactor));
    return app.exec();
}
