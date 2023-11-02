#pragma once
#include "transport_catalogue.h"
#include "input_reader.h"

#include <vector>
#include <string>
#include <string_view>

namespace transport_ctg {
namespace statreader {
class StatReader {
public:
	explicit StatReader(input::Reader& input_reader);
	// вывод результата
	void PrintInfo();

private:
	// список запросов
	std::vector<input::detail::Query> stat_queries_;
	Catalogue& stat_catalogue_;
};
} // end of namespace transport_ctg::stat
} // end of namespace transport_ctg
