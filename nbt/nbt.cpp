/** Copyright (c) 2013, Sean Kasun */

#include <zlib.h>
#include <QFile>

#include "nbt/nbt.h"


// this handles decoding the gzipped level.dat
NBT::NBT(const QString level)
  : root(&NBT::Null)  // just in case we die, init with empty data
{
  QFile f(level);
  f.open(QIODevice::ReadOnly);
  QByteArray data = f.readAll();
  f.close();

  // level.dat is typically gzip format, but autodetect here
  unpack_zlib(reinterpret_cast<Bytef *>(data.data()), data.size(), 15 + 32);    // +32 to autodetect gzip/zlib compression
}

// this handles decoding a compressed Chunk of a region file
NBT::NBT(const uchar *chunk)
  : root(&NBT::Null)  // just in case we die, init with empty data
{
  // find chunk size in first 4 bytes, format is fifth byte
  int length = (chunk[0] << 24) | (chunk[1] << 16) | (chunk[2] << 8) | chunk[3];
  const unsigned char * data = reinterpret_cast<const Bytef *>(chunk) + 5;
  // supported compression formats
  if (chunk[4] == 1) unpack_zlib(data, length - 1, 15 + 16);   // rfc1952 not used by official Minecraft
  if (chunk[4] == 2) unpack_zlib(data, length - 1, 15 + 0);    // rfc1950 default for all Chunk data
}

Tag NBT::Null;


static const int CHUNK_SIZE = 8192;
// default window size: 15 bit
// + 0 zlib data (RFC 1950)
// +16 gzip data (RFC 1952)
// +32 autodetect zlib/gzip from header
void NBT::unpack_zlib(const unsigned char * data, unsigned long length, int windowsize) {

  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = length;
  stream.next_in  = const_cast<unsigned char *>(data);  // yes we violate "const", but zlib will not change the input data

  char out[CHUNK_SIZE];

  QByteArray nbt;

  inflateInit2(&stream, windowsize);
  do {
    stream.avail_out = CHUNK_SIZE;
    stream.next_out = reinterpret_cast<Bytef *>(out);
    inflate(&stream, Z_NO_FLUSH);
    nbt.append(out, CHUNK_SIZE - stream.avail_out);
  } while (stream.avail_out == 0);
  inflateEnd(&stream);

  TagDataStream s(nbt.constData(), nbt.size());

  if (s.r8() == Tag::TAG_COMPOUND) {  // outer compound is expected
    s.skip(s.r16());  // skip name (should be empty anyways)
    root = new Tag_Compound(&s);
  }
}

bool NBT::has(const QString key) const {
  return root->has(key);
}

const Tag *NBT::at(const QString key) const {
  return root->at(key);
}

NBT::~NBT() {
  if (root != &NBT::Null)
    delete root;
}
