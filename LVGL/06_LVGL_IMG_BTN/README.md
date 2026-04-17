# 06_LVGL_IMG_BTN - 图片按钮控制蜂鸣器

这个示例展示如何使用 LVGL 的图片按钮（Image Button）控制硬件（蜂鸣器）。

## 功能

- ✅ 使用嵌入式 C 数组图片作为按钮
- ✅ 点击按钮切换蜂鸣器开关
- ✅ 按钮图标随状态变化（静音/蜂鸣器）
- ✅ 实时显示蜂鸣器状态

## 文件说明

```
06_LVGL_IMG_BTN/
├── 06_LVGL_IMG_BTN.ino  # 主程序
├── display.h            # 显示驱动头文件
├── display.cpp          # 显示驱动实现
├── image_data.h         # 图片数据声明
├── buzzer.c             # 蜂鸣器图标数据（100x100）
├── mute.c               # 静音图标数据（100x100）
└── README.md            # 本文件
```

## 硬件连接

### 蜂鸣器
- 信号引脚：GPIO 3
- VCC：3.3V
- GND：GND

### 显示屏和触摸屏
根据 `display.h` 中的配置

## 使用方法

1. 编译上传到 ESP32-S3
2. 点击屏幕中央的图标
3. 蜂鸣器会响/停止
4. 图标会在静音和蜂鸣器之间切换

## 如何更换图标

### 方法 1：使用 LVGL 在线转换工具

1. 访问：https://lvgl.io/tools/imageconverter
2. 上传你的图标（建议 100x100 像素，PNG 格式）
3. 设置参数：
   - **Color format**: CF_TRUE_COLOR
   - **Output format**: C array
4. 点击 "Convert" 下载 `.c` 文件
5. 替换 `buzzer.c` 或 `mute.c` 的内容
6. 更新 `image_data.h` 中的图标尺寸（如果需要）

### 图标建议

- **蜂鸣器图标**：喇叭、音符、声波等
- **静音图标**：带斜线的喇叭、禁止符号等
- **尺寸**：100x100 像素（可以更大或更小）
- **格式**：PNG 透明背景效果最好

## 代码说明

### 主要功能

1. **图片按钮创建**
```cpp
img_btn = lv_imgbtn_create(lv_scr_act());
lv_imgbtn_set_src(img_btn, LV_IMGBTN_STATE_RELEASED, NULL, &mute_icon, NULL);
```

2. **状态切换**
```cpp
if(buzzer_on) {
    lv_imgbtn_set_src(img_btn, LV_IMGBTN_STATE_RELEASED, NULL, &buzzer_icon, NULL);
} else {
    lv_imgbtn_set_src(img_btn, LV_IMGBTN_STATE_RELEASED, NULL, &mute_icon, NULL);
}
```

3. **蜂鸣器控制**
```cpp
digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
```

## 扩展功能建议

可以添加的功能：
- 音量调节（PWM 控制蜂鸣器）
- 不同音调切换
- 播放旋律
- 长按/短按不同功能
- 添加更多图标按钮控制其他硬件

## 故障排除

### 蜂鸣器不响

- 检查引脚连接（GPIO 3）
- 检查蜂鸣器类型（有源/无源）
- 检查电源供电
- 使用万用表测试引脚电压

### 图标不显示

- 检查 `.c` 文件格式是否正确
- 确认图标尺寸不超过屏幕
- 查看串口输出是否有错误

### 触摸无响应

- 检查触摸屏校准
- 确认触摸驱动正常工作
- 查看 `display.cpp` 中的触摸回调

## 技术细节

- LVGL 版本：8.x
- 图片格式：RGB565（16-bit 真彩色）
- 按钮类型：lv_imgbtn（图片按钮）
- 蜂鸣器控制：数字 IO（HIGH/LOW）

## 相关项目

- **03_LVGL_GPIO**: GPIO 控制示例（RGB LED）
- **05.2_IMG**: 嵌入式图片显示示例
- **02_LVGL_Lable**: 按钮和事件处理示例
