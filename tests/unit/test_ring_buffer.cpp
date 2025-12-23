#include <gtest/gtest.h>
#include "core/RingBuffer.h"
#include <vector>
#include <algorithm>

using namespace audiobridge;

class RingBufferTest : public ::testing::Test {
protected:
    // Use a power-of-2 size for testing
    static constexpr size_t kBufferSize = 256;
    RingBuffer<float, kBufferSize> buffer_;
};

TEST_F(RingBufferTest, InitialState) {
    EXPECT_EQ(buffer_.AvailableForRead(), 0);
    EXPECT_EQ(buffer_.AvailableForWrite(), kBufferSize);
}

TEST_F(RingBufferTest, WriteAndRead) {
    std::vector<float> inputData(100, 1.0f);
    std::vector<float> outputData(100, 0.0f);

    size_t written = buffer_.Write(inputData.data(), inputData.size());
    EXPECT_EQ(written, 100);
    EXPECT_EQ(buffer_.AvailableForRead(), 100);
    EXPECT_EQ(buffer_.AvailableForWrite(), kBufferSize - 100);

    size_t read = buffer_.Read(outputData.data(), outputData.size());
    EXPECT_EQ(read, 100);
    EXPECT_EQ(buffer_.AvailableForRead(), 0);
    EXPECT_EQ(buffer_.AvailableForWrite(), kBufferSize);

    // Verify data integrity
    EXPECT_TRUE(std::equal(inputData.begin(), inputData.end(), outputData.begin()));
}

TEST_F(RingBufferTest, WrapAround) {
    // Write to near end of buffer
    std::vector<float> data1(kBufferSize - 10, 1.0f);
    buffer_.Write(data1.data(), data1.size());

    // Read some data
    std::vector<float> readBuffer1(100, 0.0f);
    buffer_.Read(readBuffer1.data(), 100);

    // Write more data that will wrap around
    std::vector<float> data2(50, 2.0f);
    size_t written = buffer_.Write(data2.data(), data2.size());
    EXPECT_EQ(written, 50);

    // Read remaining data
    std::vector<float> readBuffer2(kBufferSize - 110, 0.0f);
    buffer_.Read(readBuffer2.data(), readBuffer2.size());

    // Read wrapped data
    std::vector<float> readBuffer3(50, 0.0f);
    buffer_.Read(readBuffer3.data(), 50);

    // Verify wrapped data
    EXPECT_FLOAT_EQ(readBuffer3[0], 2.0f);
    EXPECT_FLOAT_EQ(readBuffer3[49], 2.0f);
}

TEST_F(RingBufferTest, PartialWrite) {
    // Fill buffer
    std::vector<float> data(kBufferSize, 1.0f);
    buffer_.Write(data.data(), data.size());

    // Try to write more than available
    std::vector<float> extraData(100, 2.0f);
    size_t written = buffer_.Write(extraData.data(), extraData.size());
    EXPECT_EQ(written, 0);  // Buffer is full
}

TEST_F(RingBufferTest, PartialRead) {
    std::vector<float> data(50, 1.0f);
    buffer_.Write(data.data(), data.size());

    // Try to read more than available
    std::vector<float> readBuffer(100, 0.0f);
    size_t read = buffer_.Read(readBuffer.data(), readBuffer.size());
    EXPECT_EQ(read, 50);
}

TEST_F(RingBufferTest, Reset) {
    std::vector<float> data(100, 1.0f);
    buffer_.Write(data.data(), data.size());

    EXPECT_EQ(buffer_.AvailableForRead(), 100);

    buffer_.Reset();

    EXPECT_EQ(buffer_.AvailableForRead(), 0);
    EXPECT_EQ(buffer_.AvailableForWrite(), kBufferSize);
}

TEST_F(RingBufferTest, MultipleReadWrite) {
    // Multiple small writes and reads
    for (int i = 0; i < 10; ++i) {
        std::vector<float> writeData(10, static_cast<float>(i));
        buffer_.Write(writeData.data(), writeData.size());

        std::vector<float> readData(10, 0.0f);
        buffer_.Read(readData.data(), readData.size());

        EXPECT_FLOAT_EQ(readData[0], static_cast<float>(i));
        EXPECT_FLOAT_EQ(readData[9], static_cast<float>(i));
    }
}

TEST_F(RingBufferTest, ZeroSizeOperations) {
    std::vector<float> emptyData;  // Empty vector, size is 0
    std::vector<float> dummyData(1, 0.0f);  // Dummy data for null pointer test

    // Test with zero count - should not call memcpy with null
    if (emptyData.size() == 0) {
        // For empty data, we need a valid pointer (even with count=0)
        // But we're testing the count check, not the pointer
        EXPECT_EQ(buffer_.Write(dummyData.data(), 0), 0);
    }

    std::vector<float> readData(10, 0.0f);
    EXPECT_EQ(buffer_.Read(readData.data(), 0), 0);
}

TEST_F(RingBufferTest, FullBufferCapacity) {
    // Fill buffer completely
    std::vector<float> data(kBufferSize, 1.0f);
    size_t written = buffer_.Write(data.data(), data.size());
    EXPECT_EQ(written, kBufferSize);
    EXPECT_EQ(buffer_.AvailableForWrite(), 0);

    // Read all
    std::vector<float> readData(kBufferSize, 0.0f);
    size_t read = buffer_.Read(readData.data(), readData.size());
    EXPECT_EQ(read, kBufferSize);
    EXPECT_EQ(buffer_.AvailableForRead(), 0);
}

TEST_F(RingBufferTest, DataIntegrityAfterMultipleOperations) {
    // Test data that fits within buffer
    std::vector<float> testData;
    const size_t kDataSize = kBufferSize / 2;  // Use half buffer size
    for (size_t i = 0; i < kDataSize; ++i) {
        testData.push_back(static_cast<float>(i) * 0.001f);
    }

    // Write in chunks
    size_t written = 0;
    for (size_t i = 0; i < testData.size(); i += 50) {
        size_t chunkSize = std::min(size_t(50), testData.size() - i);
        written += buffer_.Write(&testData[i], chunkSize);
    }

    EXPECT_EQ(written, kDataSize);

    // Read in chunks
    std::vector<float> readData(kDataSize, 0.0f);
    size_t read = 0;
    for (size_t i = 0; i < readData.size(); i += 30) {
        size_t chunkSize = std::min(size_t(30), readData.size() - i);
        read += buffer_.Read(&readData[i], chunkSize);
    }

    EXPECT_EQ(read, kDataSize);

    // Verify data integrity
    for (size_t i = 0; i < testData.size(); ++i) {
        EXPECT_NEAR(testData[i], readData[i], 0.0001f);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
