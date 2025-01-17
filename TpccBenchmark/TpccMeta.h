#pragma once
#ifndef __CAVALIA_TPCC_BENCHMARK_TPCC_META_H__
#define __CAVALIA_TPCC_BENCHMARK_TPCC_META_H__

namespace Cavalia{
	namespace Benchmark{
		namespace Tpcc{
			enum TupleType : size_t { DELIVERY, NEW_ORDER, PAYMENT, ORDER_STATUS, STOCK_LEVEL, kProcedureCount };
			
			enum SliceType : size_t { 
				S_SCAN, S_WAREHOUSE_7, S_WAREHOUSE_8, S_DISTRICT_9, S_DISTRICT_10, S_CUSTOMER_15, S_CUSTOMER_16,
				S_HISTORY, 
				S_ITEM, S_STOCK, 
				S_NEW_ORDER, S_ORDER, S_ORDER_LINE, 
				S_SHUFFLE, kSliceCount
			};

			enum ChopType : size_t { C_ITEM, C_STOCK, C_HISTORY, C_OTHERS, kChopCount };

			enum TableType : size_t { ITEM_TABLE_ID, STOCK_TABLE_ID, WAREHOUSE_TABLE_ID, DISTRICT_TABLE_ID, DISTRICT_NEW_ORDER_TABLE_ID, NEW_ORDER_TABLE_ID, ORDER_TABLE_ID, ORDER_LINE_TABLE_ID, CUSTOMER_TABLE_ID, HISTORY_TABLE_ID, kTableCount };
		}
	}
}

#endif
