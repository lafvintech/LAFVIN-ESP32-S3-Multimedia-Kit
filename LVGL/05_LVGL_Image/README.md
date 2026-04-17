# 05.2_IMG - 嵌入式图片显示示例

这是一个简化版本的 LVGL 图片显示示例，**只支持嵌入式 C 数组图片**。

## 功能

- ✅ 显示嵌入在代码中的 StarryNight 图片（240x320）
- ✅ 简洁的代码结构，易于理解
- ✅ 无需 SD 卡，图片数据编译到固件中

## 文件说明

```
05.2_IMG/
├── 05.2_IMG.ino      # 主程序
├── display.h         # 显示驱动头文件
├── display.cpp       # 显示驱动实现
├── image_data.h      # 图片数据声明
├── StarryNight.c     # StarryNight 图片数据（240x320）
└── README.md         # 本文件
```

## 使用方法

1. 直接编译上传到 ESP32-S3
2. 图片会自动显示在屏幕中央

## 如何更换图片

### 方法 1：使用 LVGL 在线转换工具

1. 访问：https://lvgl.io/tools/imageconverter
2. 上传你的图片（建议 240x320 或更小）
3. 设置参数：
   - **Color format**: CF_TRUE_COLOR
   - **Output format**: C array
4. 点击 "Convert" 下载 `.c` 文件
5. 替换 `StarryNight.c` 的内容
6. 更新 `image_data.h` 中的图片名称（如果需要）

### 方法 2：使用命令行工具

如果你安装了 LVGL 的命令行工具：

```bash
lv_img_conv your_image.png -f true_color -c array > your_image.c
```

## 优点

- ✅ 不需要 SD 卡
- ✅ 启动快速
- ✅ 代码简洁
- ✅ 稳定可靠

## 缺点

- ❌ 占用 Flash 空间（240x320 图片约 150KB）
- ❌ 更换图片需要重新编译
- ❌ 不适合大量图片或频繁更换图片的场景

## 硬件要求

- ESP32-S3 开发板
- 240x320 TFT 显示屏
- FT6336U 触摸屏（可选）

## 相关项目

- **05_LVGL_Image**: 支持 SD 卡 PNG 图片的完整版本（目前有兼容性问题）
- **06_LVGL_IMG_BTN**: 图片按钮示例

## 故障排除

### 编译错误

- 确保所有文件都在同一文件夹
- 检查 `StarryNight.c` 文件格式是否正确

### 图片不显示

- 检查串口输出是否有错误信息
- 确认图片尺寸不超过屏幕大小
- 验证 `image_data.h` 中的声明与 `.c` 文件中的定义一致

## 技术细节

- 使用 LVGL 8.x
- 图片格式：RGB565（16-bit 真彩色）
- 显示驱动：TFT_eSPI
- 触摸驱动：FT6336U
