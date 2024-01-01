#pragma once
#include "router.h"
#include "graph.h"
#include "transport_catalogue.h"

#include <map>
#include <unordered_map>
#include <memory>
#include <string_view>

namespace transport_ctg {

struct RoutingSettings{
	int bus_wait_time = 0;
	double bus_velocity = 0.0;
};

class BusRouter {
private:
	using Graph = graph::DirectedWeightedGraph<double>;
	using Router = graph::Router<double>;

public:
	explicit BusRouter(const RoutingSettings& settings, const Catalogue& catalogue);
	
	const Graph& BuildGraph(const Catalogue& cataloge);
	const Graph& GetGraph() const;
	std::optional<Router::RouteInfo> FindRoute(Stop* from, Stop* to) const;

private:
	RoutingSettings settings_;
	std::unordered_map<Stop*, graph::VertexId> stop_to_ids_;

	Graph graph_;
	std::unique_ptr<Router> router_;
};
} // namespace transport_ctg