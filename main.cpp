#include <iostream>
#include <iomanip>
#include <string_view>

#include "transport_catalogue.h"
#include "transport_router.h"
#include "json_reader.h"
#include "json_builder.h"
#include "request_handler.h"
#include "map_renderer.h"

using namespace transport_ctg;

int main() {
	using namespace std::literals;
	
	Catalogue catalogue;
	json::JsonReader requests(std::cin);

	requests.AddToCatalogue(catalogue);
    
    const auto& settings = requests.GetRenderSettings().AsMap();
    const auto& map_renderer = requests.SetMapRenderer(settings);
    const auto& route_settings = requests.GetRoutingSettings().AsMap();
    const auto& router = requests.SetRouter(route_settings, catalogue);
    
    json::request_handler::RequestHandler(requests, catalogue, map_renderer, router, std::cout);
}
