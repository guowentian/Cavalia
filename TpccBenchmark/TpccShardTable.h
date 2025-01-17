#pragma once
#ifndef __CAVALIA_STORAGE_ENGINE_TPCC_SHARD_TABLE_H__
#define __CAVALIA_STORAGE_ENGINE_TPCC_SHARD_TABLE_H__

#include <ShardTable.h>
#include "TpccMeta.h"

namespace Cavalia {
	namespace Benchmark {
		namespace Tpcc{
			using namespace Cavalia::StorageEngine;
			class TpccShardTable : public ShardTable {
			public:
				TpccShardTable(const RecordSchema * const schema_ptr, const TableLocation &table_location, bool is_thread_safe) : ShardTable(schema_ptr, table_location, is_thread_safe){}
				virtual ~TpccShardTable() {}

				// partition by warehouse_id.
				// we temporarily place item table in the first (0) numa node.
				virtual size_t GetPartitionId(SchemaRecord *record) const {
					size_t w_id = 0;
					switch (schema_ptr_->GetTableId()) {
					case ITEM_TABLE_ID:
						w_id = 1;
						break;
					case WAREHOUSE_TABLE_ID:
						w_id = *(int*)(record->GetColumn(0));
						break;
					case DISTRICT_TABLE_ID:
						w_id = *(int*)(record->GetColumn(1));
						break;
					case CUSTOMER_TABLE_ID:
						w_id = *(int*)(record->GetColumn(2));
						break;
					case ORDER_TABLE_ID:
						w_id = *(int*)(record->GetColumn(3));
						break;
					case NEW_ORDER_TABLE_ID:
						w_id = *(int*)(record->GetColumn(2));
						break;
					case ORDER_LINE_TABLE_ID:
						w_id = *(int*)(record->GetColumn(2));
						break;
					case HISTORY_TABLE_ID:
						w_id = *(int*)(record->GetColumn(4));
						break;
					case STOCK_TABLE_ID:
						w_id = *(int*)(record->GetColumn(1));
						break;
					case DISTRICT_NEW_ORDER_TABLE_ID:
						w_id = *(int*)(record->GetColumn(1));
						break;
					default:
						assert(false);
						break;
					}
					return (w_id - 1) % partition_count_;
				}

				virtual bool IsReplication() const {
					if (schema_ptr_->GetTableId() == ITEM_TABLE_ID){
						return true;
					}
					return false;
				}

			private:
				TpccShardTable(const TpccShardTable&);
				TpccShardTable & operator=(const TpccShardTable&);
			};
		}
	}
}

#endif