#include "sd_card.h"
#include "FS.h"
#include "SD_MMC.h"

/***************************************************************************************
 * Global List Pointers
 ***************************************************************************************/
list_link *list_music = NULL;     // Music file list
list_link *list_picture = NULL;   // Picture file list
list_link *list_logo = NULL;      // Logo file list

/***************************************************************************************
 * SD Card Initialization
 ***************************************************************************************/

int sdcard_init(void) {
  Serial.println("Initializing SD card...");
  
  // Configure SD_MMC pins
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

  // Initialize SD_MMC (1-bit mode)
  int success = SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5);
  
  if (!success) {
    Serial.println("✗ SD card mount failed!");
    return 0;
  }
  
  // Print SD card info
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("✓ SD card mounted successfully\n");
  Serial.printf("  Card size: %llu MB\n", cardSize);
  Serial.printf("  Card type: ");
  
  switch (SD_MMC.cardType()) {
    case CARD_MMC:
      Serial.println("MMC");
      break;
    case CARD_SD:
      Serial.println("SDSC");
      break;
    case CARD_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("UNKNOWN");
      break;
  }
  
  return 1;
}

/***************************************************************************************
 * Folder Scanning Functions
 ***************************************************************************************/

void setup_list_head_music(void) {
  create_folder(MUSIC_FOLDER);
  
  if (list_music != NULL) {
    list_music = list_destory(list_music);
  }
  
  list_music = sdcard_read_folder(MUSIC_FOLDER, "mp3");
  Serial.printf("Found %d music files\n", list_count_number(list_music));
}

void setup_list_head_picture(void) {
  create_folder(PICTURE_FOLDER);
  
  if (list_picture != NULL) {
    list_picture = list_destory(list_picture);
  }
  
  list_picture = sdcard_read_folder(PICTURE_FOLDER, "bmp");
  Serial.printf("Found %d picture files\n", list_count_number(list_picture));
}

void setup_list_head_logo(void) {
  create_folder(LOGO_FOLDER);
  
  if (list_logo != NULL) {
    list_logo = list_destory(list_logo);
  }
  
  list_logo = sdcard_read_folder(LOGO_FOLDER, "jpg");
  Serial.printf("Found %d logo files\n", list_count_number(list_logo));
}

/***************************************************************************************
 * File Operations
 ***************************************************************************************/

void write_file(char *path, uint8_t *buf, long size) {
  if (path == NULL || buf == NULL || size == 0) {
    Serial.println("✗ Invalid parameters for write_file");
    return;
  }
  
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("✗ Failed to open file: %s\n", path);
    return;
  }
  
  long written = file.write(buf, size);
  file.close();
  
  if (written == size) {
    Serial.printf("✓ File written: %s (%ld bytes)\n", path, size);
  } else {
    Serial.printf("✗ Write failed: %s (wrote %ld/%ld bytes)\n", path, written, size);
  }
}

void write_rgb565_to_bmp(char *path, uint8_t *buf, long size, long height, long width) {
  if (path == NULL || buf == NULL || size == 0) {
    Serial.println("✗ Invalid parameters for write_rgb565_to_bmp");
    return;
  }
  
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("✗ Failed to create BMP file: %s\n", path);
    return;
  }
  
  // BMP file header (70 bytes)
  uint8_t bmp_header[] = {
    0x42, 0x4d, 0x48, 0xc2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x38, 0x00,
    0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x02, 0xc2, 0x01, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x1f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  uint8_t bmp_footer[] = {0x00, 0x00};
  
  // Write BMP header
  file.write(bmp_header, 70);

  // BMP format requires writing rows bottom-to-top
  for (int i = height - 1; i >= 0; i--) {
    file.write(&buf[i * width * 2], width * 2);
  }
  
  // Write footer padding
  file.write(bmp_footer, 2);
  file.close();
  
  Serial.printf("✓ BMP saved: %s (%ldx%ld)\n", path, width, height);
}

void create_folder(char *path) {
  if (path == NULL) {
    return;
  }
  
  File root = SD_MMC.open(path);
  
  if (!root) {
    // Folder does not exist, attempt to create it
    if (SD_MMC.mkdir(path)) {
      Serial.printf("✓ Folder created: %s\n", path);
    } else {
      Serial.printf("✗ Failed to create folder: %s\n", path);
    }
  }
  
  root.close();
}

