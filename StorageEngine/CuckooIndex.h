#pragma once
#ifndef __CAVALIA_STORAGE_ENGINE_CUCKOO_INDEX_H__
#define __CAVALIA_STORAGE_ENGINE_CUCKOO_INDEX_H__

#include <libcuckoo/cuckoohash_map.hh>
#include <libcuckoo/city_hasher.hh>
#include "BaseUnorderedIndex.h"

namespace Cavalia{
	namespace StorageEngine {
		class CuckooIndex : public BaseUnorderedIndex{
			typedef cuckoohash_map<std::string, TableRecord*, CityHasher<std::string>> CuckooMap;
		public:
			CuckooIndex(){}
			virtual ~CuckooIndex(){}

			virtual void InsertRecord(const std::string &key, TableRecord *record){
				hash_index_[key] = record;
			}

			virtual void DeleteRecord(const std::string &key){
				//hash_index_.unsafe_erase(key);
			}

			virtual TableRecord* SearchRecord(const std::string &key){
				TableRecord *record;
				if (hash_index_.find(key, record) == true){
					return record;
				}
				else{
					return NULL;
				}
			}

			virtual size_t GetSize() const {
				return hash_index_.size();
			}

			virtual void SaveCheckpoint(std::ofstream &out_stream, const size_t &record_size){
				auto lt = hash_index_.lock_table();
				for (const auto &it : lt) {
					out_stream.write(it.second->record_->data_ptr_, record_size);
				}
				out_stream.flush();
			}

		private:
			CuckooIndex(const CuckooIndex &);
			CuckooIndex& operator=(const CuckooIndex &);

		private:
			CuckooMap hash_index_;
		};
	}
}

#endif
