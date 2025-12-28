/** Copyright (c) 2013, Sean Kasun */

#include <zlib.h>
#include <QFile>

#include "nbt/nbt.h"
#include "lz4/lz4.h"


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
  if (chunk[4] == 3) decode_nbt(data, length - 1);             // copy uncompressed data
  if (chunk[4] == 4) unpack_lz4(data, length - 1);             // LZ4 compression
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

  decode_nbt(nbt.constData(), nbt.size());
}


// Minecraft uses an incompatible Java implementation of LZ4
// -> so we have to decode magic headers by ourself
static const char LZ4_MAGIC[] = {'L', 'Z', '4', 'B', 'l', 'o', 'c', 'k'};
static const int  LZ4_MAGIC_LENGTH = sizeof(LZ4_MAGIC);

static const int LZ4_COMPRESSION_METHOD_RAW = 0x10;
static const int LZ4_COMPRESSION_METHOD_LZ4 = 0x20;
static const int LZ4_COMPRESSION_LEVEL_BASE = 10;
static const int LZ4_DEFAULT_SEED = 0x9747b28c;

// read an int from Little Endian byte stream
long readIntLE(const unsigned char * data) {
  return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
}

void NBT::unpack_lz4(const unsigned char * data, unsigned long length) {
  if (length < LZ4_MAGIC_LENGTH+13) return;

  QByteArray nbt;

  const unsigned char * input = data;

  while ((input - data) < length) {
    // decode LZ4-Java block header
    for (int m=0; m<LZ4_MAGIC_LENGTH; m++) {
      if (input[m] != LZ4_MAGIC[m]) return;
    }
    const unsigned char token = input[LZ4_MAGIC_LENGTH];
    const unsigned char compression_method = token & 0xF0;
    const unsigned char compression_level  = LZ4_COMPRESSION_LEVEL_BASE + (token & 0x0F);
    if ((compression_method != LZ4_COMPRESSION_METHOD_RAW) && (compression_method != LZ4_COMPRESSION_METHOD_LZ4)) return;
    const long compressed_length  = readIntLE(input + LZ4_MAGIC_LENGTH + 1);
    const long original_length    = readIntLE(input + LZ4_MAGIC_LENGTH + 5);
    const long checksum           = readIntLE(input + LZ4_MAGIC_LENGTH + 9);
    input += LZ4_MAGIC_LENGTH + 13;

    // special block indicating "no more data"
    if ((compressed_length == 0) && (original_length == 0)) break;
    // error checks
    if (compressed_length < 0) return;
    if (original_length < 0) return;
    if ((compressed_length == 0) && (original_length != 0)) return;
    if ((original_length == 0) && (compressed_length != 0)) return;
    if ((compression_method == LZ4_COMPRESSION_METHOD_RAW) && (original_length != compressed_length)) return;

    // input buffer overflow check
    if (((input - data) + compressed_length) > length) return;

    if (compression_method == LZ4_COMPRESSION_METHOD_RAW) {
      // copy RAW block
      nbt.append(reinterpret_cast<const char *>(input), original_length);
    } else {
      // decompress one block
      char * output = reinterpret_cast<char *>(malloc(original_length));
      int ret = LZ4_decompress_safe(reinterpret_cast<const char *>(input), output,
                                    compressed_length, original_length);
      nbt.append(output, ret);
      free(output);
    }
    // advance input data pointer
    input += compressed_length;
  }

  decode_nbt(nbt.constData(), nbt.size());
}

void NBT::decode_nbt(const unsigned char * data, unsigned long length) {
  decode_nbt(reinterpret_cast<const char *>(data),  length);
}

void NBT::decode_nbt(const char * data, unsigned long length) {
  TagDataStream s(data, length);

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
