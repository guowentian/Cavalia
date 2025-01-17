#if defined(DBX)
#include "TransactionManager.h"

namespace Cavalia {
	namespace StorageEngine {
		bool TransactionManager::InsertRecord(TxnContext *context, const size_t &table_id, const std::string &primary_key, SchemaRecord *record) {
			BEGIN_PHASE_MEASURE(thread_id_, INSERT_PHASE);
			Insertion *insertion = insertion_list_.NewInsertion();
			insertion->local_record_ = record;
			insertion->table_id_ = table_id;
			insertion->primary_key_ = primary_key;
			END_PHASE_MEASURE(thread_id_, INSERT_PHASE);
			return true;
		}

		bool TransactionManager::SelectRecordCC(TxnContext *context, const size_t &table_id, TableRecord *t_record, SchemaRecord *&s_record, const AccessType access_type, const size_t &access_id, bool is_key_access) {
			if (access_type == READ_ONLY) {
				Access *access = access_list_.NewAccess();
				access->access_type_ = READ_ONLY;
				access->access_record_ = t_record;
				access->timestamp_ = t_record->content_.GetTimestamp();
				s_record = t_record->record_;
				return true;
			}
			else if (access_type == READ_WRITE) {
				Access *access = access_list_.NewAccess();
				access->access_type_ = READ_WRITE;
				access->access_record_ = t_record;
				// copy data
				BEGIN_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
				char *local_data = allocator_->Alloc(t_record->record_->schema_ptr_->GetSchemaSize());
				SchemaRecord *local_record = (SchemaRecord*)allocator_->Alloc(sizeof(SchemaRecord));
				new(local_record)SchemaRecord(t_record->record_->schema_ptr_, local_data);
				END_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
				access->timestamp_ = t_record->content_.GetTimestamp();
				COMPILER_MEMORY_FENCE;
				local_record->CopyFrom(t_record->record_);
				access->local_record_ = local_record;
				// reset returned record.
				s_record = local_record;
				return true;
			}
			else {
				assert(access_type == DELETE_ONLY);
				Access *access = access_list_.NewAccess();
				access->access_type_ = DELETE_ONLY;
				access->access_record_ = t_record;
				s_record = t_record->record_;
				return true;
			}
		}

