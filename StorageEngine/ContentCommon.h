#pragma once
#ifndef __CAVALIA_STORAGE_ENGINE_CONTENT_COMMON_H__
#define __CAVALIA_STORAGE_ENGINE_CONTENT_COMMON_H__

#include <cstdint>
#include <mutex>
#include "MetaTypes.h"

namespace Cavalia{
	namespace StorageEngine{

		static const size_t kRecycleLength = 100;

		struct RequestEntry{
			RequestEntry(){
				is_ready_ = NULL;
				data_ = NULL;
				next_ = NULL;
			}
			~RequestEntry(){
				is_ready_ = NULL;
				data_ = NULL;
				next_ = NULL;
			}
			uint64_t timestamp_;
			volatile bool *is_ready_;
			char **data_;
			RequestEntry *next_;
		};

		struct MvRequestEntry {
			MvRequestEntry() {
				is_ready_ = NULL;
				data_ = NULL;
				next_ = NULL;
			}
			~MvRequestEntry() {
				is_ready_ = NULL;
				data_ = NULL;
				next_ = NULL;
			}
			// for we need to modify a pointer's value
			uint64_t timestamp_;
			volatile bool *is_ready_;
			char **data_;
			MvRequestEntry* next_;
		};

		struct MvHistoryEntry {
			MvHistoryEntry() {
				data_ptr_ = NULL;
				next_ = NULL;
				prev_ = NULL;
			}
			uint64_t timestamp_;
			char* data_ptr_;
			MvHistoryEntry* next_;
			MvHistoryEntry* prev_;
		};

		//struct ConflictFlag{
		//	ConflictFlag() : in_conflict_(false), out_conflict_(false){}
		//	bool in_conflict_;
		//	bool out_conflict_;
		//	uint64_t commit_timestamp_;
		//	std::mutex mutex_;
		//};

		//struct SsiHistoryEntry {
		//	SsiHistoryEntry() {
		//		data_ptr_ = NULL;
		//		next_ = NULL;
		//		prev_ = NULL;
		//	}
		//	uint64_t timestamp_;
		//	char* data_ptr_;
		//	ConflictFlag* conflict_flag_;
		//	SsiHistoryEntry* next_;
		//	SsiHistoryEntry* prev_;
		//};

		struct LockEntry{
			LockEntry(){
				lock_ready_ = NULL;
				next_ = NULL;
			}
			~LockEntry(){
				lock_ready_ = NULL;
				next_ = NULL;
			}
			LockType lock_type_;
			uint64_t timestamp_;
			volatile bool* lock_ready_;
			LockEntry* next_;
		};
	}
}

#endif