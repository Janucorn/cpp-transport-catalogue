#include "stat_reader.h"

#include <iostream>
#include <iomanip>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace transport_ctg {
namespace statreader {

StatReader::StatReader(input::Reader& input_reader)
	: stat_catalogue_ (input_reader.GetCatalogue()) {}

void StatReader::PrintInfo() {
	using namespace std::literals;
	stat_queries_ = input::detail::SetQueryBase();
	for (const auto& query : stat_queries_) {

		std::cout << query.type << ' ' << query.text << ": "sv;
		// Обработка Bus-запросов
		if (query.type == "Bus"sv) {
			if (!stat_catalogue_.FindBus(query.text)) {
				std::cout << "not found"sv;
			}
			else {
				BusInfo bus = stat_catalogue_.GetBusInfo(query.text);
				// берем длину между остановками маршрута
				std::cout << std::setprecision(6)
					<< bus.stops_on_route << " stops on route, "sv
					<< bus.unique_stops << " unique stops, "sv
					<< bus.route_length << " route length, "sv
					<< static_cast<double>(bus.route_length * 1.0 / bus.coordinate_length) << " curvature"sv;
			}
		}
		// Обарботка Stop-запросов
		else {
			if (!stat_catalogue_.FindStop(query.text)) {
				std::cout << "not found"sv;
			}
			else {
				const auto& busnames = stat_catalogue_.GetBusesForStop(query.text);
				if (busnames.empty()) {
					std::cout << "no buses"sv;
				}
				else {
					std::cout << "buses"sv;
					for (const auto& busname : busnames) {
						std::cout << ' ' << busname;
					}
				}
			}
		}
		std::cout << std::endl;
	}
}

} // end of namespace transport_ctg::stat
} // end of namespace transport_ctg