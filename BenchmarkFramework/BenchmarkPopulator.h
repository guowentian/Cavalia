#pragma once
#ifndef __CAVALIA_BENCHMARK_FRAMEWORK_BENCHMARK_POPULATOR_H__
#define __CAVALIA_BENCHMARK_FRAMEWORK_BENCHMARK_POPULATOR_H__


#include <TimeMeasurer.h>
#include <BaseStorageManager.h>

namespace Cavalia{
	namespace Benchmark{
		using namespace Cavalia::StorageEngine;
		class BenchmarkPopulator{
		public:
			BenchmarkPopulator(BaseStorageManager *storage_manager) : storage_manager_(storage_manager){}
			virtual ~BenchmarkPopulator(){}

			void Start(){
				TimeMeasurer timer;
				timer.StartTimer();
				StartPopulate();
				timer.EndTimer();
				std::cout << "populate elapsed time=" << timer.GetElapsedMilliSeconds() << "ms" << std::endl;
			}

			virtual void StartPopulate() = 0;

		private:
			BenchmarkPopulator(const BenchmarkPopulator &);
			BenchmarkPopulator& operator=(const BenchmarkPopulator &);

		protected:
			BaseStorageManager *const storage_manager_;
		};
	}
}

#endif
