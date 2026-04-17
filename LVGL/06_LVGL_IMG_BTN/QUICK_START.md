# 快速开始 - 06_LVGL_IMG_BTN

## 项目概述

这个项目使用 LVGL 图片按钮控制蜂鸣器，点击屏幕上的图标可以开关蜂鸣器。

## 文件结构

```
06_LVGL_IMG_BTN/
├── 06_LVGL_IMG_BTN.ino  ← 主程序
├── display.h            ← 显示驱动
├── display.cpp          ← 显示驱动实现
├── image_data.h         ← 图标声明
├── buzzer.c             ← 蜂鸣器图标（100x100）
└── mute.c               ← 静音图标（100x100）
```

## 硬件连接

| 组件 | 引脚 |
|------|------|
| 蜂鸣器信号 | GPIO 3 |
| 蜂鸣器 VCC | 3.3V |
| 蜂鸣器 GND | GND |

## 编译上传

```bash
arduino-cli compile -b esp32:esp32:esp32s3 -e -u -p COM90 .
```

## 使用方法

1. 上传代码到 ESP32-S3
2. 打开串口监视器（115200 波特率）
3. 点击屏幕中央的图标
4. 蜂鸣器会响/停止
5. 图标会在静音和蜂鸣器之间切换

## 功能说明

### 初始状态
- 图标：静音（mute_icon）
- 蜂鸣器：关闭
- 状态文字：红色 "Buzzer: OFF"

### 点击后
- 图标：蜂鸣器（buzzer_icon）
- 蜂鸣器：开启
- 状态文字：绿色 "Buzzer: ON"

### 再次点击
- 回到初始状态

## 代码关键点

### 1. 图片按钮创建
```cpp
img_btn = lv_imgbtn_create(lv_scr_act());
lv_imgbtn_set_src(img_btn, LV_IMGBTN_STATE_RELEASED, NULL, &mute_icon, NULL);
```

### 2. 事件处理
```cpp
static void imgbtn_event_handler(lv_event_t * e) {
    if(code == LV_EVENT_CLICKED) {
        setBuzzer(!buzzer_on);  // 切换状态
        // 切换图标...
    }
}
```

### 3. 硬件控制
```cpp
void setBuzzer(bool state) {
    digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
}
```

## 自定义图标

### 使用 LVGL 图片转换工具

1. 访问：https://lvgl.io/tools/imageconverter
2. 上传你的图标（PNG，建议 100x100）
3. 设置：
   - Color format: **CF_TRUE_COLOR**
   - Output format: **C array**
4. 下载 `.c` 文件
5. 替换 `buzzer.c` 或 `mute.c`

### 图标要求

- 格式：PNG（透明背景最佳）
- 尺寸：100x100 像素（可调整）
- 颜色：RGB 真彩色

## 扩展功能

### 添加音量控制
```cpp
// 使用 PWM 控制蜂鸣器音量
ledcAttach(BUZZER_PIN, 2000, 8);  // 2kHz, 8-bit
ledcWrite(BUZZER_PIN, volume);    // 0-255
```

### 添加不同音调
```cpp
// 改变频率
ledcWriteTone(BUZZER_PIN, 1000);  // 1kHz
ledcWriteTone(BUZZER_PIN, 2000);  // 2kHz
```

### 播放旋律
```cpp
int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
for(int note : melody) {
    ledcWriteTone(BUZZER_PIN, note);
    delay(200);
}
```

## 故障排除

### 蜂鸣器不响
- ✅ 检查引脚连接（GPIO 3）
- ✅ 检查蜂鸣器类型（有源/无源）
- ✅ 测试引脚电压（应该是 3.3V）
- ✅ 尝试直接连接 3.3V 测试蜂鸣器

### 图标不显示
- ✅ 检查 `buzzer.c` 和 `mute.c` 格式
- ✅ 确认 `image_data.h` 声明正确
- ✅ 查看串口输出错误信息

### 触摸无响应
- ✅ 检查触摸屏校准
- ✅ 确认 `display.cpp` 中触摸驱动正常
- ✅ 测试其他 LVGL 示例（如 02_LVGL_Lable）

## 参考项目

- **03_LVGL_GPIO**: RGB LED 控制（类似的硬件控制）
- **02_LVGL_Lable**: 按钮事件处理
- **05.2_IMG**: 嵌入式图片显示

## 技术支持

如有问题，请检查：
1. 串口输出的错误信息
2. 硬件连接是否正确
3. 图标文件格式是否正确
4. LVGL 库版本是否兼容（推荐 8.x）