void delete_file(char *path) {
  if (path == NULL) {
    return;
  }
  
  // Remove path prefix if present
  String filepath = String(path);
  if (filepath.startsWith("//")) {
    filepath.remove(0, 2);
  }
  
  if (SD_MMC.remove(filepath.c_str())) {
    Serial.printf("✓ File deleted: %s\n", filepath.c_str());
  } else {
    Serial.printf("✗ Failed to delete: %s\n", filepath.c_str());
  }
}

/***************************************************************************************
 * Linked List Operations
 ***************************************************************************************/

list_link *sdcard_read_folder(char *dirname, char *extension) {
  if (dirname == NULL || extension == NULL) {
    Serial.println("✗ Invalid parameters for sdcard_read_folder");
    return NULL;
  }
  
  // Create head node
  list_link *head = list_create_node(0, dirname);
  if (head == NULL) {
    return NULL;
  }
  
  File root = SD_MMC.open(dirname);
  if (!root) {
    Serial.printf("✗ Failed to open directory: %s\n", dirname);
    return head;
  }
  
  if (!root.isDirectory()) {
    Serial.printf("✗ Not a directory: %s\n", dirname);
    root.close();
    return head;
  }
  
  // Iterate through folder
  File file = root.openNextFile();
  while (file) {
    String filename = String(file.name());

    // Check file extension
    if (filename.endsWith(extension)) {
      list_insert_tail(head, (char *)filename.c_str());
    }
    
    file = root.openNextFile();
  }
  
  root.close();
  return head;
}

list_link *list_create_node(int index, char *name) {
  if (name == NULL) {
    return NULL;
  }
  
  list_link *node = (list_link *)malloc(sizeof(list_link));
  if (node == NULL) {
    Serial.println("✗ Failed to allocate memory for list node");
    return NULL;
  }
  
  node->index_number = index;
  memset(node->file_name, 0, FILE_NAME_LENGTH);
  strncpy(node->file_name, name, FILE_NAME_LENGTH - 1);  // Use strncpy to prevent overflow
  node->file_prev = NULL;
  node->file_next = NULL;
  
  return node;
}

void list_insert_tail(list_link *phead, char *name) {
  if (phead == NULL || name == NULL) {
    return;
  }
  
  // Find tail of list
  list_link *tail = phead;
  while (tail->file_next != NULL) {
    tail = tail->file_next;
  }

  // Create new node
  list_link *new_node = list_create_node(tail->index_number + 1, name);
  if (new_node == NULL) {
    return;
  }

  // Insert at tail
  new_node->file_prev = tail;
  tail->file_next = new_node;
}

void list_printf(list_link *phead) {
  if (phead == NULL) {
    Serial.println("List is empty");
    return;
  }
  
  list_link *current = phead;
  Serial.println("=== File List ===");
  while (current != NULL) {
    Serial.printf("[%d] %s\n", current->index_number, current->file_name);
    current = current->file_next;
  }
  Serial.println("=================");
}

void list_printf_back(list_link *phead) {
  if (phead == NULL) {
    Serial.println("List is empty");
    return;
  }
  
  // Find tail node
  list_link *tail = phead;
  while (tail->file_next != NULL) {
    tail = tail->file_next;
  }

  // Print in reverse
  Serial.println("=== File List (Reverse) ===");
  while (tail != NULL) {
    Serial.printf("[%d] %s\n", tail->index_number, tail->file_name);
    tail = tail->file_prev;
  }
  Serial.println("===========================");
}

list_link *list_destory(list_link *phead) {
  if (phead == NULL) {
    return NULL;
  }
  
  list_link *current = phead;
  list_link *next_node;
  
  while (current != NULL) {
    next_node = current->file_next;
    free(current);
    current = next_node;
  }
  
  return NULL;
}

char *list_find_node(list_link *phead, int index) {
  if (phead == NULL || index <= 0) {
    return NULL;
  }
  
  int max_index = list_count_number(phead);
  if (index > max_index) {
    return NULL;
  }
  
  list_link *current = phead;
  while (current != NULL) {
    if (current->index_number == index) {
      return current->file_name;
    }
    current = current->file_next;
  }
  
  return NULL;
}

int list_count_number(list_link *phead) {
  if (phead == NULL) {
    return 0;
  }
  
  int count = 0;
  list_link *current = phead;
  
  while (current->file_next != NULL) {
    count++;
    current = current->file_next;
  }
  
  return count;
}
