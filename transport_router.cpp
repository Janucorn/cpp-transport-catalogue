#include "transport_router.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <string>
#include <string_view>

namespace transport_ctg {
    
BusRouter::BusRouter(const RoutingSettings& settings, const Catalogue& catalogue) 
	: settings_(settings)
	{
	graph_ = BuildGraph(catalogue);
}

const BusRouter::Graph& BusRouter::BuildGraph(const Catalogue& catalogue) {
	const auto& all_stops = catalogue.GetSortedStops();
	const auto& all_buses = catalogue.GetSortedBuses();
	// граф с двумя вершинами на каждой остановке
	Graph stops_graph(all_stops.size() * 2);
	graph::VertexId vertex_id = 0;

	for (const auto& [stopname, stop] : all_stops) {
		stop_to_ids_.insert({ stop, vertex_id });
		// ребро ожидания автобуса
		stops_graph.AddEdge({
			static_cast<std::string>(stopname),
			0,
			vertex_id,
			++vertex_id,
			static_cast<double>(settings_.bus_wait_time)
			});
		++vertex_id;
	}

	std::for_each(all_buses.begin(), all_buses.end(),
		[&stops_graph, this, &catalogue](const auto& item) {
			const auto& bus = item.second;
			const auto& stops = bus->stops_ptr;
			size_t stops_count = stops.size();

			for (size_t i = 0; i < stops_count; ++i) {
				for (size_t j = i + 1; j < stops_count; ++j) {
					Stop* from = stops[i];
					Stop* to = stops[j];
					int distance = 0;
					int inverse_distance = 0;
					static const double km_to_meters = 1000.0;
					static const double hour_to_minutes = 60.0;

					for (size_t k = i + 1; k <= j; ++k) {
						distance += catalogue.GetDistanceBetweenStops({ stops[k - 1], stops[k] });
						inverse_distance += catalogue.GetDistanceBetweenStops({ stops[k], stops[k - 1] });
					}

					stops_graph.AddEdge({
						static_cast<std::string>(bus->name),
						j - i,
						stop_to_ids_.at(from) + 1,
						stop_to_ids_.at(to),
						static_cast<double>(distance) / (settings_.bus_velocity * km_to_meters / hour_to_minutes)
						});

					if (!bus->is_roundtrip) {
						stops_graph.AddEdge({
							static_cast<std::string>(bus->name),
							j - i,
							stop_to_ids_.at(to) + 1,
							stop_to_ids_.at(from),
							static_cast<double>(inverse_distance) / (settings_.bus_velocity * km_to_meters / hour_to_minutes)
							});
					}
				}
			}
		});

	graph_ = std::move(stops_graph);
	router_ = std::make_unique<graph::Router<double>>(graph_);

	return graph_;
}

const BusRouter::Graph& BusRouter::GetGraph() const {
	return graph_;
}

std::optional<BusRouter::Router::RouteInfo> BusRouter::FindRoute(Stop* from, Stop* to) const {
	return router_->BuildRoute(stop_to_ids_.at(from), stop_to_ids_.at(to));
}

} // namespace transport_ctg