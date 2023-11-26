#pragma once
#include "domain.h"

#include <deque>
#include <map>
#include <set>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace transport_ctg {

	class Catalogue {
	public:

		int GetStopCount() const;
		int GetBusCount() const;
		// добавление остановки в базу
		// Stop X: latitude, longitude
		void AddStop(Stop stop);
		// поиск остановки по имени
		Stop* FindStop(const std::string_view stop_name) const;

		// добавление маршрута в базу
		// Bus X: stop1>stop2>...>stopN>stop1 (кольцевой маршрут)
		// Bus X: stop1-stop2-...-stopN (обычный маршрут)
		void AddBus(const Bus bus);

		// поиск маршрута по имени
		Bus* FindBus(const std::string_view bus_name) const;

		// получение информации о маршруте
		// Bus X: R stops on route, U unique stops, L route length
		BusInfo GetBusInfo(const std::string_view bus_name) const;

		// метод для получения списка автобусов по остановке
		std::set<std::string_view> GetBusesForStop(std::string_view stop_name) const;

		// метода задания дистанции между остановками
		void AddDistanceBetweenStops(const std::pair<Stop*, Stop*> stops, const uint32_t distance);

		// получение дистанции между остановками
		uint32_t GetDistanceBetweenStops(const std::pair<Stop*, Stop*> stops) const;

		const std::map<std::string_view, Bus*> GetSortedBuses() const;
		const std::map<std::string_view, Stop*> GetSortedStops() const;

	private:
		std::deque<Stop> stops_;
		std::deque<Bus> buses_;
		// хеш-таблица название-указатель остановки
		std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
		// хеш-таблица название-указатель маршрута
		std::unordered_map<std::string_view, Bus*> busname_to_bus_;
		// список уникальных остановок маршрута
		std::unordered_map<Bus*, std::unordered_set<Stop*>> unique_stops_;
		// список автобусов для остановки
		std::unordered_map<Stop*, std::set<std::string_view>> stop_to_buses_;
		// таблица расстояний между остановками
		std::unordered_map<std::size_t, uint32_t> stop_distance_;

		struct HasherStops {
			std::size_t operator() (const std::pair<Stop*, Stop*>& StopPtrPair) const;
		};
	}; // end of class Catalogue
}// конец пространства имен transport_ctg