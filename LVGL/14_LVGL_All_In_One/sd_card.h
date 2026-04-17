#ifndef __SD_CARD_H
#define __SD_CARD_H

#include "Arduino.h"
#include "FS.h"
#include <LittleFS.h>
#include "SD_MMC.h"
#include "lvgl.h"
#include "stdlib.h"
#include "string.h"

/***************************************************************************************
 * SD 卡引脚配置 (SD Card Pin Configuration)
 ***************************************************************************************/
#define SD_MMC_CMD  38   // 命令引脚 (Command Pin) - 请勿修改
#define SD_MMC_CLK  39   // 时钟引脚 (Clock Pin) - 请勿修改
#define SD_MMC_D0   40   // 数据引脚 (Data Pin) - 请勿修改

/***************************************************************************************
 * 文件夹路径定义 (Folder Path Definitions)
 ***************************************************************************************/
#define MUSIC_FOLDER    "/music"     // 音乐文件夹
#define PICTURE_FOLDER  "/picture"   // 图片文件夹
#define LOGO_FOLDER     "/logo"      // Logo 文件夹

/***************************************************************************************
 * 文件系统常量 (File System Constants)
 ***************************************************************************************/
#define FILE_NAME_LENGTH  255        // 文件名最大长度

/***************************************************************************************
 * 链表节点结构体 (Linked List Node Structure)
 ***************************************************************************************/
typedef struct Link{
  int index_number;                  // 节点索引号
  char file_name[FILE_NAME_LENGTH];  // 文件名
  struct Link *file_prev;            // 指向前一个节点
  struct Link *file_next;            // 指向下一个节点
}list_link;

/***************************************************************************************
 * 全局链表指针 (Global List Pointers)
 ***************************************************************************************/
extern list_link *list_music;     // 音乐文件链表
extern list_link *list_picture;   // 图片文件链表
extern list_link *list_logo;      // Logo 文件链表

/***************************************************************************************
 * SD 卡初始化函数 (SD Card Initialization)
 ***************************************************************************************/

/**
 * @brief 初始化 SD_MMC 模块
 * @return 1 初始化成功, 0 初始化失败
 */
int sdcard_init(void);

/***************************************************************************************
 * 文件夹扫描函数 (Folder Scanning Functions)
 ***************************************************************************************/

/**
 * @brief 扫描音乐文件夹并生成链表
 * @note 自动创建文件夹(如果不存在),扫描 .mp3 文件
 */
void setup_list_head_music(void);

/**
 * @brief 扫描图片文件夹并生成链表
 * @note 自动创建文件夹(如果不存在),扫描 .bmp 文件
 */
void setup_list_head_picture(void);

/**
 * @brief 扫描 Logo 文件夹并生成链表
 * @note 自动创建文件夹(如果不存在),扫描 .jpg 文件
 */
void setup_list_head_logo(void);

/***************************************************************************************
 * 文件操作函数 (File Operations)
 ***************************************************************************************/

/**
 * @brief 写入数据到文件
 * @param path 文件路径
 * @param buf  数据缓冲区
 * @param size 数据大小(字节)
 */
void write_file(char *path, uint8_t *buf, long size);

/**
 * @brief 将 RGB565 数据保存为 BMP 图片
 * @param path   文件路径
 * @param buf    RGB565 图像数据
 * @param size   数据大小(字节)
 * @param height 图像高度
 * @param width  图像宽度
 */
void write_rgb565_to_bmp(char *path, uint8_t *buf, long size, long height, long width);

/**
 * @brief 创建文件夹(如果不存在)
 * @param path 文件夹路径
 */
void create_folder(char *path);

/**
 * @brief 删除文件
 * @param path 文件路径
 */
void delete_file(char *path);

/***************************************************************************************
 * 链表操作函数 (Linked List Operations)
 ***************************************************************************************/

/**
 * @brief 读取指定文件夹并生成文件链表
 * @param dirname   文件夹路径
 * @param extension 文件扩展名(如 "mp3", "bmp")
 * @return 链表头节点指针
 */
list_link *sdcard_read_folder(char *dirname, char *extension);

/**
 * @brief 创建链表节点
 * @param index 节点索引
 * @param name  文件名
 * @return 新节点指针,失败返回 NULL
 */
list_link *list_create_node(int index, char *name);

/**
 * @brief 在链表尾部插入节点
 * @param phead 链表头节点
 * @param name  文件名
 */
void list_insert_tail(list_link *phead, char *name);

/**
 * @brief 打印链表内容(正序)
 * @param phead 链表头节点
 */
void list_printf(list_link *phead);

/**
 * @brief 打印链表内容(倒序)
 * @param phead 链表头节点
 */
void list_printf_back(list_link *phead);

/**
 * @brief 销毁链表并释放内存
 * @param phead 链表头节点
 * @return NULL
 */
list_link *list_destory(list_link *phead);

/**
 * @brief 根据索引查找节点
 * @param phead 链表头节点
 * @param index 节点索引(从 1 开始)
 * @return 文件名指针,未找到返回 NULL
 */
char* list_find_node(list_link *phead, int index);

/**
 * @brief 计算链表节点数量
 * @param phead 链表头节点
 * @return 节点数量(不包括头节点)
 */
int list_count_number(list_link *phead);

#endif