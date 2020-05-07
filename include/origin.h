//
// Created by Alex on 2020/5/7.
//

#ifndef GEDITOR_ORIGIN_H
#define GEDITOR_ORIGIN_H
#if WIN32
#include <Windows.h>
#include <fileapi.h>
#include <memoryapi.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#endif
class Origin {
#if WIN32
    HANDLE m_hFile, m_hMapping;
#endif
    const void *m_pContent = nullptr;
    size_t m_nSize;
public:
    Origin(const char *file) {
#if WIN32
        m_hFile = CreateFileA(file, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        m_hMapping = CreateFileMappingA(m_hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
        m_pContent = (const char *) MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0);
        m_nSize = GetFileSize(m_hFile, 0);
#else
        int fd = open(file, O_RDWR);
        struct stat st;
        fstat(fd, &st);
        m_nSize = st.st_size;
        m_pContent = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        close(fd);
#endif
    }
    ~Origin() {
#if WIN32
        UnmapViewOfFile(m_pContent);
        CloseHandle(m_hMapping);
        CloseHandle(m_hFile);
#else
        munmap (m_pContent, m_nSize);
#endif
    }
    const void *ptr() { return m_pContent; }
    size_t size() { return m_nSize; }
};

#endif //GEDITOR_ORIGIN_H
