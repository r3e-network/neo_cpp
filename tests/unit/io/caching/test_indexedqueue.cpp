#include <gtest/gtest.h>
#include <neo/io/caching/indexed_queue.h>

using namespace neo::io::caching;

class IndexedQueueTest : public ::testing::Test
{
  protected:
    IndexedQueue<int> queue_;
};

TEST_F(IndexedQueueTest, EnqueueDequeue)
{
    queue_.Enqueue(10);
    queue_.Enqueue(20);
    ASSERT_EQ(queue_.GetCount(), 2u);

    auto item = queue_.Dequeue();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 10);
    ASSERT_EQ(queue_.GetCount(), 1u);

    item = queue_.Dequeue();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 20);
    ASSERT_TRUE(queue_.IsEmpty());

    EXPECT_FALSE(queue_.Dequeue().has_value());
}

TEST_F(IndexedQueueTest, RemoveByValue)
{
    queue_.Enqueue(1);
    queue_.Enqueue(2);
    queue_.Enqueue(3);

    EXPECT_TRUE(queue_.Remove(2));
    EXPECT_EQ(queue_.GetCount(), 2u);

    auto first = queue_.Dequeue();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first.value(), 1);

    auto second = queue_.Dequeue();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second.value(), 3);
}

TEST_F(IndexedQueueTest, Contains)
{
    queue_.Enqueue(5);
    queue_.Enqueue(6);
    EXPECT_TRUE(queue_.Contains(5));
    EXPECT_TRUE(queue_.Contains(6));
    EXPECT_FALSE(queue_.Contains(7));
}

TEST_F(IndexedQueueTest, Clear)
{
    queue_.Enqueue(100);
    queue_.Enqueue(200);
    queue_.Clear();
    EXPECT_TRUE(queue_.IsEmpty());
}

