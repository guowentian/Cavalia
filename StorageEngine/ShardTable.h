#pragma once
#ifndef __CAVALIA_STORAGE_ENGINE_SHARD_TABLE_H__
#define __CAVALIA_STORAGE_ENGINE_SHARD_TABLE_H__

#include "BaseTable.h"
#include "StdUnorderedIndex.h"
#include "StdUnorderedIndexMT.h"
#include "StdOrderedIndex.h"
#include "StdOrderedIndexMT.h"
#if defined(CUCKOO_INDEX)
#include "CuckooIndex.h"
#endif

namespace Cavalia {
	namespace StorageEngine {
		class ShardTable : public BaseTable {
		public:
			ShardTable(const RecordSchema * const schema_ptr, const TableLocation &table_location, bool is_thread_safe) : BaseTable(schema_ptr), partition_count_(table_location.GetPartitionCount()){
				table_location_ = table_location;
				if (is_thread_safe == true){
#if defined(CUCKOO_INDEX)
					primary_index_ = new BaseUnorderedIndex*[partition_count_];
#else
					primary_index_ = new BaseUnorderedIndex*[partition_count_];
#endif			
					secondary_indexes_ = new BaseOrderedIndex**[partition_count_];
					for (size_t i = 0; i < partition_count_; ++i){
						size_t numa_node_id = table_location.node_ids_.at(i);
#if defined(CUCKOO_INDEX)
						CuckooIndex *p_index = (CuckooIndex*)MemAllocator::AllocNode(sizeof(CuckooIndex), numa_node_id);
						new(p_index)CuckooIndex();
						primary_index_[i] = p_index;
#else
						StdUnorderedIndexMT *p_index = (StdUnorderedIndexMT*)MemAllocator::AllocNode(sizeof(StdUnorderedIndexMT), numa_node_id);
						new(p_index)StdUnorderedIndexMT();
						primary_index_[i] = p_index;
#endif				
						BaseOrderedIndex **s_indexes = (BaseOrderedIndex**)MemAllocator::AllocNode(sizeof(void*)*secondary_count_, numa_node_id);
						memset(s_indexes, 0, sizeof(void*)*secondary_count_);
						secondary_indexes_[i] = s_indexes;
						for (size_t j = 0; j < secondary_count_; ++j){
							StdOrderedIndexMT *s_index = (StdOrderedIndexMT*)MemAllocator::AllocNode(sizeof(StdOrderedIndexMT), numa_node_id);
							new(s_index)StdOrderedIndexMT();
							secondary_indexes_[i][j] = s_index;
						}
					}
				}
				else{
					primary_index_ = new BaseUnorderedIndex*[partition_count_];
					secondary_indexes_ = new BaseOrderedIndex**[partition_count_];
					for (size_t i = 0; i < partition_count_; ++i){
						size_t numa_node_id = table_location.node_ids_.at(i);
						StdUnorderedIndex *p_index = (StdUnorderedIndex*)MemAllocator::AllocNode(sizeof(StdUnorderedIndex), numa_node_id);
						new(p_index)StdUnorderedIndex();
						primary_index_[i] = p_index;
						BaseOrderedIndex **s_indexes = (BaseOrderedIndex**)MemAllocator::AllocNode(sizeof(void*)*secondary_count_, numa_node_id);
						memset(s_indexes, 0, sizeof(void*)*secondary_count_);
						secondary_indexes_[i] = s_indexes;
						for (size_t j = 0; j < secondary_count_; ++j){
							StdOrderedIndex *s_index = (StdOrderedIndex*)MemAllocator::AllocNode(sizeof(StdOrderedIndex), numa_node_id);
							new(s_index)StdOrderedIndex();
							secondary_indexes_[i][j] = s_index;
						}
					}
				}
			}

			virtual ~ShardTable() {
				//// records in the table is released by primary index.
				//for (size_t i = 0; i < partition_count_; ++i){
				//	MemAllocator::FreeNode((char*)(primary_index_[i]), sizeof(StdUnorderedIndex));
				//	primary_index_[i] = NULL;
				//	for (size_t j = 0; j < secondary_count_; ++j) {
				//		MemAllocator::FreeNode((char*)(secondary_indexes_[i][j]), sizeof(MultiIndex));
				//		secondary_indexes_[i][j] = NULL;
				//	}
				//	MemAllocator::FreeNode((char*)(secondary_indexes_[i]), sizeof(void*)*secondary_count_);
				//	secondary_indexes_[i] = NULL;
				//}
				//delete[] primary_index_;
				//primary_index_ = NULL;
				//delete[] secondary_indexes_;
				//secondary_indexes_ = NULL;
			}

