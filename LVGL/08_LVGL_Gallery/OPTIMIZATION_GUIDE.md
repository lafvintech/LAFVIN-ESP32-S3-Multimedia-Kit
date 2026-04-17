# 图片库性能优化指南

## 已完成的优化

### 1. ✅ 删除 Home 键
- 从结构体中移除 `picture_home` 成员
- 删除 `picture_imgbtn_home_event_handler` 函数
- 移除 `lv_img_home_init()` 调用
- 左右按钮位置保持不变

### 2. ✅ 启用图片缓存
```cpp
lv_img_cache_set_size(8);  // 缓存最多 8 张图片
```
这样切换到之前看过的图片时会更快。

### 3. ✅ 添加加载时间监控
现在会在串口输出每张图片的加载时间,方便诊断性能问题。

---

## 进一步优化建议

### 方案 1: 提高 SD 卡速度 ⭐⭐⭐⭐⭐
**最有效的方法**

当前代码使用 1-bit SD_MMC 模式,可以改为 4-bit 模式:

```cpp
// 在 sd_card.cpp 的 sdcard_init() 中修改:
// 当前: SD_MMC.begin("/sdcard", true, true, ...)  // 1-bit 模式
// 改为: SD_MMC.begin("/sdcard", false, true, ...) // 4-bit 模式
```

**注意**: 需要连接额外的数据线 (D1, D2, D3)

**预期提升**: 3-4倍速度提升

---

### 方案 2: 优化图片格式 ⭐⭐⭐⭐
**推荐使用**

#### 选项 A: 使用 RAW RGB565 格式
BMP 文件需要解码,RAW 格式可以直接显示:

1. 将 BMP 转换为 RAW RGB565 格式
2. 修改文件扩展名为 `.bin`
3. 修改代码扫描 `.bin` 文件

**预期提升**: 2-3倍速度提升

#### 选项 B: 压缩图片尺寸
如果不需要 240x240 全屏,可以使用更小的尺寸:
- 200x200: 节省 30% 文件大小
- 180x180: 节省 44% 文件大小

---

### 方案 3: 使用 DMA 传输 ⭐⭐⭐
修改 `display.cpp` 中的 `my_disp_flush` 函数,使用 DMA 传输:

```cpp
// 在 TFT_eSPI 配置中启用 DMA
tft.initDMA();
```

**预期提升**: 20-30% 速度提升

---

### 方案 4: 预加载下一张图片 ⭐⭐⭐
在后台预加载下一张图片到缓冲区:

```cpp
// 创建 FreeRTOS 任务在后台加载
xTaskCreate(preload_next_image_task, "Preload", 4096, NULL, 1, NULL);
```

**预期提升**: 用户感知的加载时间接近 0

---

### 方案 5: 减少 LVGL 缓冲区刷新 ⭐⭐
在 `display.cpp` 中增加缓冲区大小:

```cpp
// 当前: #define LVGL_BUF_SIZE (screenWidth * 10)
// 改为: #define LVGL_BUF_SIZE (screenWidth * 20)
```

**注意**: 会占用更多内存

**预期提升**: 10-15% 速度提升

---

## 性能测试

运行程序后,查看串口输出:
```
Loading image: 1.bmp
✓ Image loaded in 850 ms
```

### 性能参考值:
- **优秀**: < 200ms
- **良好**: 200-500ms
- **一般**: 500-1000ms
- **较慢**: > 1000ms

---

## 推荐优化顺序

1. **先测试当前速度** - 查看串口输出的加载时间
2. **启用 4-bit SD 模式** - 最简单且效果最好
3. **增加图片缓存** - 已完成,缓存 8 张
4. **考虑使用 RAW 格式** - 如果还不够快
5. **实现预加载** - 终极方案

---

## 硬件建议

### SD 卡选择:
- ✅ 使用 Class 10 或 UHS-I 卡
- ✅ 选择知名品牌 (SanDisk, Samsung)
- ❌ 避免使用廉价 SD 卡

### 连接建议:
- 使用短的连接线
- 确保良好的电源供应
- 检查引脚连接是否牢固

---

## 故障排查

### 如果图片加载仍然很慢:

1. **检查 SD 卡速度**
   ```cpp
   // 在 sdcard_init() 后添加:
   uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
   Serial.printf("Card size: %llu MB\n", cardSize);
   ```

2. **检查图片文件大小**
   - 240x240 BMP 应该约 115KB
   - 如果更大,说明格式有问题

3. **测试 SD 卡读取速度**
   ```cpp
   File file = SD_MMC.open("/test.bin", FILE_READ);
   unsigned long start = millis();
   while(file.available()) {
     file.read();
   }
   unsigned long elapsed = millis() - start;
   Serial.printf("Read speed: %lu KB/s\n", file.size() / elapsed);
   ```

---

## 技术支持

如有问题,请提供:
1. 串口输出的加载时间
2. SD 卡型号和容量
3. 图片文件大小
4. 是否使用 1-bit 或 4-bit 模式
