#pragma once
#ifndef __CAVALIA_STORAGE_ENGINE_TV_LOCK_CONTENT_H__
#define __CAVALIA_STORAGE_ENGINE_TV_LOCK_CONTENT_H__

#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>

namespace Cavalia {
	namespace StorageEngine {
		class TvLockContent {
		public:
			TvLockContent() : read_count_(0), is_writing_(false), is_certifying_(false){
				memset(&spinlock_, 0, sizeof(spinlock_));
			}
			~TvLockContent(){}

			bool AcquireReadLock() {
				bool rt = true;
				spinlock_.lock();
				if (is_certifying_ == true){
					rt = false;
				}
				else{
					++read_count_;
				}
				spinlock_.unlock();
				return rt;
			}

			void ReleaseReadLock() {
				spinlock_.lock();
				assert(read_count_ > 0);
				--read_count_;
				spinlock_.unlock();
			}

			bool AcquireWriteLock() {
				bool rt = true;
				spinlock_.lock();
				if (is_writing_ == true || is_certifying_ == true){
					rt = false;
				}
				else{
					is_writing_ = true;
				}
				spinlock_.unlock();
				return rt;
			}

			void ReleaseWriteLock() {
				spinlock_.lock();
				assert(is_writing_ == true);
				is_writing_ = false;
				spinlock_.unlock();
			}

			bool AcquireCertifyLock() {
				bool rt = true;
				spinlock_.lock();
				assert(is_writing_ == true);
				assert(is_certifying_ == false);
				if (read_count_ != 0){
					rt = false;
				}
				else{
					is_writing_ = false;
					is_certifying_ = true;
				}
				spinlock_.unlock();
				return rt;
			}

			void ReleaseCertifyLock() {
				spinlock_.lock();
				assert(is_certifying_ == true);
				is_certifying_ = false;
				spinlock_.unlock();
			}

		private:
			boost::detail::spinlock spinlock_;
			size_t read_count_;
			bool is_writing_;
			bool is_certifying_;
		};
	}
}

#endif