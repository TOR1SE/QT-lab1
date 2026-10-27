#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAbstractButton;
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief 带键盘事件的计算器主窗口
 *
 * 设计要点：
 * 1. 鼠标点击与键盘按键最终都调用同一组核心处理函数
 *    （appendDigit / appendPoint / applyOperator ...），
 *    保证两种输入方式行为完全一致；
 * 2. 计算状态由 m_pendingOperator、m_accumulatedValue、
 *    m_waitingForNewOperand 三个成员统一管理。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    /** 键盘事件入口：把物理按键翻译成与鼠标等价的操作 */
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /** 统一的按钮点击槽：从 sender() 取回按钮文本后再分发 */
    void onButtonClicked();

private:
    // ===== 核心处理逻辑（鼠标与键盘共用）=====
    void appendDigit(const QString &digit);
    void appendPoint();
    void backspace();
    void clearAll();      // 全部清除（C / Esc）
    void clearEntry();    // 只清除当前正在输入的操作数（Delete）
    void applyOperator(const QString &op);
    void calculateEqual();
    void toggleSign();
    void applyPercent();

    // ===== 辅助函数 =====
    void setupConnections();      // 建立信号与槽的关联
    void updateDisplay();         // 刷新显示屏
    void updateExpressionLabel(); // 刷新上方的表达式标签（如 "12 +"）
    void enterErrorState(const QString &message);   // 进入错误状态（如除零）
    void flashButton(QAbstractButton *button);      // 键盘操作时让对应按钮闪一下
    void applyStyleSheet();       // 加载 QSS 样式表
    double executeOperation(double left, const QString &op, double right) const;
    QString formatResult(double value) const;   // 去掉多余的 0，控制显示精度

    Ui::MainWindow *ui;

    // ===== 计算状态 =====
    double  m_accumulatedValue = 0.0;   // 已保存的第一个操作数 / 上一次运算结果
    QString m_pendingOperator;          // 待执行的运算符（+ − × ÷）
    bool    m_waitingForNewOperand = true;  // true：下一个数字将开启新的操作数
    QString m_lastOperator;             // 上一次完成的运算符，用于连按等号时重复运算
    double  m_lastOperand = 0.0;        // 上一次完成的第二个操作数
    bool    m_hasLastOperation = false; // 是否存在可重复的上一次运算
    QString m_currentInput = QStringLiteral("0");  // 当前正在输入的操作数文本
    bool    m_hasError = false;         // 是否处于错误状态（如除零）
    QString m_errorMessage;             // 错误状态下显示屏显示的提示文字
};

#endif // MAINWINDOW_H
