# 蜂鸣器类型选择指南

## 问题现象

如果你发现蜂鸣器**只在切换时响一声**，而不是持续响，说明你的蜂鸣器类型设置不正确。

## 蜂鸣器类型

### 1. 有源蜂鸣器（Active Buzzer）

**特点：**
- 内置振荡电路
- 直接给电就会响
- 通常有极性（+/-）
- 价格稍贵

**控制方式：**
```cpp
#define ACTIVE_BUZZER true  // 设置为 true

// 代码会使用：
digitalWrite(BUZZER_PIN, HIGH);  // 响
digitalWrite(BUZZER_PIN, LOW);   // 停
```

**适用场景：**
- 简单的提示音
- 报警声
- 按键反馈

---

### 2. 无源蜂鸣器（Passive Buzzer）

**特点：**
- 没有内置振荡电路
- 需要外部提供频率信号（PWM）
- 可以发出不同音调
- 价格便宜

**控制方式：**
```cpp
#define ACTIVE_BUZZER false  // 设置为 false

// 代码会使用：
ledcWriteTone(BUZZER_PIN, 2000);  // 输出 2kHz 频率，响
ledcWriteTone(BUZZER_PIN, 0);     // 停止输出，停
```

**适用场景：**
- 播放旋律
- 不同音调提示
- 音乐播放

---

## 如何判断你的蜂鸣器类型？

### 方法 1：外观判断
- **有源**：通常有贴纸或标签，标注 "Active" 或有极性标记
- **无源**：通常是裸露的黑色圆盘，没有标签

### 方法 2：测试判断
1. 直接连接 3.3V 和 GND
2. **有源**：会立即响
3. **无源**：不会响（或只有微弱的咔哒声）

### 方法 3：万用表测试
- **有源**：内部有电路，电阻较大（几百欧姆到几千欧姆）
- **无源**：只是线圈，电阻很小（几欧姆到几十欧姆）

---

## 代码配置

### 当前设置（在代码顶部）

```cpp
// 蜂鸣器类型选择
#define ACTIVE_BUZZER false  // ← 修改这里

// 如果是有源蜂鸣器，改为：
#define ACTIVE_BUZZER true

// 如果是无源蜂鸣器，改为：
#define ACTIVE_BUZZER false
```

### 无源蜂鸣器参数调整

```cpp
#define BUZZER_FREQ 2000    // 频率（Hz）
// 常用频率：
// - 1000 Hz：低音
// - 2000 Hz：中音（默认）
// - 4000 Hz：高音
```

---

## 常见问题

### Q1: 只响一声就停了
**原因：** 你的蜂鸣器是**无源**的，但代码设置为 `ACTIVE_BUZZER true`

**解决：** 改为 `#define ACTIVE_BUZZER false`

---

### Q2: 完全不响
**可能原因：**
1. 引脚连接错误（检查 GPIO 45）
2. 蜂鸣器损坏
3. 电源不足
4. 蜂鸣器类型设置错误

**解决步骤：**
1. 检查硬件连接
2. 尝试切换 `ACTIVE_BUZZER` 设置
3. 测试蜂鸣器是否正常（直接连 3.3V）

---

### Q3: 声音太小
**可能原因：**
1. 无源蜂鸣器频率不对
2. 电源不足

**解决：**
```cpp
// 尝试不同频率
#define BUZZER_FREQ 1000  // 或 2000, 3000, 4000
```

---

### Q4: 想播放不同音调
**解决：** 使用无源蜂鸣器，修改代码：

```cpp
// 在 setBuzzer 函数中
void setBuzzer(bool state, int frequency = BUZZER_FREQ) {
    if(state) {
        ledcWriteTone(BUZZER_PIN, frequency);
    } else {
        ledcWriteTone(BUZZER_PIN, 0);
    }
}

// 调用时
setBuzzer(true, 1000);  // 1kHz
setBuzzer(true, 2000);  // 2kHz
setBuzzer(true, 4000);  // 4kHz
```

---

## 推荐设置

### 简单应用（只需要响/不响）
→ 使用**有源蜂鸣器** + `ACTIVE_BUZZER true`

### 需要不同音调
→ 使用**无源蜂鸣器** + `ACTIVE_BUZZER false`

### 播放音乐
→ 使用**无源蜂鸣器** + 自定义频率数组

---

## 示例：播放简单旋律（无源蜂鸣器）

```cpp
// 音符频率（Hz）
#define NOTE_C 262
#define NOTE_D 294
#define NOTE_E 330
#define NOTE_F 349
#define NOTE_G 392
#define NOTE_A 440
#define NOTE_B 494

// 播放 Do Re Mi
void playMelody() {
    int melody[] = {NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G, NOTE_A, NOTE_B};
    for(int i = 0; i < 7; i++) {
        ledcWriteTone(BUZZER_PIN, melody[i]);
        delay(300);
    }
    ledcWriteTone(BUZZER_PIN, 0);  // 停止
}
```

---

## 总结

| 特性 | 有源蜂鸣器 | 无源蜂鸣器 |
|------|-----------|-----------|
| 控制方式 | 简单（HIGH/LOW） | 复杂（PWM） |
| 音调 | 固定 | 可变 |
| 价格 | 较贵 | 便宜 |
| 代码设置 | `ACTIVE_BUZZER true` | `ACTIVE_BUZZER false` |
| 适用场景 | 简单提示音 | 音乐播放 |

**当前代码默认设置：`ACTIVE_BUZZER false`（无源蜂鸣器）**

如果你的蜂鸣器只响一声，请改为 `true`！
