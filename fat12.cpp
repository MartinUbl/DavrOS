#include "fat12.h"
#include "std.h"

#include "console.h"

int FAT12Reader::Initialize(BlockDataSource* src)
{
    m_source = src;

    // seek to the very beginning and read bootsector
    m_source->SeekTo(0);
    m_source->ReadBytes((char*)&m_bootsector, sizeof(m_bootsector));

    // seek to next sector
    m_source->SeekTo(m_bootsector.bytes_per_sector);

    // read FAT tables
    m_fat_table_copies = new uint8_t*[m_bootsector.table_count];
    int pos;
    for (uint8_t i = 0; i < m_bootsector.table_count; i++)
    {
        // store position
        pos = m_source->GetPosition();

        m_fat_table_copies[i] = new uint8_t[m_bootsector.bytes_per_sector];
        m_source->ReadBytes((char*)m_fat_table_copies[i], m_bootsector.bytes_per_sector);

        // seek to next sector start (may be next table, may be rootdir block)
        m_source->SeekTo(pos + m_bootsector.table_size_16*m_bootsector.bytes_per_sector);
    }
    m_fat_table = m_fat_table_copies[0];

    m_root_directory_real_count = 0;
    // read root directory entries
    m_root_directory_entries = new fat12_directory_entry[m_bootsector.root_entry_count + 1];
    m_formatted_entries = new DirectoryEntry*[m_bootsector.root_entry_count + 1];
    for (uint16_t i = 0; i < m_bootsector.root_entry_count; i++)
    {
        m_source->ReadBytes((char*)&m_root_directory_entries[m_root_directory_real_count], sizeof(fat12_directory_entry));
        if (m_root_directory_entries[m_root_directory_real_count].name[0] == 0)
            break;
        if (m_root_directory_entries[m_root_directory_real_count].name[0] == 0xE5)
            continue;

        m_formatted_entries[m_root_directory_real_count] = new DirectoryEntry;
        m_formatted_entries[m_root_directory_real_count]->name = new char[12];
        strncpy((const char*)m_root_directory_entries[m_root_directory_real_count].name, m_formatted_entries[m_root_directory_real_count]->name, 11);
        m_formatted_entries[m_root_directory_real_count]->name[11] = '\0';
        m_formatted_entries[m_root_directory_real_count]->size_bytes = m_root_directory_entries[m_root_directory_real_count].file_size;

        m_root_directory_real_count++;
    }
    m_formatted_entries[m_root_directory_real_count] = nullptr;

    m_first_cluster_offset = m_bootsector.table_size_16 * m_bootsector.table_count * m_bootsector.bytes_per_sector
                                + m_bootsector.root_entry_count * sizeof(fat12_directory_entry)
                                + m_bootsector.reserved_sector_count * m_bootsector.bytes_per_sector;

    return 0;
}

int FAT12Reader::GetNextCluster(int cluster)
{
    unsigned int fat_offset = cluster + (cluster / 2); // integer multiplication by 1.5 (12 bits, one byte is 8, so this "expands" offset)
    unsigned short table_value = *(unsigned short*)&m_fat_table[fat_offset];

    if (cluster & 0x0001)
        return table_value >> 4;

    return table_value & 0x0FFF;
}

char* FAT12Reader::ReadFile(const char* filename)
{
    char convertedname[11];
    int i, j;

    // convert name to proper format
    memset(convertedname, ' ', 11);
    for (i = 0, j = 0; i < 11 && filename[j] != '\0'; i++, j++)
    {
        if (filename[j] == '.' && i < 8)
        {
            i = 7;
            continue;
        }

        convertedname[i] = filename[j];
    }

    bool found = false;
    for (i = 0; i < m_root_directory_real_count; i++)
    {
        if (strncmp(convertedname, (char*)m_root_directory_entries[i].name, 11) == 0)
        {
            found = true;
            break;
        }
    }

    if (!found)
        return nullptr;

    char* ret = new char[m_root_directory_entries[i].file_size];
    int cluster = m_root_directory_entries[i].cluster_number_low;

    const int step = m_bootsector.bytes_per_sector * m_bootsector.sectors_per_cluster;
    int cursor = 0;

    while (cluster < 0xFF8)
    {
        m_source->SeekTo(m_first_cluster_offset + (cluster - 2) * m_bootsector.sectors_per_cluster * m_bootsector.bytes_per_sector);
        m_source->ReadBytes((char*)&ret[cursor], m_bootsector.sectors_per_cluster * m_bootsector.bytes_per_sector);

        cluster = GetNextCluster(cluster);
        cursor += step;
    }

    return ret;
}

DirectoryEntry** FAT12Reader::GetDirectoryList()
{
    return m_formatted_entries;
}
