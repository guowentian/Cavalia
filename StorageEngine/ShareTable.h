#pragma once
#ifndef __CAVALIA_STORAGE_ENGINE_SHARE_TABLE_H__
#define __CAVALIA_STORAGE_ENGINE_SHARE_TABLE_H__

#include "BaseTable.h"
#include "StdUnorderedIndex.h"
#include "StdUnorderedIndexMT.h"
#include "StdOrderedIndex.h"
#include "StdOrderedIndexMT.h"
#if defined(CUCKOO_INDEX)
#include "CuckooIndex.h"
#endif

namespace Cavalia{
	namespace StorageEngine{
		class ShareTable : public BaseTable {
		public:
			ShareTable(const RecordSchema * const schema_ptr, bool is_thread_safe) : BaseTable(schema_ptr), partition_count_(kTablePartitionNum){
				if (is_thread_safe == true){
#if defined(CUCKOO_INDEX)
					primary_index_ = new CuckooIndex();
#else
					primary_index_ = new StdUnorderedIndexMT();
#endif
					secondary_indexes_ = new BaseOrderedIndex*[secondary_count_];
					for (size_t i = 0; i < secondary_count_; ++i){
						secondary_indexes_[i] = new StdOrderedIndexMT();
					}
				}
				else{
					primary_index_ = new StdUnorderedIndex();
					secondary_indexes_ = new BaseOrderedIndex*[secondary_count_];
					for (size_t i = 0; i < secondary_count_; ++i){
						secondary_indexes_[i] = new StdOrderedIndex();
					}
				}
			}

			virtual ~ShareTable(){
				// records in the table is released by primary index.
				delete primary_index_;
				primary_index_ = NULL;
				for (size_t i = 0; i < secondary_count_; ++i){
					delete secondary_indexes_[i];
					secondary_indexes_[i] = NULL;
				}
				delete[] secondary_indexes_;
				secondary_indexes_ = NULL;
			}

			// get the number of records in this table.
			virtual const size_t GetTableSize() const {
				return primary_index_->GetSize();
			}

			///////////////////INSERT//////////////////
			virtual void InsertRecord(TableRecord *record){
				primary_index_->InsertRecord(record->record_->GetPrimaryKey(), record);
				// build secondary index here
				for (size_t i = 0; i < secondary_count_; ++i){
					secondary_indexes_[i]->InsertRecord(record->record_->GetSecondaryKey(i), record);
				}
			}

			virtual void InsertRecord(const std::string &primary_key, TableRecord *record){
				primary_index_->InsertRecord(primary_key, record);
				// build secondary index here
				for (size_t i = 0; i < secondary_count_; ++i){
					secondary_indexes_[i]->InsertRecord(record->record_->GetSecondaryKey(i), record);
				}
			}
			
			/////////////////////DELETE//////////////////
			virtual void DeleteRecord(TableRecord *record){
				primary_index_->DeleteRecord(record->record_->GetPrimaryKey());
				// update secondary index here
				for (size_t i = 0; i < secondary_count_; ++i){
					secondary_indexes_[i]->DeleteRecord(record->record_->GetSecondaryKey(i));
				}
				// ========================IMPORTANT========================
				// TODO: we do not delete data, potential memory leak problem!
				//delete record;
				//record = NULL;
			}

			virtual void DeleteRecord(const std::string &primary_key, TableRecord *record) {
				primary_index_->DeleteRecord(primary_key);
				for (size_t i = 0; i < secondary_count_; ++i) {
					secondary_indexes_[i]->DeleteRecord(record->record_->GetSecondaryKey(i));
				}
			}

			///////////////////NEW API//////////////////
			virtual void SelectKeyRecord(const std::string &primary_key, TableRecord *&record) const {
				record = primary_index_->SearchRecord(primary_key);
			}

			virtual void SelectKeyRecord(const size_t &part_id, const std::string &primary_key, TableRecord *&record) const {
				assert(false);
			}

			virtual void SelectRecord(const size_t &idx_id, const std::string &key, TableRecord *&record) const {
				record = secondary_indexes_[idx_id]->SearchRecord(key);
			}

			virtual void SelectRecord(const size_t &part_id, const size_t &idx_id, const std::string &key, TableRecord *&record) const {
				assert(false);
			}

			virtual void SelectRecords(const size_t &idx_id, const std::string &key, TableRecords *records) const {
				secondary_indexes_[idx_id]->SearchRecords(key, records);
			}

			virtual void SelectRecords(const size_t &part_id, const size_t &idx_id, const std::string &key, TableRecords *records) const {
				assert(false);
			}

			virtual void SelectUpperRecords(const size_t &idx_id, const std::string &key, TableRecords *records) const {
				secondary_indexes_[idx_id]->SearchUpperRecords(key, records);
			}

			virtual void SelectUpperRecords(const size_t part_id, const size_t &idx_id, const std::string &key, TableRecords *records) const {
				assert(false);
			}

			virtual void SelectLowerRecords(const size_t &idx_id, const std::string &key, TableRecords *records) const {
				secondary_indexes_[idx_id]->SearchLowerRecords(key, records);
			}

			virtual void SelectLowerRecords(const size_t part_id, const size_t &idx_id, const std::string &key, TableRecords *records) const {
				assert(false);
			}

			virtual void SelectRangeRecords(const size_t &idx_id, const std::string &lower_key, std::string &upper_key, TableRecords *records) const {
				secondary_indexes_[idx_id]->SearchRangeRecords(lower_key, upper_key, records);
			}

			virtual void SelectRangeRecords(const size_t part_id, const size_t &idx_id, const std::string &lower_key, const std::string &upper_key, TableRecords *records) const {
				assert(false);
			}


			virtual void SaveCheckpoint(std::ofstream &out_stream){
				size_t record_size = schema_ptr_->GetSchemaSize();
				primary_index_->SaveCheckpoint(out_stream, record_size);
			}

			virtual void ReloadCheckpoint(std::ifstream &in_stream){
				size_t record_size = schema_ptr_->GetSchemaSize();
				in_stream.seekg(0, std::ios::end);
				size_t file_size = static_cast<size_t>(in_stream.tellg());
				in_stream.seekg(0, std::ios::beg);
				size_t file_pos = 0;
				while (file_pos < file_size){
					char *entry = new char[record_size];
					in_stream.read(entry, record_size);
					InsertRecord(new TableRecord(new SchemaRecord(schema_ptr_, entry)));
					file_pos += record_size;
				}
			}

		private:
			ShareTable(const ShareTable&);
			ShareTable & operator=(const ShareTable&);

		protected:
			const size_t partition_count_;
			BaseUnorderedIndex *primary_index_;
			BaseOrderedIndex **secondary_indexes_;
		};
	}
}

#endif
