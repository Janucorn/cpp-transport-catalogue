#include "transport_catalogue.h"
#include "geo.h"
#include "domain.h"

#include <functional>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace transport_ctg {
using namespace std::literals;

void Catalogue::AddStop(Stop stop) {
	stops_.push_back(stop);
	stopname_to_stop_.insert({ stop.name, &stops_.back() });

	static std::set<std::string_view> empty_bus;
	stop_to_buses_.insert({ stopname_to_stop_[stop.name], empty_bus });
}

Stop* Catalogue::FindStop(const std::string_view stop_name) const {
	if (stopname_to_stop_.empty() || !stopname_to_stop_.count(stop_name)) {
		return nullptr;
	}
	return stopname_to_stop_.at(stop_name);
}

void Catalogue::AddBus(const Bus bus) {
	buses_.push_back(bus);

	for (const auto& stop : bus.stops_ptr) {
		// создаем список уникальных остановок маршрута
		unique_stops_[&buses_.back()].insert(stop);
		// вносим маршрут, проезжающий через остановку
		stop_to_buses_[stop].insert(bus.name);
	}
	// хеш-таблица маршрут - адрес структуры
	busname_to_bus_.insert({ bus.name, &buses_.back() });
}

Bus* Catalogue::FindBus(const std::string_view bus_name) const {
	if (busname_to_bus_.empty() || !busname_to_bus_.count(bus_name)) {
		return nullptr;
	}
	return busname_to_bus_.at(bus_name);
}

BusInfo Catalogue::GetBusInfo(const std::string_view bus_name) const {
	BusInfo info;

	// проверка наличия автобуса в базе
	if (!busname_to_bus_.count(bus_name)) {
		static BusInfo empty_info;
		return empty_info;
	}
	const auto bus_ptr = busname_to_bus_.at(bus_name);
	info.bus_ptr = bus_ptr;
	info.stops_on_route = bus_ptr->stops_ptr.size();

	// Расчитываем расстояние между остановками
	Stop* prev_stop = nullptr;
	for (const auto& stop : bus_ptr->stops_ptr) {
		if (prev_stop) {
			info.coordinate_length += geo::ComputeDistance(
				prev_stop->coordinates, stop->coordinates
			);
			info.route_length += static_cast<double>(GetDistanceBetweenStops({ prev_stop, stop }));
		}
		prev_stop = stop;
	}

	info.unique_stops = unique_stops_.at(bus_ptr).size();

	return info;
}

std::set<std::string_view> Catalogue::GetBusesForStop(std::string_view stop_name) const {
	const auto stop_ptr = stopname_to_stop_.at(stop_name);
	if (!stop_to_buses_.count(stop_ptr)) {
		static std::set<std::string_view> empty_buses;
		return empty_buses;
	}
	return stop_to_buses_.at(stop_ptr);
}

std::size_t Catalogue::HasherStops::operator() (const std::pair<Stop*, Stop*>& stops) const {
	std::hash<const Stop*> stop_hash;
	return stop_hash(stops.first) + static_cast<std::size_t>(37 * 10000) * stop_hash(stops.second);
}

void Catalogue::AddDistanceBetweenStops(const std::pair<Stop*, Stop*> stops, const uint32_t distance) {
	HasherStops hasher;
	if (!stop_distance_.count(hasher(stops))) {
		stop_distance_.insert({ hasher(stops), distance });
	} else {
		stop_distance_[hasher(stops)] = distance;
	}
}

uint32_t Catalogue::GetDistanceBetweenStops(const std::pair<Stop*, Stop*> stops) const {
	HasherStops hasher;
	// ищем остановки в обратном поярдке B - A
	if (!stop_distance_.count(hasher(stops))) {
		// возвращаем 0, если не задано расстояние
		if (!stop_distance_.count(hasher({ stops.second, stops.first }))) {
			return 0;
		} else {
			return stop_distance_.at(hasher({ stops.second, stops.first }));
		}
	}// Ищем остановки A - B
	else {
		return stop_distance_.at(hasher(stops));
	}
}

int Catalogue::GetStopCount() const {
	return static_cast<int>(stops_.size());
}

int Catalogue::GetBusCount() const {
	return static_cast<int>(buses_.size());
}

const std::map<std::string_view, Bus*> Catalogue::GetSortedBuses() const {
	std::map<std::string_view, Bus*> sorted_buses;
	for (const auto& bus : busname_to_bus_) {
		sorted_buses.emplace(bus);
	}
	return sorted_buses;
}

const std::map<std::string_view, Stop*> Catalogue::GetSortedStops() const {
	std::map<std::string_view, Stop*> sorted_stops;
	for (const auto& stop : stopname_to_stop_) {
		sorted_stops.emplace(stop);
	}
	return sorted_stops;
}
} // end of namespace transport_ctg