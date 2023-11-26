#pragma once

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

namespace renderer{
bool IsZero(double value);

// хранит настройки визуализации карты
struct RenderSettings {
    RenderSettings() = default;

    double width = 0.0; // ширина изображения в пикселях
    double height = 0.0; // высота изображения в пикселях
    double padding = 0.0; // отступ от краев карты до границ SVG-документа
    double line_width = 0.0; // толщина линий
    double stop_radius = 0.0; // радиус окружностей, которыми обозначаются остановки
    uint32_t bus_label_font_size = 0; // размер текста, которыми написаны названия маршрутов
    svg::Point bus_label_offset = { 0.0, 0.0 }; // смещение надписи с названием маршрута относительно координат конечной остановки
    uint32_t stop_label_font_size = 0; //
    svg::Point stop_label_offset = { 0.0, 0.0 };
    svg::Color underlayer_color = { svg::NoneColor }; //цвет подложки под названиями остановок и маршрутов
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette{}; // цветовая палитра
};

// проецирует координаты остановок на карту
class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
    double max_width, double max_height, double padding)
    : padding_(padding) //
{
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
        return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициент масштабирования вдоль координаты x
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    // Вычисляем коэффициент масштабирования вдоль координаты y
    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        // Коэффициенты масштабирования по ширине и высоте ненулевые,
        // берём минимальный из них
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        // Коэффициент масштабирования по ширине ненулевой, используем его
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        // Коэффициент масштабирования по высоте ненулевой, используем его
        zoom_coeff_ = *height_zoom;
    }
}

class MapRenderer {
public:
    MapRenderer(const RenderSettings& render_settings);

    std::vector<svg::Polyline> GetRoute(const std::map<std::string_view, transport_ctg::Bus*>& buses, const SphereProjector& sp) const;
    std::vector<svg::Text> GetBusLabel(const std::map<std::string_view, transport_ctg::Bus*>& buses, const SphereProjector& sp) const;
    std::vector<svg::Circle> GetStopsSymbols(const std::map<std::string_view, transport_ctg::Stop*>& stops, const SphereProjector& sp) const;
    std::vector<svg::Text> GetStopLabel(const std::map<std::string_view, transport_ctg::Stop*>& stops, const SphereProjector& sp) const;

    svg::Document GetRenderedMap(const std::map<std::string_view, transport_ctg::Bus*>& buses) const;
private:
    const RenderSettings render_settings_;
};

} // namespace renderer