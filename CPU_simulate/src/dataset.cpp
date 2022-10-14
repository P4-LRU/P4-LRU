#include <arpa/inet.h>
#include "dataset.h"
#include "logger.h"
using namespace std;

void Dataset::init(string PATH, int size_per_item)
{
    struct stat buf;
    LOG_DEBUG("Opening file %s", PATH.c_str());
    int fd=Open(PATH.c_str(),O_RDONLY);
    fstat(fd,&buf);
    int n_elements = buf.st_size / size_per_item;
    TOTAL_PACKETS=n_elements;
    LOG_DEBUG("\tcnt=%d", n_elements);
    LOG_DEBUG("Mmap..."); 
    void* addr=mmap(NULL,buf.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    raw_data = new data_t[n_elements];
    close(fd);
    if (addr==MAP_FAILED)
    {
        LOG_ERROR("MMAP FAILED!");
        exit(-1);
    }

    char *ptr = reinterpret_cast<char *>(addr);
    for (int i = 0; i < n_elements; i++)
    {
        raw_data[i].id = *reinterpret_cast<header_t *>(ptr);
        raw_data[i].size = ntohs(*reinterpret_cast<uint16_t *>(ptr + 13));
        raw_data[i].timestamp = *reinterpret_cast<double *>(ptr + 15);
        ptr += size_per_item;
    }

    munmap(addr, buf.st_size);

    for (count_t i = 0; i < n_elements;i++)
    {
        auto it = counter.find(raw_data[i].id);
        if (it==counter.end())
        {
            counter.insert(std::make_pair(raw_data[i].id, raw_data[i].size));
        }
        else
        {
            it->second+=raw_data[i].size;
        }
    }
    TOTAL_FLOWS = counter.size();
    LOG_INFO("Total packets: %llu, Total flows: %llu", TOTAL_PACKETS, TOTAL_FLOWS);
}