/** Copyright (c) 2013, Sean Kasun */

#include "chunkloader.h"
#include "chunkcache.h"
#include "chunk.h"


ChunkLoader::ChunkLoader(QString path, int cx, int cz)
  : path(path)
  , cx(cx), cz(cz)
  , cache(ChunkCache::Instance())
{}

ChunkLoader::~ChunkLoader()
{}

void ChunkLoader::run() {
  // get existing Chunk entry from Cache
  QSharedPointer<Chunk> chunk(cache.fetchCached(cx, cz));
  // load & parse NBT data
  loadNbt(path, cx, cz, chunk);
  emit loaded(cx, cz);
}

bool ChunkLoader::loadNbt(QString path, int cx, int cz, QSharedPointer<Chunk> chunk)
{
  // check if chunk is a valid storage
  if (!chunk) {
    return false;
  }

  // get coordinates of region file
  int rx = cx >> 5;
  int rz = cz >> 5;

  QString filename;

  filename = path + "/region/r." + QString::number(rx) + "." + QString::number(rz) + ".mca";
  bool result = loadNbtHelper(filename, cx, cz, chunk, ChunkLoader::MAIN_MAP_DATA);

  filename = path + "/entities/r." + QString::number(rx) + "." + QString::number(rz) + ".mca";
  loadNbtHelper(filename, cx, cz, chunk, ChunkLoader::SEPARATED_ENTITIES);

  return result;
}

bool ChunkLoader::loadNbtHelper(QString filename, int cx, int cz, QSharedPointer<Chunk> chunk, int loadtype)
{
  QFile f(filename);

  if (!f.open(QIODevice::ReadOnly)) {
    // no chunks in this region (region file not present at all)
    return false;
  }

  const int headerSize = 4096;

  if (f.size() < headerSize) {
    // file header not yet fully written by minecraft
    return false;
  }

  // map header into memory
  uchar *header = f.map(0, headerSize);
  int offset = 4 * ((cx & 31) + (cz & 31) * 32);

  int coffset = (header[offset] << 16) | (header[offset + 1] << 8) | header[offset + 2];
  int numSectors = header[offset+3];
  f.unmap(header);

  if (coffset == 0) {
    // no Chunk information stored in region file
    f.close();
    return false;
  }

  const int chunkStart = coffset * 4096;
  const int chunkSize = numSectors * 4096;

  // Check if chunk header (5 bytes: 4 length + 1 compression) is readable
  if (f.size() < chunkStart + 5) {
    f.close();
    return false;
  }

  // Read chunk header to get actual data length
  f.seek(chunkStart);
  char headerBuf[5];
  if (f.read(headerBuf, 5) != 5) {
    f.close();
    return false;
  }
  const uchar *hdr = reinterpret_cast<const uchar*>(headerBuf);
  int actualLength = (hdr[0] << 24) | (hdr[1] << 16) | (hdr[2] << 8) | hdr[3];

  // Sanity check: length must be positive and fit within allocated sectors
  if (actualLength <= 0 || actualLength + 4 > chunkSize) {
    f.close();
    return false;
  }

  // Check actual data fits in file (handles unpadded files like WorldTools exports)
  if (f.size() < chunkStart + 4 + actualLength) {
    f.close();
    return false;
  }

  uchar *raw = f.map(chunkStart, actualLength + 4);
  if (raw == NULL) {
    f.close();
    return false;
  }
  // parse Chunk data
  // Chunk will be flagged "loaded" in a thread save way
  NBT nbt(raw);
  switch (loadtype) {
    case ChunkLoader::MAIN_MAP_DATA:
      chunk->load(nbt);
      Q_FALLTHROUGH();
    case ChunkLoader::SEPARATED_ENTITIES:
      chunk->loadEntities(nbt);
  }
  f.unmap(raw);
  f.close();

  // if we reach this point, everything went well
  return true;
}
