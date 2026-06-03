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
 * SD Card Pin Configuration
 ***************************************************************************************/
#define SD_MMC_CMD  38   // Command Pin - do not modify
#define SD_MMC_CLK  39   // Clock Pin - do not modify
#define SD_MMC_D0   40   // Data Pin - do not modify

/***************************************************************************************
 * Folder Path Definitions
 ***************************************************************************************/
#define MUSIC_FOLDER    "/music"     // Music folder
#define PICTURE_FOLDER  "/picture"   // Picture folder
#define LOGO_FOLDER     "/logo"      // Logo folder

/***************************************************************************************
 * File System Constants
 ***************************************************************************************/
#define FILE_NAME_LENGTH  255        // Max file name length

/***************************************************************************************
 * Linked List Node Structure
 ***************************************************************************************/
typedef struct Link{
  int index_number;                  // Node index
  char file_name[FILE_NAME_LENGTH];  // File name
  struct Link *file_prev;            // Pointer to previous node
  struct Link *file_next;            // Pointer to next node
}list_link;

/***************************************************************************************
 * Global List Pointers
 ***************************************************************************************/
extern list_link *list_music;     // Music file list
extern list_link *list_picture;   // Picture file list
extern list_link *list_logo;      // Logo file list

/***************************************************************************************
 * SD Card Initialization
 ***************************************************************************************/

/**
 * @brief Initialize the SD_MMC module
 * @return 1 on success, 0 on failure
 */
int sdcard_init(void);

/***************************************************************************************
 * Folder Scanning Functions
 ***************************************************************************************/

/**
 * @brief Scan music folder and build linked list
 * @note Auto-creates folder if missing, scans for .mp3 files
 */
void setup_list_head_music(void);

/**
 * @brief Scan picture folder and build linked list
 * @note Auto-creates folder if missing, scans for .bmp files
 */
void setup_list_head_picture(void);

/**
 * @brief Scan logo folder and build linked list
 * @note Auto-creates folder if missing, scans for .jpg files
 */
void setup_list_head_logo(void);

/***************************************************************************************
 * File Operations
 ***************************************************************************************/

/**
 * @brief Write data to a file
 * @param path File path
 * @param buf  Data buffer
 * @param size Data size in bytes
 */
void write_file(char *path, uint8_t *buf, long size);

/**
 * @brief Save RGB565 data as a BMP image
 * @param path   File path
 * @param buf    RGB565 image data
 * @param size   Data size in bytes
 * @param height Image height
 * @param width  Image width
 */
void write_rgb565_to_bmp(char *path, uint8_t *buf, long size, long height, long width);

/**
 * @brief Create a folder if it does not exist
 * @param path Folder path
 */
void create_folder(char *path);

/**
 * @brief Delete a file
 * @param path File path
 */
void delete_file(char *path);

/***************************************************************************************
 * Linked List Operations
 ***************************************************************************************/

/**
 * @brief Read a folder and build a file linked list
 * @param dirname   Folder path
 * @param extension File extension (e.g. "mp3", "bmp")
 * @return Pointer to list head node
 */
list_link *sdcard_read_folder(char *dirname, char *extension);

/**
 * @brief Create a linked list node
 * @param index Node index
 * @param name  File name
 * @return Pointer to new node, or NULL on failure
 */
list_link *list_create_node(int index, char *name);

/**
 * @brief Insert a node at the tail of the list
 * @param phead List head node
 * @param name  File name
 */
void list_insert_tail(list_link *phead, char *name);

/**
 * @brief Print list contents in forward order
 * @param phead List head node
 */
void list_printf(list_link *phead);

/**
 * @brief Print list contents in reverse order
 * @param phead List head node
 */
void list_printf_back(list_link *phead);

/**
 * @brief Destroy the list and free all memory
 * @param phead List head node
 * @return NULL
 */
list_link *list_destory(list_link *phead);

/**
 * @brief Find a node by index
 * @param phead List head node
 * @param index Node index (starts at 1)
 * @return File name pointer, or NULL if not found
 */
char* list_find_node(list_link *phead, int index);

/**
 * @brief Count the number of nodes in the list
 * @param phead List head node
 * @return Node count (excluding head node)
 */
int list_count_number(list_link *phead);

#endif