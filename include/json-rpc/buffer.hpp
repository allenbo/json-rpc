#ifndef __JSONRPC_BUFFER_HPP__
#define __JSONRPC_BUFFER_HPP__

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>

static const size_t KB = 1024;
static const size_t MB = KB * KB;

static const size_t MIN_BUFFER_SIZE = KB;

class RDONBuffer {
  public:
    RDONBuffer(const char* str, int size)
        :_read_only_str(str), _size(size), _pos(0){
      assert(_size > 0);
    }

    virtual size_t read(void* ptr, size_t size) {
      if (_pos == _size) return 0;

      size_t real_size = size;
      if (_pos + size > _size)  {
        real_size = _size - _pos;
      }

      memcpy(ptr, _read_only_str + _pos, real_size);
      _pos += real_size;
      return real_size;
    }

    size_t size() const  { return _size;}
    const char* c_str() const { return _read_only_str; }
  private:
    const char* _read_only_str;
    size_t _size;
    size_t _pos;
};

class WRONBuffer {
  public:
    WRONBuffer() {
      _size = MIN_BUFFER_SIZE;
      _write_only_str = new char[_size];
      _pos = 0;
    }

    WRONBuffer(size_t size){
      assert(size > 0);
      size_t real_size = std::max(size, MIN_BUFFER_SIZE);
      _write_only_str = new char[real_size];
      _size = real_size;
      _pos = 0;
    }

    ~WRONBuffer() {
      _size = 0;
      _pos = 0;
      delete [] _write_only_str;
    }

    virtual size_t write(void* ptr, size_t size) {
      if (_pos + size > _size) {
        int new_size = _size + KB > _pos + size ? _size + KB : _pos + size;

        char* new_str = new char[new_size];
        memcpy(new_str, _write_only_str, _pos);
        delete [] _write_only_str;
        _write_only_str = new_str;
      }

      memcpy(_write_only_str + _pos, ptr, size);
      _pos += size;
      return size;
    }

    size_t size() const { return _pos; }
    const char* c_str() const { return _write_only_str; }
  private:
    char* _write_only_str;
    size_t _size;
    size_t _pos;
    
};

#endif