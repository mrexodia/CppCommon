//
// Created by Ivan Shynkarenka on 16.01.2016.
//

#include "cppbenchmark.h"

#include <functional>
#include <thread>

#include "threads/wf_ring_queue.h"

const int64_t items_to_produce = 100000000;

template<typename T, int64_t N>
void produce_consume(CppBenchmark::Context& context, const std::function<void()>& wait_strategy)
{
    int64_t crc = 0;

    // Create wait-free ring queue
    CppCommon::WFRingQueue<T> queue(N);

    // Start consumer thread
    auto consumer = std::thread([&queue, &wait_strategy, &crc]()
    {
        for (int64_t i = 0; i < items_to_produce; ++i)
        {
            // Dequeue using the given waiting strategy
            T item;
            while (!queue.Dequeue(item))
                wait_strategy();

            // Consume the item
            crc += item;
        }
    });

    // Start producer thread
    auto producer = std::thread([&queue, &wait_strategy, &consumer]()
    {
        for (int64_t i = 0; i < items_to_produce; ++i)
        {
            // Enqueue using the given waiting strategy
            while (!queue.Enqueue((T)i))
                wait_strategy();
        }

        // Wait for the consumer thread
        consumer.join();
    });

    // Wait for the producer thread
    producer.join();

    // Update benchmark metrics
    context.metrics().AddIterations(items_to_produce - 1);
    context.metrics().AddItems(items_to_produce);
    context.metrics().AddBytes(items_to_produce * sizeof(T));
    context.metrics().SetCustom("RingQueue.capacity", N);
    context.metrics().SetCustom("CRC", crc);
}

BENCHMARK("RingQueue-SpinWait")
{
    produce_consume<int, 1048576>(context, []{});
}

BENCHMARK("RingQueue-YieldWait")
{
    produce_consume<int, 1048576>(context, []{ std::this_thread::yield(); });
}

BENCHMARK_MAIN()
