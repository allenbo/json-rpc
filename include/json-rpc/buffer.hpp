#ifndef __JSONRPC_BUFFER_HPP__
#define __JSONRPC_BUFFER_HPP__

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>

static const size_t KB = 1024;
static const size_t MB = KB * KB;

static const size_t MIN_BUFFER_SIZE = KB;

/**
 * A read only buffer that takes a char* and it's size as parameters. You can
 * keep reading chunks of data from this buffer
 **/
class RDONBuffer {
  public:
    RDONBuffer(const char* str, int size)
        :_read_only_str(str), _size(size), _pos(0){
      assert(_size > 0);
    }

    virtual ~RDONBuffer() {}

    virtual size_t read(void* ptr, size_t size) {
      if (_pos == _size) return 0;

      size_t real_size = size;
      if (_pos + size > _size)  {
        real_size = _size - _pos;
      }

      memcpy(ptr, (void*)(_read_only_str + _pos), real_size);
      _pos += real_size;
      return real_size;
    }

    size_t size() const  { return _size;}
    const char* c_str() const { return _read_only_str; }
    void reset() { _pos = 0; }
  protected:
    const char* _read_only_str;
    size_t _size;
    size_t _pos;
};

/**
 * A heavy read only buffer. The only differece from RDONBuffer is this buffer
 * copy string into an array in it.
 **/
class HRDONBuffer : public RDONBuffer{
  public:
    HRDONBuffer(const char* str, int size)
        : RDONBuffer(str, size) {
      const char* temp = _read_only_str;
      _read_only_str = new char[_size];
      memcpy((void*)_read_only_str, (void*)temp, _size);
    }

    ~HRDONBuffer() {
      delete [] _read_only_str;
    }
};


/**
 * A buffer with it's string's length embedded at the first 4 bytes
 **/
class SizedRDONBuffer : public RDONBuffer {
  public:
    SizedRDONBuffer(const char* str, int size)
        : RDONBuffer(str, size) {
      const char* temp = _read_only_str;
      _read_only_str = new char[sizeof(int) + _size];
      *(int*)_read_only_str = size;
      memcpy((void*)(_read_only_str + sizeof(int)), (void*)temp, _size);
      _size = _size + sizeof(int);
    }

    ~SizedRDONBuffer() {
      delete []_read_only_str;
    }

    inline bool ready() { return _pos == _size;}
};


/**
 * A write only buffer. You can write chunks of data into this buffer and
 * get them as a contiguous string
 **/
class WRONBuffer {
  public:
    WRONBuffer() {
      _size = MIN_BUFFER_SIZE;
      _write_only_str = new char[_size];
      _pos = 0;
    }

    WRONBuffer(size_t size){
      assert(size > 0);
      size_t real_size = size;
      _write_only_str = new char[real_size];
      _size = real_size;
      _pos = 0;
    }

    ~WRONBuffer() {
      _size = 0;
      _pos = 0;
      delete [] _write_only_str;
    }

    size_t write(void* ptr, size_t size) {
      if (_pos + size > _size) {
        int new_size = _size + KB > _pos + size ? _size + KB : _pos + size;

        char* new_str = new char[new_size];
        memcpy((void*)new_str, (void*)_write_only_str, _pos);
        delete [] _write_only_str;
        _write_only_str = new_str;
      }

      memcpy((void*)(_write_only_str + _pos), ptr, size);
      _pos += size;
      return size;
    }

    size_t size() const { return _pos; }
    const char* c_str() const { return _write_only_str; }
    void reset() {
      _pos = 0;
      memset((void*)_write_only_str, 0, _size);
    }
  protected:
    char* _write_only_str;
    size_t _size;
    size_t _pos;
    
};

/**
 * A write only buffer that restricts how many bytes you can write into it.
 **/
class SizedWRONBuffer : public WRONBuffer {
  public:
    SizedWRONBuffer(size_t size)
        : WRONBuffer(size) {
    }

    size_t write(void* ptr, size_t size) {
      if (_pos == _size) return 0;
      if (_pos + size > _size) {
        size = _size - _pos;
      }

      memcpy((void*)(_write_only_str + _pos), ptr, size);
      _pos += size;
      return size;
    }

    inline bool ready() { return _pos == _size;}
};

#endif
