#pragma once

#include "../DaxaCore.hpp"

#include <mutex>
#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

#include "GPUHandles.hpp"

namespace daxa {
	constexpr inline size_t STAGING_BUFFER_POOL_BUFFER_SIZE = 1 << 26;	// 67 MB, about 4k texture size

	namespace {
		struct StagingBufferPoolSharedData {
			std::mutex mut = {};
			std::vector<BufferHandle> pool = {};
		};
	}

	class StagingBuffer {
	public:
		StagingBuffer(BufferHandle& handle, std::weak_ptr<StagingBufferPoolSharedData> pool);
		StagingBuffer(StagingBuffer&&) noexcept													= default;
		StagingBuffer(StagingBuffer const&) 													= delete;
		StagingBuffer& operator=(StagingBuffer&&) noexcept;
		StagingBuffer& operator=(StagingBuffer const&) 											= delete;
		~StagingBuffer();

		size_t getLeftOverSize() const { return STAGING_BUFFER_POOL_BUFFER_SIZE - usedUpSize; }

		size_t usedUpSize = {};
		std::optional<BufferHandle> buffer = {};
	private:
		void cleanup();
		std::weak_ptr<StagingBufferPoolSharedData> sharedData = {};
	};

	class StagingBufferPool {
	public:
		StagingBufferPool(
			std::shared_ptr<void> deviceBackend, 
			size_t size = STAGING_BUFFER_POOL_BUFFER_SIZE, 
			MemoryType memoryType = MemoryType::CPU_TO_GPU
		);
		~StagingBufferPool();

		StagingBuffer getStagingBuffer();
	private:
		std::shared_ptr<void> deviceBackend = {};
		std::shared_ptr<StagingBufferPoolSharedData> sharedData = {};
		size_t size;
		MemoryType memoryType;
	};
}