		bool TransactionManager::CommitTransaction(TxnContext *context, EventTuple *param, CharArray &ret_str){
			BEGIN_PHASE_MEASURE(thread_id_, COMMIT_PHASE);
			// step 1: acquire lock and validate
			uint64_t max_rw_ts = 0;
			bool is_success = true;
			// begin hardware transaction.
			rtm_lock_->Lock();
			for (size_t i = 0; i < access_list_.access_count_; ++i) {
				Access *access_ptr = access_list_.GetAccess(i);
				if (access_ptr->access_type_ == READ_ONLY) {
					// whether someone has changed the tuple after my read
					if (access_ptr->access_record_->content_.GetTimestamp() != access_ptr->timestamp_) {
						UPDATE_CC_ABORT_COUNT(thread_id_, context->txn_type_, access_ptr->access_record_->GetTableId());
						is_success = false;
						break;
					}
					if (access_ptr->timestamp_ > max_rw_ts){
						max_rw_ts = access_ptr->timestamp_;
					}
				}
				else if (access_ptr->access_type_ == READ_WRITE) {
					// whether someone has changed the tuple after my read
					if (access_ptr->access_record_->content_.GetTimestamp() != access_ptr->timestamp_) {
						UPDATE_CC_ABORT_COUNT(thread_id_, context->txn_type_, access_ptr->access_record_->GetTableId());
						is_success = false;
						break;
					}
					if (access_ptr->timestamp_ > max_rw_ts){
						max_rw_ts = access_ptr->timestamp_;
					}
				}
				else {
					assert(access_ptr->access_type_ == DELETE_ONLY);
				}
			}
			// step 2: if success, then overwrite and commit
			if (is_success == true) {
				BEGIN_CC_TS_ALLOC_TIME_MEASURE(thread_id_);
				uint64_t curr_ts = ScalableTimestamp::GetTimestamp();
				END_CC_TS_ALLOC_TIME_MEASURE(thread_id_);
				uint64_t commit_ts = GenerateTimestamp(curr_ts, max_rw_ts);

				for (size_t i = 0; i < access_list_.access_count_; ++i) {
					Access *access_ptr = access_list_.GetAccess(i);
					TableRecord *access_record = access_ptr->access_record_;
					if (access_ptr->access_type_ == READ_WRITE) {
						assert(commit_ts > access_ptr->timestamp_);
						access_record->record_->CopyFrom(access_ptr->local_record_);
						COMPILER_MEMORY_FENCE;
						access_record->content_.SetTimestamp(commit_ts);
					}
					else if (access_ptr->access_type_ == DELETE_ONLY) {
						assert(max_rw_ts >= access_ptr->timestamp_);
						assert(commit_ts > access_ptr->timestamp_);
						access_record->record_->is_visible_ = false;
						COMPILER_MEMORY_FENCE;
						access_record->content_.SetTimestamp(commit_ts);
					}
				}
				for (size_t i = 0; i < insertion_list_.insertion_count_; ++i) {
					Insertion *insertion_ptr = insertion_list_.GetInsertion(i);
					TableRecord *tb_record = new TableRecord(insertion_ptr->local_record_);
					tb_record->content_.SetTimestamp(commit_ts);
					insertion_ptr->insertion_record_ = tb_record;
					//storage_manager_->tables_[insertion_ptr->table_id_]->InsertRecord(insertion_ptr->primary_key_, insertion_ptr->insertion_record_);
				}
				// commit.
				// end hardware transaction.
				rtm_lock_->Unlock();
				// step 3: release locks and clean up.
				for (size_t i = 0; i < access_list_.access_count_; ++i) {
					Access *access_ptr = access_list_.GetAccess(i);
					if (access_ptr->access_type_ == READ_WRITE) {
						BEGIN_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
						allocator_->Free(access_ptr->local_record_->data_ptr_);
						access_ptr->local_record_->~SchemaRecord();
						allocator_->Free((char*)access_ptr->local_record_);
						END_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
					}
				}
				for (size_t i = 0; i < insertion_list_.insertion_count_; ++i) {
					Insertion *insertion_ptr = insertion_list_.GetInsertion(i);
				}
			}
			// if failed.
			else {
				// end hardware transaction.
				rtm_lock_->Unlock();
				// step 3: release locks and clean up.
				for (size_t i = 0; i < access_list_.access_count_; ++i) {
					Access *access_ptr = access_list_.GetAccess(i);
					if (access_ptr->access_type_ == READ_WRITE) {
						BEGIN_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
						allocator_->Free(access_ptr->local_record_->data_ptr_);
						access_ptr->local_record_->~SchemaRecord();
						allocator_->Free((char*)access_ptr->local_record_);
						END_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
					}
				}
				for (size_t i = 0; i < insertion_list_.insertion_count_; ++i) {
					Insertion *insertion_ptr = insertion_list_.GetInsertion(i);
					BEGIN_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
					allocator_->Free(insertion_ptr->local_record_->data_ptr_);
					insertion_ptr->local_record_->~SchemaRecord();
					allocator_->Free((char*)insertion_ptr->local_record_);
					END_CC_MEM_ALLOC_TIME_MEASURE(thread_id_);
				}
			}
			assert(insertion_list_.insertion_count_ <= kMaxAccessNum);
			assert(access_list_.access_count_ <= kMaxAccessNum);
			insertion_list_.Clear();
			access_list_.Clear();
			END_PHASE_MEASURE(thread_id_, COMMIT_PHASE);
			return is_success;
		}

		void TransactionManager::AbortTransaction() {
			assert(false);
		}
	}
}

#endif
