#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QPushButton>

namespace {
// 显示屏最多允许输入的有效字符数（含小数点与负号），防止数字过长溢出
constexpr int kMaxInputLength = 15;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupConnections();
    updateDisplay();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief 一次性把界面上所有 QPushButton 的 clicked 信号接到统一槽
 *
 * 这样做的好处：新增按键无需改动这里；所有输入（鼠标点击）都走同一入口
 * onButtonClicked()，键盘事件也复用同一批处理函数，保证行为一致。
 */
void MainWindow::setupConnections()
{
    const QList<QPushButton *> buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        connect(button, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    }
}

/**
 * @brief 统一的按钮点击处理槽
 *
 * 通过 sender() 拿到被点击按钮，再按其 objectName 分派到具体处理函数。
 */
void MainWindow::onButtonClicked()
{
    const QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    const QString name = button->objectName();

    // 数字按钮：pushButton_0 ~ pushButton_9
    if (name.startsWith(QStringLiteral("pushButton_"))
        && name.size() == QStringLiteral("pushButton_0").size()
        && name.back().isDigit()) {
        appendDigit(QString(name.back()));
        return;
    }

    // 功能按钮
    if (name == QStringLiteral("pushButton_Point")) {
        appendPoint();
    } else if (name == QStringLiteral("pushButton_Backspace")) {
        backspace();
    } else if (name == QStringLiteral("pushButton_Clear")) {
        clearAll();
    } else if (name == QStringLiteral("pushButton_Sign")) {
        toggleSign();
    } else if (name == QStringLiteral("pushButton_Percent")) {
        applyPercent();
    }
    // 四则运算符与等号将在下一步实现
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // TODO: 最后一步实现键盘事件
    QMainWindow::keyPressEvent(event);
}

/**
 * @brief 追加一位数字
 * @param digit 单个数字字符 '0'~'9'
 */
void MainWindow::appendDigit(const QString &digit)
{
    if (m_hasError) {
        return;                       // 出错后需按 C 才能继续，避免脏数据参与运算
    }

    if (m_waitingForNewOperand) {
        // 上一个操作数已确认，本次输入开启一个全新的操作数
        m_currentInput = digit;
        m_waitingForNewOperand = false;
    } else if (m_currentInput == QStringLiteral("0")) {
        m_currentInput = digit;       // 避免 "000123" 这类无效前导零
    } else if (m_currentInput == QStringLiteral("-0")) {
        m_currentInput = QLatin1Char('-') + digit;
    } else if (m_currentInput.length() < kMaxInputLength) {
        m_currentInput += digit;
    }

    updateDisplay();
}

/**
 * @brief 输入小数点（同一个操作数内只允许出现一次）
 */
void MainWindow::appendPoint()
{
    if (m_hasError) {
        return;
    }

    if (m_waitingForNewOperand) {
        // 直接按小数点时自动补 0，得到 "0."
        m_currentInput = QStringLiteral("0");
        m_waitingForNewOperand = false;
    }

    // 关键点：contains('.') 判断，重复按小数点时直接忽略，保证一个操作数只有一个小数点
    if (!m_currentInput.contains(QLatin1Char('.'))) {
        m_currentInput += QLatin1Char('.');
    }

    updateDisplay();
}

/**
 * @brief 退格：删除当前操作数的最后一位
 */
void MainWindow::backspace()
{
    if (m_hasError) {
        return;
    }
    if (m_waitingForNewOperand) {
        return;                       // 刚进行完运算，显示屏内容为历史结果，不参与退格
    }

    m_currentInput.chop(1);
    if (m_currentInput.isEmpty() || m_currentInput == QStringLiteral("-")) {
        m_currentInput = QStringLiteral("0");   // 删空后回落到 0，而不是显示空白
    }

    updateDisplay();
}

/**
 * @brief 清除：把显示屏与所有计算状态恢复到初始值
 */
void MainWindow::clearAll()
{
    m_currentInput = QStringLiteral("0");
    m_accumulatedValue = 0.0;
    m_pendingOperator.clear();
    m_waitingForNewOperand = true;
    m_hasError = false;
    ui->labelExpression->clear();

    updateDisplay();
}

void MainWindow::applyOperator(const QString &op)
{
    Q_UNUSED(op);
    // TODO: 下一步实现
}

void MainWindow::calculateEqual()
{
    // TODO: 下一步实现
}

/**
 * @brief 正负号切换
 */
void MainWindow::toggleSign()
{
    if (m_hasError) {
        return;
    }

    if (m_currentInput.startsWith(QLatin1Char('-'))) {
        m_currentInput.remove(0, 1);
    } else if (m_currentInput != QStringLiteral("0")) {
        m_currentInput.prepend(QLatin1Char('-'));
    }

    updateDisplay();
}

/**
 * @brief 百分号：当前数值除以 100
 */
void MainWindow::applyPercent()
{
    if (m_hasError) {
        return;
    }

    const double value = m_currentInput.toDouble() / 100.0;
    m_currentInput = formatResult(value);
    updateDisplay();
}

/**
 * @brief 刷新主显示屏内容
 */
void MainWindow::updateDisplay()
{
    ui->lineEditDisplay->setText(m_currentInput);
}

void MainWindow::applyStyleSheet()
{
    // TODO: 界面美化阶段实现
}

double MainWindow::executeOperation(double left, const QString &op, double right) const
{
    Q_UNUSED(left);
    Q_UNUSED(op);
    Q_UNUSED(right);
    return 0.0;
}

/**
 * @brief 结果格式化：去掉多余的 0，并限制有效位数
 */
QString MainWindow::formatResult(double value) const
{
    if (qFuzzyIsNull(value)) {
        return QStringLiteral("0");     // 避免 "-0" 这种奇怪的显示
    }

    QString text = QString::number(value, 'g', 12);
    if (text == QStringLiteral("-0")) {
        text = QStringLiteral("0");
    }
    return text;
}
