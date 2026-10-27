#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractButton>
#include <QKeyEvent>
#include <QPushButton>
#include <QTimer>

#include <cmath>
#include <limits>

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
    } else if (name == QStringLiteral("pushButton_Add")) {
        applyOperator(QStringLiteral("+"));
    } else if (name == QStringLiteral("pushButton_Subtract")) {
        applyOperator(QStringLiteral("−"));
    } else if (name == QStringLiteral("pushButton_Multiply")) {
        applyOperator(QStringLiteral("×"));
    } else if (name == QStringLiteral("pushButton_Divide")) {
        applyOperator(QStringLiteral("÷"));
    } else if (name == QStringLiteral("pushButton_Equal")) {
        calculateEqual();
    }
}

/**
 * @brief 键盘事件处理
 *
 * 思路：键盘按键不单独实现一套逻辑，而是翻译成与鼠标点击完全相同的调用
 * （appendDigit / applyOperator / calculateEqual ...），唯一区别是额外让
 * 对应按钮闪一下作为视觉反馈。这也是鼠标与键盘行为能保持一致的原因。
 */
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    // ---- 数字键（主键盘 0~9，以及带 KeypadModifier 的小键盘）----
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        const int number = key - Qt::Key_0;
        appendDigit(QString::number(number));
        flashButton(findChild<QPushButton *>(QStringLiteral("pushButton_%1").arg(number)));
        return;
    }

    switch (key) {
    case Qt::Key_Plus:
        applyOperator(QStringLiteral("+"));
        flashButton(ui->pushButton_Add);
        break;
    case Qt::Key_Minus:
        applyOperator(QStringLiteral("−"));
        flashButton(ui->pushButton_Subtract);
        break;
    case Qt::Key_Asterisk:
        applyOperator(QStringLiteral("×"));
        flashButton(ui->pushButton_Multiply);
        break;
    case Qt::Key_Slash:
        applyOperator(QStringLiteral("÷"));
        flashButton(ui->pushButton_Divide);
        break;
    case Qt::Key_Equal:
    case Qt::Key_Enter:      // 小键盘回车
    case Qt::Key_Return:     // 主键盘回车
        calculateEqual();
        flashButton(ui->pushButton_Equal);
        break;
    case Qt::Key_Period:
    case Qt::Key_Comma:      // 小键盘的逗号（部分键盘布局）
        appendPoint();
        flashButton(ui->pushButton_Point);
        break;
    case Qt::Key_Backspace:
        backspace();
        flashButton(ui->pushButton_Backspace);
        break;
    case Qt::Key_Percent:
        applyPercent();
        flashButton(ui->pushButton_Percent);
        break;
    case Qt::Key_Escape:
        clearAll();
        flashButton(ui->pushButton_Clear);
        break;
    case Qt::Key_Delete:
        clearEntry();
        break;
    default:
        QMainWindow::keyPressEvent(event);   // 其他按键（Tab、Alt 等）交给基类
        return;
    }
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
    m_errorMessage.clear();
    ui->labelExpression->clear();

    updateDisplay();
}

/**
 * @brief 清除当前正在输入的操作数，不影响已保存的左操作数与运算符
 */
void MainWindow::clearEntry()
{
    if (m_hasError) {
        clearAll();
        return;
    }
    m_currentInput = QStringLiteral("0");
    updateDisplay();
}

/**
 * @brief 按下四则运算符
 * @param op 运算符文本（+ − × ÷）
 *
 * 分三种情况：
 *  1) 首次按下运算符  → 仅保存左操作数；
 *  2) 连按多个运算符  → 只替换待执行的运算符，防止 "2 + × 3" 出错；
 *  3) 已有左操作数且右操作数已输入 → 先完成上一次运算（支持 2+3+ 连续运算），
 *     再把结果作为新的左操作数。
 */