			// get the number of records in this table.
			virtual const size_t GetTableSize() const {
				size_t size = 0;
				for (size_t i = 0; i < partition_count_; ++i){
					size += primary_index_[i]->GetSize();
				}
				return size;
			}

			///////////////////INSERT//////////////////
			virtual void InsertRecord(TableRecord *record) {
				InsertRecord(record->record_->GetPrimaryKey(), record);
			}

			virtual void InsertRecord(const std::string &primary_key, TableRecord *record){
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = GetPartitionId(record->record_);
				}
				if(record->record_->schema_ptr_->GetTableId()==0){
				}
				primary_index_[part_id]->InsertRecord(primary_key, record);
				// build secondary index here
				for (size_t i = 0; i < secondary_count_; ++i) {
					secondary_indexes_[part_id][i]->InsertRecord(record->record_->GetSecondaryKey(i), record);
				}
			}

			// for replication
			virtual void InsertRecord(const size_t &part_id, TableRecord *record){
				primary_index_[part_id]->InsertRecord(record->record_->GetPrimaryKey(), record);
				for (size_t i = 0; i < secondary_count_; ++i) {
					secondary_indexes_[part_id][i]->InsertRecord(record->record_->GetSecondaryKey(i), record);
				}
			}

			virtual void DeleteRecord(TableRecord *record){
				DeleteRecord(record->record_->GetPrimaryKey(), record);
			}

			virtual void DeleteRecord(const std::string &primary_key, TableRecord *record) {
				size_t part_id = 0;
				if (partition_count_ != 1) {
					part_id = GetPartitionId(record->record_);
				}
				primary_index_[part_id]->DeleteRecord(primary_key);
				for (size_t i = 0; i < secondary_count_; ++i) {
					secondary_indexes_[part_id][i]->DeleteRecord(primary_key);
				}
			}
			///////////////////NEW API//////////////////
			virtual void SelectKeyRecord(const std::string &primary_key, TableRecord *&record) const {
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = schema_ptr_->GetPartitionHashcode(primary_key) % partition_count_;
				}
				record = primary_index_[part_id]->SearchRecord(primary_key);
			}

			virtual void SelectKeyRecord(const size_t &part_id, const std::string &primary_key, TableRecord *&record) const {
				record = primary_index_[part_id]->SearchRecord(primary_key);
			}

