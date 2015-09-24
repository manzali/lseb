#ifndef COMMON_MEMORY_POOL_H
#define COMMON_MEMORY_POOL_H

#include <vector>

#include <cassert>
#include <sys/uio.h>

namespace lseb {

class MemoryPool {
  void* m_buffer;
  size_t m_buffer_len;
  size_t m_chunk_len;
  std::vector<iovec> m_pool;

 public:
  MemoryPool(void* buffer, size_t buffer_len, size_t chunk_len)
      :
        m_buffer(buffer),
        m_buffer_len(buffer_len),
        m_chunk_len(chunk_len) {
    assert(m_chunk_len <= m_buffer_len && "Wrong sizes");
    size_t const end_len = m_buffer_len - m_chunk_len;
    for (size_t cur_len = 0; cur_len <= end_len; cur_len += m_chunk_len) {
      m_pool.push_back( { m_buffer + cur_len, m_chunk_len });
    }
  }

  iovec alloc() {
    iovec iov = m_pool.back();
    m_pool.pop_back();
    return iov;
  }

  void free(iovec iov) {
    assert(iov.iov_len <= m_chunk_len);
    assert(iov.iov_base >= m_buffer);
    assert(iov.iov_base + iov.iov_len <= m_buffer + m_buffer_len);
    m_pool.push_back(iov);
  }

  MemoryPool(const MemoryPool&) = delete;            // disable copying
  MemoryPool& operator=(const MemoryPool&) = delete;  // disable assignment
};

}

#endif