void MainWindow::applyOperator(const QString &op)
{
    if (m_hasError) {
        return;
    }

    if (m_pendingOperator.isEmpty()) {
        // 情况 1：第一次按运算符，把当前显示屏内容记为第一个操作数
        m_accumulatedValue = m_currentInput.toDouble();
    } else if (m_waitingForNewOperand) {
        // 情况 2：刚按过运算符还没输入新数字，即连续按运算符，只更新运算符
        m_pendingOperator = op;
        updateExpressionLabel();
        return;
    } else {
        // 情况 3：连续运算，先把已经能算的部分算出来
        const double result = executeOperation(m_accumulatedValue, m_pendingOperator,
                                               m_currentInput.toDouble());
        if (std::isnan(result)) {
            enterErrorState(tr("除数不能为零"));
            return;
        }
        m_accumulatedValue = result;
        m_currentInput = formatResult(result);
    }

    m_pendingOperator = op;
    m_waitingForNewOperand = true;   // 下一个输入的数字将作为第二个操作数
    updateExpressionLabel();
    updateDisplay();
}

/**
 * @brief 按下等号：执行一次待执行的运算
 */
void MainWindow::calculateEqual()
{
    if (m_hasError || m_pendingOperator.isEmpty()) {
        return;                       // 没有待执行的运算符时直接忽略
    }

    const double left  = m_accumulatedValue;
    const double right = m_currentInput.toDouble();
    const double result = executeOperation(left, m_pendingOperator, right);

    ui->labelExpression->setText(QStringLiteral("%1 %2 %3 =")
                                     .arg(formatResult(left),
                                          m_pendingOperator,
                                          formatResult(right)));

    if (std::isnan(result)) {
        enterErrorState(tr("除数不能为零"));
        return;
    }

    m_currentInput = formatResult(result);
    m_accumulatedValue = result;      // 结果可作为下一轮运算的左操作数
    m_pendingOperator.clear();
    m_waitingForNewOperand = true;    // 再输入数字则开启新的一次计算
    updateDisplay();
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
    ui->lineEditDisplay->setText(m_hasError ? m_errorMessage : m_currentInput);
}

/**
 * @brief 刷新上方表达式标签，显示「已保存的操作数 待执行运算符」
 */
void MainWindow::updateExpressionLabel()
{
    if (m_pendingOperator.isEmpty()) {
        ui->labelExpression->clear();
    } else {
        ui->labelExpression->setText(QStringLiteral("%1 %2")
                                         .arg(formatResult(m_accumulatedValue),
                                              m_pendingOperator));
    }
}

/**
 * @brief 进入错误状态并显示提示，此时除「清除」外的操作均被忽略
 */
void MainWindow::enterErrorState(const QString &message)
{
    m_hasError = true;
    m_errorMessage = message;
    m_pendingOperator.clear();
    m_waitingForNewOperand = true;
    m_accumulatedValue = 0.0;
    updateDisplay();
}

/**
 * @brief 键盘触发时让对应按钮短暂处于按下状态，提供视觉反馈
 */
void MainWindow::flashButton(QAbstractButton *button)
{
    if (button == nullptr) {
        return;
    }
    button->setDown(true);
    QTimer::singleShot(120, button, [button]() {
        button->setDown(false);
    });
}

void MainWindow::applyStyleSheet()
{
    // TODO: 界面美化阶段实现
}

/**
 * @brief 执行单次二元运算
 * @return 运算结果；除数为 0 时返回 NaN，由调用方转换为错误提示
 */
double MainWindow::executeOperation(double left, const QString &op, double right) const
{
    if (op == QStringLiteral("+")) {
        return left + right;
    }
    if (op == QStringLiteral("−") || op == QStringLiteral("-")) {
        return left - right;
    }
    if (op == QStringLiteral("×") || op == QStringLiteral("*")) {
        return left * right;
    }
    if (op == QStringLiteral("÷") || op == QStringLiteral("/")) {
        if (qFuzzyIsNull(right)) {
            return std::numeric_limits<double>::quiet_NaN();   // 除零 → 交给上层处理
        }
        return left / right;
    }
    return right;
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
