#ifndef PAYLOAD_EXTRACT_BUFFER_H
#define PAYLOAD_EXTRACT_BUFFER_H

#include <algorithm>
#include <memory>

namespace skkk {
	template<typename T>
	class Buffer {
		uint64_t size_ = 0;
		std::unique_ptr<T[]> data_{};

		void allocate(uint64_t size) {
			this->size_ = size;
			this->data_ = std::make_unique<T[]>(size);
		}

		void setValue(T value) {
			std::fill(data_.get(), data_.get() + size_, value);
		}

		public:
			Buffer() = default;

			explicit Buffer(uint64_t size) {
				allocate(size);
			}

			explicit Buffer(T value, uint64_t size) {
				allocate(size);
				setValue(value);
			}

			Buffer(const Buffer &other) = delete;

			Buffer &operator=(const Buffer &other) = delete;

			Buffer(Buffer &&other)
				noexcept : size_(other.size_),
				           data_(std::move(other.data_)) {
				other.size_ = 0;
			}

			Buffer &operator=(Buffer &&other) noexcept {
				if (this == &other)
					return *this;
				size_ = other.size_;
				data_ = std::move(other.data_);
				other.size_ = 0;
				return *this;
			}

			bool operator==(std::nullptr_t) noexcept {
				return !data_;
			}

			explicit operator bool() const noexcept {
				return data_ != nullptr;
			}

			void reserve(uint64_t size) {
				allocate(size);
			}

			void reserve(T value, uint64_t size) {
				allocate(size);
				setValue(value);
			}

			void resize(uint64_t size) {
				allocate(size);
			}

			void resize(T value, uint64_t size) {
				allocate(size);
				setValue(value);
			}

			T *get() {
				return data_.get();
			}
	};
}

#endif //PAYLOAD_EXTRACT_BUFFER_H
