#include <iostream>
#include <iomanip>
#include <string_view>

#include "transport_catalogue.h"
#include "json_reader.h"
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

    json::request_handler::RequestHandler(requests, catalogue, map_renderer, std::cout);
}