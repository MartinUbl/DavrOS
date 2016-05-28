#ifndef __FAT_12_H__
#define __FAT_12_H__

#include "filesystem.h"
#include "datasource.h"
#include "stdint.h"

typedef struct
{
	uint8_t bootjump[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t table_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t table_size_16;
	uint16_t sectors_per_track;
	uint16_t head_side_count;
	uint32_t hidden_sector_count;
	uint32_t total_sectors_32;

	uint8_t bios_drive_num;
	uint8_t reserved1;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t fat_type_label[8];
} __attribute__((packed)) fat12_bootsector;

typedef struct
{
    uint8_t name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_number_high; // FAT32 only
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t cluster_number_low;
    uint32_t file_size;
} __attribute__((packed)) fat12_directory_entry;

enum FAT12Flags
{
    FAT12_DIR_NO_MORE_ENTRIES       = 0x00,
    FAT12_DIR_UNUSED_ENTRY          = 0xE5
};

class FAT12Reader : public FileSystem
{
    public:
        int Initialize(BlockDataSource* src);
        DirectoryEntry** GetDirectoryList();
        char* ReadFile(const char* filename);

    protected:
        int GetNextCluster(int cluster);

    private:
        BlockDataSource* m_source;
        fat12_bootsector m_bootsector;

        uint8_t* m_fat_table;
        uint8_t** m_fat_table_copies;

        uint32_t m_first_cluster_offset;

        uint16_t m_root_directory_real_count;
        fat12_directory_entry* m_root_directory_entries;
        DirectoryEntry** m_formatted_entries;
};

#endif
