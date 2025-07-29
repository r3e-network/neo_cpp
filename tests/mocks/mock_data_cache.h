#pragma once

#include "neo/persistence/data_cache.h"
#include <gmock/gmock.h>

namespace neo::tests
{

class MockDataCache : public persistence::DataCache
{
  public:
    MOCK_METHOD(void, Add, (const persistence::StorageKey& key, const persistence::StorageItem& item), (override));
    MOCK_METHOD(void, AddOrUpdate, (const persistence::StorageKey& key, const persistence::StorageItem& item),
                (override));
    MOCK_METHOD(void, Delete, (const persistence::StorageKey& key), (override));
    MOCK_METHOD(std::optional<persistence::StorageItem>, TryGet, (const persistence::StorageKey& key),
                (const, override));
    MOCK_METHOD(void, Commit, (), (override));
    MOCK_METHOD(std::unique_ptr<persistence::IIterator>, Find, (const persistence::StorageKey& prefix), (override));
};

}  // namespace neo::tests