			virtual void SelectRecord(const size_t &idx_id, const std::string &key, TableRecord *&record) const {
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = schema_ptr_->GetPartitionHashcode(idx_id, key) % partition_count_;
				}
				record = secondary_indexes_[part_id][idx_id]->SearchRecord(key);
			}
			
			virtual void SelectRecord(const size_t &part_id, const size_t &idx_id, const std::string &key, TableRecord *&record) const {
				record = secondary_indexes_[part_id][idx_id]->SearchRecord(key);
			}
			
			virtual void SelectRecords(const size_t &idx_id, const std::string &key, TableRecords *records) const {
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = schema_ptr_->GetPartitionHashcode(idx_id, key) % partition_count_;
				}
				secondary_indexes_[part_id][idx_id]->SearchRecords(key, records);
			}
			
			virtual void SelectRecords(const size_t &part_id, const size_t &idx_id, const std::string &key, TableRecords *records) const {
				secondary_indexes_[part_id][idx_id]->SearchRecords(key, records);
			}

			virtual void SelectUpperRecords(const size_t &idx_id, const std::string &key, TableRecords *records) const {
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = schema_ptr_->GetPartitionHashcode(idx_id, key) % partition_count_;
				}
				secondary_indexes_[part_id][idx_id]->SearchUpperRecords(key, records);
			}
			
			virtual void SelectUpperRecords(const size_t part_id, const size_t &idx_id, const std::string &key, TableRecords *records) const {
				secondary_indexes_[part_id][idx_id]->SearchUpperRecords(key, records);
			}
			
			virtual void SelectLowerRecords(const size_t &idx_id, const std::string &key, TableRecords *records) const {
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = schema_ptr_->GetPartitionHashcode(idx_id, key) % partition_count_;
				}
				secondary_indexes_[part_id][idx_id]->SearchLowerRecords(key, records);
			}
			
			virtual void SelectLowerRecords(const size_t part_id, const size_t &idx_id, const std::string &key, TableRecords *records) const {
				secondary_indexes_[part_id][idx_id]->SearchLowerRecords(key, records);
			}
			
			virtual void SelectRangeRecords(const size_t &idx_id, const std::string &lower_key, std::string &upper_key, TableRecords *records) const {
				size_t part_id = 0;
				if (partition_count_ != 1){
					part_id = schema_ptr_->GetPartitionHashcode(idx_id, lower_key) % partition_count_;
				}
				secondary_indexes_[part_id][idx_id]->SearchRangeRecords(lower_key, upper_key, records);
			}
			
			virtual void SelectRangeRecords(const size_t part_id, const size_t &idx_id, const std::string &lower_key, const std::string &upper_key, TableRecords *records) const {
				secondary_indexes_[part_id][idx_id]->SearchRangeRecords(lower_key, upper_key, records);
			}


			virtual void SaveCheckpoint(std::ofstream &out_stream) {
				size_t record_size = schema_ptr_->GetSchemaSize();
				for (size_t partition = 0; partition < partition_count_; ++partition) {
					primary_index_[partition]->SaveCheckpoint(out_stream, record_size);
				}
				out_stream.flush();
			}

			virtual void ReloadCheckpoint(std::ifstream &in_stream) {
				size_t record_size = schema_ptr_->GetSchemaSize();
				SchemaRecord *tmp_record = new SchemaRecord(schema_ptr_, NULL);
				char *tmp_entry = new char[record_size];

				for (size_t part_id = 0; part_id < partition_count_; ++part_id){
					size_t numa_node_id = table_location_.node_ids_.at(part_id);

					in_stream.seekg(0, std::ios::end);
					size_t file_size = static_cast<size_t>(in_stream.tellg());
					in_stream.seekg(0, std::ios::beg);
					size_t file_pos = 0;
					while (file_pos < file_size) {
						in_stream.read(tmp_entry, record_size);
						tmp_record->data_ptr_ = tmp_entry;
						if (IsReplication() == true){
							char *entry = MemAllocator::AllocNode(record_size, numa_node_id);
							memcpy(entry, tmp_entry, record_size);
							SchemaRecord *s_record = (SchemaRecord*)MemAllocator::AllocNode(sizeof(SchemaRecord), numa_node_id);
							new(s_record)SchemaRecord(schema_ptr_, entry);
							TableRecord *t_record = (TableRecord*)MemAllocator::AllocNode(sizeof(TableRecord), numa_node_id);
							new(t_record)TableRecord(s_record);
							InsertRecord(part_id, t_record);
						}
						else{
							size_t partition_id = GetPartitionId(tmp_record);
							if (partition_id == part_id){
								char *entry = MemAllocator::AllocNode(record_size, numa_node_id);
								memcpy(entry, tmp_entry, record_size);
								SchemaRecord *s_record = (SchemaRecord*)MemAllocator::AllocNode(sizeof(SchemaRecord), numa_node_id);
								new(s_record)SchemaRecord(schema_ptr_, entry);
								TableRecord *t_record = (TableRecord*)MemAllocator::AllocNode(sizeof(TableRecord), numa_node_id);
								new(t_record)TableRecord(s_record);
								InsertRecord(t_record);
							}
						}
						file_pos += record_size;
					}
				}

				delete[] tmp_entry;
				tmp_entry = NULL;
				delete tmp_record;
				tmp_record = NULL;
			}

			// default partition: hash partition.
			virtual size_t GetPartitionId(SchemaRecord *record) const {
				return record->GetPartitionHashcode() % partition_count_;
			}

			virtual bool IsReplication() const {
				return false;
			}

		private:
			ShardTable(const ShardTable&);
			ShardTable & operator=(const ShardTable&);

		protected:
			const size_t partition_count_;
			TableLocation table_location_;
			BaseUnorderedIndex **primary_index_;
			BaseOrderedIndex ***secondary_indexes_;
		};
	}
}

#endif
