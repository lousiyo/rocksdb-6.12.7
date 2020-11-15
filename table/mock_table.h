//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
#pragma once

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "db/version_edit.h"
#include "port/port.h"
#include "rocksdb/comparator.h"
#include "rocksdb/io_status.h"
#include "rocksdb/table.h"
#include "table/internal_iterator.h"
#include "table/table_builder.h"
#include "table/table_reader.h"
#include "test_util/testharness.h"
#include "test_util/testutil.h"
#include "util/kv_map.h"
#include "util/mutexlock.h"

namespace ROCKSDB_NAMESPACE {
namespace mock {

stl_wrappers::KVMap MakeMockFile(
    std::initializer_list<std::pair<const std::string, std::string>> l = {});

struct MockTableFileSystem {
  port::Mutex mutex;
  std::map<uint32_t, stl_wrappers::KVMap> files;
};

class MockTableFactory : public TableFactory {
 public:
  enum MockCorruptionMode {
    kCorruptNone,
    kCorruptKey,
    kCorruptValue,
  };

  MockTableFactory();
  const char* Name() const override { return "MockTable"; }
  using TableFactory::NewTableReader;
  Status NewTableReader(
      const ReadOptions& ro, const TableReaderOptions& table_reader_options,
      std::unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
      std::unique_ptr<TableReader>* table_reader,
      bool prefetch_index_and_filter_in_cache = true) const override;
  TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      uint32_t column_familly_id, WritableFileWriter* file) const override;

  // This function will directly create mock table instead of going through
  // MockTableBuilder. file_contents has to have a format of <internal_key,
  // value>. Those key-value pairs will then be inserted into the mock table.
  Status CreateMockTable(Env* env, const std::string& fname,
                         stl_wrappers::KVMap file_contents);

  virtual Status SanitizeOptions(
      const DBOptions& /*db_opts*/,
      const ColumnFamilyOptions& /*cf_opts*/) const override {
    return Status::OK();
  }

  virtual std::string GetPrintableTableOptions() const override {
    return std::string();
  }

  void SetCorruptionMode(MockCorruptionMode mode) { corrupt_mode_ = mode; }
  // This function will assert that only a single file exists and that the
  // contents are equal to file_contents
  void AssertSingleFile(const stl_wrappers::KVMap& file_contents);
  void AssertLatestFile(const stl_wrappers::KVMap& file_contents);

 private:
  uint32_t GetAndWriteNextID(WritableFileWriter* file) const;
  uint32_t GetIDFromFile(RandomAccessFileReader* file) const;

  mutable MockTableFileSystem file_system_;
  mutable std::atomic<uint32_t> next_id_;
  MockCorruptionMode corrupt_mode_;
};

}  // namespace mock
}  // namespace ROCKSDB_NAMESPACE