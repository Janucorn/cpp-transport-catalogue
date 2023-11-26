#pragma once
/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */
#include "geo.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace transport_ctg {
	
struct Stop {
	std::string_view name;
	geo::Coordinates coordinates {0.0, 0.0};
};

struct Bus {
	std::string_view name;
	std::vector<Stop*> stops_ptr;
	bool is_roundtrip = false;
};

struct BusInfo {
	Bus* bus_ptr = nullptr;
	int stops_on_route = 0;
	int unique_stops = 0;
	double route_length = 0.;
	double coordinate_length = 0.;
};

}