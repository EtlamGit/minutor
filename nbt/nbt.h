/** Copyright (c) 2013, Sean Kasun */
#ifndef NBT_H_
#define NBT_H_

#include <QString>

#include "nbt/tag.h"


class NBT {
 public:
  explicit NBT(const QString level);
  explicit NBT(const uchar *chunk);
  ~NBT();

  bool        has(const QString key) const;
  const Tag * at(const QString key) const;

  static Tag Null;

 private:
  void unpack_zlib(const unsigned char * data, unsigned long length, int windowsize = 15);
  void unpack_lz4(const unsigned char * data, unsigned long length);
  void decode_nbt(const unsigned char * data, unsigned long length);
  void decode_nbt(const char * data, unsigned long length);

  Tag * root;
};

#endif  // NBT_H_
