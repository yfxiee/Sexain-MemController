// addr_trans_table.h
// Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>

#ifndef SEXAIN_ADDR_TRANS_TABLE_H_
#define SEXAIN_ADDR_TRANS_TABLE_H_

#include <cerrno>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "index_queue.h"

enum EntryState {
  DIRTY_ENTRY = 0,
  CLEAN_ENTRY,
  FREE_ENTRY,
};

struct ATTEntry {
  uint64_t phy_tag;
  uint64_t mach_addr;
  IndexNode queue_node;
  EntryState state;

  ATTEntry() : state(FREE_ENTRY) {
    queue_node.first = queue_node.second = -EINVAL;
  }
};

class AddrTransTable : public IndexArray {
 public:
  AddrTransTable(int length, int block_bits);

  uint64_t Lookup(uint64_t phy_tag, EntryState* state);
  void Setup(uint64_t phy_tag, uint64_t mach_addr);
  void Revoke(uint64_t phy_tag);
  ///
  /// Replace an existing clean mapping with the specified one.
  /// @return the replaced clean mapping
  ///
  std::pair<uint64_t, uint64_t> Replace(uint64_t phy_tag, uint64_t mach_addr);
  int Clean();

  bool IsEmpty(EntryState state) { return queues_[state].Empty(); }
  int GetLength(EntryState state) { return queues_[state].length(); }

  uint64_t Tag(uint64_t addr) { return addr >> block_bits_; }
  uint64_t Addr(uint64_t tag) { return tag << block_bits_; }
  uint64_t Translate(uint64_t phy_addr, uint64_t mach_addr);

  int length() const { return length_; }
  int block_size() const { return 1 << block_bits_; }
  int block_bits() const { return block_bits_; }
  IndexNode& operator[](int i) { return entries_[i].queue_node; }

 private:
  const int length_;
  const int block_bits_;
  const uint64_t block_mask_;
  std::unordered_map<uint64_t, int> tag_index_;
  std::vector<ATTEntry> entries_;
  std::vector<IndexQueue> queues_;
};

inline AddrTransTable::AddrTransTable(int length, int block_bits) :
    length_(length), block_bits_(block_bits), block_mask_(block_size() - 1),
    entries_(length_), queues_(3, *this) {
  for (int i = 0; i < length_; ++i) {
    queues_[FREE_ENTRY].PushBack(i);
  }
}

inline uint64_t AddrTransTable::Translate(
    uint64_t phy_addr, uint64_t mach_addr) {
  return mach_addr + (phy_addr & block_mask_);
}

#endif // SEXAIN_ADDR_TRANS_TABLE_H_

