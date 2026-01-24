#ifndef TYPES_H
#define TYPES_H
#define C8_SCREEN_H 32
#define C8_SCREEN_W 64
#define ALLOC_FAILURE -1
struct vec2i{
  i16 x,y;
  void operator+=(vec2i other){
	this->x += other.x;
	this->y += other.y;
  }
  vec2i operator+(vec2i other){
	vec2i ret;
	ret.x = this->x + other.x;
	ret.y = this->y + other.y;
	return ret;
  }
};
// abstraction for up scaling screen;
struct pixel{
  vec2i pos;
  byte on = 0;;
};
struct chip8{
  u16 pc;
  u16 sp;
  u16 ireg;
  u16 stack[16];
  pixel display[C8_SCREEN_H * C8_SCREEN_W];
  byte dt;
  byte v[16];
  byte ram[4096];
  byte kb[16];
  byte st; //unused;
};

template <typename T> struct Dyn_Arry {
  size_t cap;
  size_t len;
  T *buffer;
  /*pushes elements to the back will allocate if cap is too small*/
  int append(T element) {
    if (len >= cap) {
      int pass = grow(10);
      if (pass == ALLOC_FAILURE) {
        return ALLOC_FAILURE;
      }
    }
    buffer[len] = element;
    len++;
    return 0;
  }
  int append_arr(T *ptr, size_t size) {
    if (len + size > cap) {
      int code = grow((cap + size) + 10);
      if (code == ALLOC_FAILURE) {
        return ALLOC_FAILURE;
      }
    }
    for (int i = 0; i < size; i++) {
      buffer[len] = ptr[i];
      len++;
    }
    return 0;
  }
  /* reallocs the buffer and updates the meta data of the struct*/
  int grow(size_t size) {
    T *tmp = (T *)realloc(buffer, (cap + size) * sizeof(T));
    if (tmp == NULL) {
      return ALLOC_FAILURE;
    }
    buffer = tmp;
    cap += size;
    return 0;
  }
  void free_arr() {
    free(buffer);
    cap = 0;
    len = 0;
  }
  /*zeros the buffer*/
  inline void erase() {
    memset(buffer, 0, cap - 1);
    len = 0;
  }
  /*replaces the element at the given index with the supplied value*/
  inline void replace_at(size_t index, T elem) {
    if (index >= len) {
      abort();
    }
    buffer[index] = elem;
  }
  /*This can allocate default is to allocate space for 10 more elments*/
  int insert_at(size_t index, T elem) {
    if (len + 1 >= cap) {
      int suc = grow(10);
      if (suc == ALLOC_FAILURE) {
        return suc;
      }
    }
    len++;
    T holder1 = buffer[index];
    T holder2;
    buffer[index] = elem;
    for (int i = index + 1; i < len + 1; i++) {
      holder2 = buffer[i];
      buffer[i] = holder1;
      holder1 = holder2;
    }
    return 0;
  }
  /*
    This does not rezise the buffer but deletes whatever element and shifts them
    down
  */
  void delete_at(size_t index) {
    T holder1 = buffer[index + 1];
    buffer[index] = holder1;
    for (int i = index + 1; i < len + 1; i++) {
      holder1 = buffer[i];
      buffer[i - 1] = holder1;
    }
    len--;
  }
  // this does not shrink the array just 0s the last element and decrements the
  // len
  inline void pop() {
    buffer[len - 1] = 0;
    len--;
  }
  /*shrinks the array by the given argument */
  int shrink(size_t decrement) {
    T *tmp = (T *)realloc(buffer, (cap - decrement * sizeof(T)));
    if (tmp == NULL) {
      return ALLOC_FAILURE;
    }
    buffer = tmp;
    cap -= decrement;
    return 0;
  }
  T operator[](size_t index) {
    if (index >= len) {
	  abort();
    }
    return buffer[index];
  }
};


template <typename T> Dyn_Arry<T> new_dyn_arry(size_t size) {
  Dyn_Arry<T> ret;
  ret.buffer = (T *)calloc(size, sizeof(T));
  ret.cap = size;
  ret.len = 0;
  return ret;
}


#endif
