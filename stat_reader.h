#pragma once
#include "transport_catalogue.h"
#include "input_reader.h"

#include <iostream>
#include <vector>

namespace transport_ctg {
namespace statreader {
class StatReader {
public:
	explicit StatReader(input::Reader& input_reader);
	// вывод результата
	void PrintInfo(std::ostream& out, std::istream& in);

private:
	// список запросов
	std::vector<input::detail::Query> stat_queries_;
	Catalogue& stat_catalogue_;
};
} // end of namespace transport_ctg::statreader
} // end of namespace transport_ctg
