# 带键盘事件的计算器（Qt 实验一）

东莞理工学院《Qt 应用程序开发》实验一：基于 **Qt Widgets + CMake** 实现的带键盘事件计算器，
支持鼠标点击与键盘两种输入方式，两者复用同一套处理逻辑。

## 功能特性

- 四则运算（`+ − × ÷`），支持小数、负数与百分号
- 连续运算（`2 + 3 + 4 =`）、连按等号重复上一次运算（`2 + 3 = =` → `8`）
- 退格 `⌫`、全部清除 `C`、正负号 `±`
- 异常输入处理：
  - 同一操作数重复输入小数点会被忽略（`1.5` 中再按 `.` 无效）
  - 连续按多个运算符只保留最后一个（`5 + × 2 =` → `10`）
  - 除零提示「除数不能为零」，按 `C` 后可恢复
  - 计算完成后继续输入数字会开启新的一次计算
  - 输入位数上限 15 位，自动去除前导零
- 键盘输入与鼠标点击走同一组核心函数，键盘命中时对应按钮高亮反馈

## 键盘按键映射

| 按键 | 功能 |
| --- | --- |
| `0`~`9`（主键盘 / 小键盘） | 输入数字 |
| `+` `-` `*` `/` | 加 减 乘 除 |
| `Enter` / `Return` / `=` | 等号 |
| `.`（或小键盘 `.`） | 小数点 |
| `Backspace` | 退格 |
| `Esc` | 全部清除（C） |
| `Delete` | 清除当前输入（CE） |
| `%` | 百分号 |

## 构建与运行

### Qt Creator

直接打开 `Calculator/CMakeLists.txt` 即可（工程已处理中文构建路径下 moc 失败的问题）。

### 命令行（MinGW）

```bash
cmake -G Ninja -B build -DCMAKE_PREFIX_PATH="<Qt安装目录>/mingw_64"
cmake --build build
./build/Calculator.exe
```

### 运行自测

```bash
cmake --build build --target CalculatorSelfCheck
./build/CalculatorSelfCheck.exe
```

自测以程序方式模拟点击与按键，覆盖 33 个场景
（基本运算 / 连续运算 / 除零 / 重复小数点 / 退格 / 清除后重算 / 鼠标键盘混合输入等），
退出码为 0 表示全部通过。

## 目录结构

```
├── Calculator/
│   ├── CMakeLists.txt        # 构建脚本（含中文路径兼容处理）
│   ├── main.cpp
│   ├── mainwindow.h/.cpp     # 计算器主窗口与计算逻辑
│   ├── mainwindow.ui         # Qt Designer 界面文件
│   └── tests/SelfCheck/      # 自测程序入口
└── docs/images/              # 运行截图
```

## 主要设计说明

- **统一输入入口**：所有按钮 `clicked` 信号接到 `onButtonClicked()`，按 `objectName`
  分发到 `appendDigit() / applyOperator() / calculateEqual()` 等核心函数；
  `keyPressEvent()` 把按键翻译成对同一批函数的调用，保证鼠标与键盘行为一致。
- **状态管理**：`m_accumulatedValue`（第一个操作数）、`m_pendingOperator`（待执行运算符）、
  `m_waitingForNewOperand`（是否等待新操作数）三个成员描述整个计算状态机；
  `m_lastOperator / m_lastOperand` 支持连按等号重复上一次运算。
- **错误处理**：`executeOperation()` 除零时返回 NaN，上层统一转入错误态，
  错误态下除 `C` 外的输入都被忽略。
