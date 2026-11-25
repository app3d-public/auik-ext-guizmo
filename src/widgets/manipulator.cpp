#include <amal/geometric.hpp>
#include <amal/trigonometric.hpp>
#include <auik/auik.hpp>
#include <auik/detail/vertex_draw/lines.hpp>
#include <auik/ext/guizmo/manipulator.hpp>
#include <auik/ext/guizmo/widget_tags.hpp>
#include <bit>

namespace auik::guizmo
{
    using detail::append_segment;
    using detail::append_vertex;

    static inline amal::vec2 perpendicular(amal::vec2 p) { return {-p.y, p.x}; }

    Manipulator::Manipulator(u32 id, ManipulatorCreateFlags flags)
        : Widget(id, WidgetFlagBits::visible | WidgetFlagBits::attachable, EventFlagBits::none, {{}, {1.f, 1.f}}, 0),
          _create_flags(flags)
    {
    }

    Manipulator::Appearance Manipulator::read_appearance() const
    {
        auto *theme = get_theme();
        Appearance appearance;
        if (!theme) return appearance;
        auto style = [&](u32 tag, StyleState state = StyleState::normal) -> const Style & {
            return theme->get_style(theme->get_resolved_style(tag, 0u, 0u, state));
        };
        const auto &shaft = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_SHAFT);
        const auto &arrow = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ARROW);
        const auto &shaft_hover = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_SHAFT, StyleState::hover);
        const auto &arrow_hover = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ARROW, StyleState::hover);
        const auto &guide = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE);
        const auto &edge = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_EDGE);
        appearance.reach = shaft.width();
        appearance.hover_reach = shaft_hover.width();
        appearance.shaft_width = shaft.height();
        appearance.hover_shaft_width = shaft_hover.height();
        appearance.arrow_size = {arrow.width(), arrow.height()};
        appearance.hover_arrow_size = {arrow_hover.width(), arrow_hover.height()};
        using Properties = detail::StylePropertiesBits;
        const auto dimensions = Properties::width | Properties::height;
        appearance.hover_shaft_geometry = bool(make_style_update_flags(shaft, shaft_hover, dimensions));
        appearance.hover_arrow_geometry = bool(make_style_update_flags(arrow, arrow_hover, dimensions));
        auto plane_style = [&](u32 tag) -> PlaneAppearance {
            const auto &base = style(tag);
            const auto &hover = style(tag, StyleState::hover);
            const auto &active = style(tag, StyleState::active);
            return {{base.width(), base.border_radius(), base.border_thickness()},
                    {base.background_color(), hover.background_color(), active.background_color()},
                    {base.border_color(), hover.border_color(), active.border_color()}};
        };
        appearance.planes = {plane_style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_PLANE_YZ),
                             plane_style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_PLANE_XZ),
                             plane_style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_PLANE_XY)};
        appearance.locked_plane = plane_style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_PLANE_LOCKED);

        constexpr u32 axis_tags[] = {AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_X, AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_Y,
                                     AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_Z};
        for (u32 axis = 0; axis < 3; ++axis)
            appearance.axes[axis] = {style(axis_tags[axis]).background_color(),
                                     style(axis_tags[axis], StyleState::hover).background_color(),
                                     style(axis_tags[axis], StyleState::active).background_color()};
        appearance.locked_alpha = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_LOCKED).background_color() & 0xff000000u;
        appearance.dimmed_alpha = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_DIMMED).background_color() >> 24u;
        appearance.hover_dimmed_alpha =
            style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_DIMMED, StyleState::hover).background_color() >> 24u;
        appearance.guide_width = guide.height();
        appearance.drag_pivot_diameter = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_DRAG_PIVOT).width();
        appearance.guide_colors = {style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE_X).background_color(),
                                   style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE_Y).background_color(),
                                   style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE_Z).background_color()};
        const auto &rotation = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION);
        const auto &rotation_hover = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION, StyleState::hover);
        const auto &scale = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_SCALE);
        const auto &scale_hover = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_SCALE, StyleState::hover);
        const auto &view_ring = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_VIEW_RING);
        appearance.rotation_radius = rotation.width();
        appearance.view_ring_gap = view_ring.padding().x;
        appearance.view_ring_width = view_ring.height();
        appearance.view_ring_outline_width = view_ring.border_thickness();
        appearance.view_ring_outline_color = view_ring.border_color();
        appearance.view_ring = {
            view_ring.background_color(),
            style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_VIEW_RING, StyleState::hover).background_color(),
            style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_VIEW_RING, StyleState::active).background_color()};
        constexpr u32 sector_tags[] = {AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION_SECTOR_X,
                                       AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION_SECTOR_Y,
                                       AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION_SECTOR_Z};
        for (u32 axis = 0; axis < 3; ++axis)
            appearance.rotation_sector_colors[axis] = style(sector_tags[axis]).background_color();
        appearance.view_sector_color = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION_SECTOR).background_color();
        appearance.rotation_width = rotation.height();
        appearance.hover_rotation_width = rotation_hover.height();
        appearance.scale_size = scale.width();
        appearance.hover_scale_size = scale_hover.width();
        appearance.hover_rotation_geometry =
            bool(make_style_update_flags(rotation, rotation_hover, Properties::height));
        appearance.hover_scale_geometry = bool(make_style_update_flags(scale, scale_hover, Properties::width));
        appearance.edge_size = {edge.width(), edge.height()};
        appearance.edge_padding = edge.padding().x;
        appearance.edge_rounding = edge.border_radius();
        appearance.edge = {edge.background_color(),
                           style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_EDGE, StyleState::hover).background_color(),
                           edge.background_color()};
        return appearance;
    }

    StyleUpdateFlags Manipulator::update_style()
    {
        _appearance = read_appearance();
        if (_projection) project_frame(*_projection);
        return {};
    }

    bool Manipulator::project(const amal::mat4 &matrix, const amal::vec3 &pivot, const amal::rect &viewport,
                              const amal::rect &bounds, const Options &options)
    {
        _projection = Projection{matrix, pivot, viewport, bounds, options};
        return project_frame(*_projection);
    }

    bool Manipulator::project_frame(const Projection &projection, bool visual_changed)
    {
        const auto &[matrix, pivot, viewport, bounds, options] = projection;
        if (bounds.size.x <= 32.f || bounds.size.y <= 32.f) return false;
        auto clip = matrix * amal::vec4(pivot, 1.f);
        auto half = viewport.size * 0.5f;
        auto center = viewport.offset + half;
        auto w = std::max(std::abs(clip.w), 1e-6f);
        amal::vec2 ndc{clip.x / w, clip.y / w};
        auto point = center + ndc * half;
        Frame frame{};
        frame.operation = options.operation;
        const f32 length = amal::length(options.view_direction);
        if (frame.operation == Operation::rotate && length > 1e-6f)
            frame.view_direction = options.view_direction / length;
        frame.available = _create_flags & ManipulatorCreateFlagBits::all_axes;
        if (!frame.available) return false;
        frame.enabled = options.enabled_axes & frame.available;
        const bool rotating = frame.operation == Operation::rotate;
        if (!rotating)
        {
            frame.reversed_handles = options.reversed_handles;
            for (u32 axis = 0; axis < 3; ++axis)
                if (options.view_direction[axis] < 0.f) frame.reversed_handles ^= 1u << axis;
        }
        if (!_appearance) _appearance = read_appearance();
        frame.appearance = *_appearance;
        const f32 reach = rotating ? frame.appearance.rotation_radius : frame.appearance.reach;
        if (reach <= 0.f) return false;
        auto inset = bounds.size * 0.5f - amal::vec2{frame.appearance.edge_padding};
        if (inset.x <= 0.f || inset.y <= 0.f) return false;
        auto edge_center = bounds.offset + bounds.size * 0.5f;
        auto direction = point - edge_center;
        if (clip.w <= 0 || std::abs(direction.x) > inset.x || std::abs(direction.y) > inset.y)
        {
            if (!(_create_flags & ManipulatorCreateFlagBits::out_of_bounds_marker)) return false;
            if (amal::length(direction) < 0.01f) direction = {0.f, -1.f};
            frame.edge_direction = amal::normalize(direction);
            point =
                edge_center + direction / std::max(std::abs(direction.x) / inset.x, std::abs(direction.y) / inset.y);
        }
        else if (_dragging && rotating)
        {
            frame.axes = _drag_frame.axes;
            frame.view_direction = _drag_frame.view_direction;
            frame.rotation_handle = _drag_handle ? _drag_handle : _drag_mask;
            frame.rotation_start = _rotation_start;
            frame.rotation_sweep = _rotation_sweep;
        }
        else
        {
            // A rotation ring needs the two projected basis vectors in its plane.
            u32 projected_axes = frame.available;
            if (rotating) projected_axes = std::has_single_bit(projected_axes) ? 7u ^ projected_axes : 7u;
            const bool need_vertical = rotating || projected_axes != 7u;
            f32 xx = 0.f, xy = 0.f, yy = 0.f;
            for (u32 remaining = projected_axes; remaining; remaining &= remaining - 1u)
            {
                const auto i = std::countr_zero(remaining);
                const auto column = matrix[i];
                const auto axis = (amal::vec2{column.x, column.y} - ndc * column.w) * half / w;
                frame.axes[i] = axis;
                xx += axis.x * axis.x;
                if (need_vertical) yy += axis.y * axis.y;
                if (rotating) xy += axis.x * axis.y;
            }
            // The largest projected radius fits inside the view ring even away from the center.
            const f32 norm_squared = rotating ? (xx + yy + std::hypot(xx - yy, 2.f * xy)) * 0.5f : std::max(xx, yy);
            _world_radius = reach / amal::max(amal::sqrt(norm_squared), 1e-6f);
            for (u32 remaining = projected_axes; remaining; remaining &= remaining - 1u)
            {
                auto &axis = frame.axes[std::countr_zero(remaining)];
                axis *= _world_radius;
                // Keep subpixel noise from invalidating the vertex stream during translation.
                axis.x = amal::round(axis.x * 64.f) / 64.f;
                axis.y = amal::round(axis.y * 64.f) / 64.f;
            }
            if (_dragging && _drag_mask != 7u && (_create_flags & ManipulatorCreateFlagBits::axis_guides))
            {
                frame.guides = _drag_mask;
                frame.guide_length = amal::length(bounds.size);
            }
        }
        place(point, frame, visual_changed);
        return true;
    }

    u32 Manipulator::begin_drag(const amal::vec2 &screen, bool drag_background)
    {
        if (_drag_mask) end_drag();
        const auto handle = hit_handle(screen);
        if (handle == edge_handle) return handle;
        auto hit = handle == view_ring_handle && _frame.enabled == 7u ? view_ring_handle : handle & _frame.enabled;
        if (handle && !hit) return 0;
        if (!handle && drag_background && _frame.edge_direction == amal::vec2{})
        {
            hit = _frame.enabled;
            if (_frame.operation == Operation::rotate)
            {
                f32 best = -1.f;
                for (u32 i = 0; i < 3; ++i)
                {
                    if ((_frame.enabled & (1u << i)) && amal::abs(_frame.view_direction[i]) > best)
                    {
                        best = amal::abs(_frame.view_direction[i]);
                        hit = 1u << i;
                    }
                }
            }
        }
        _drag_mask = hit;
        _drag_handle = handle;
        _drag_frame = _frame;
        _drag_world_radius = _world_radius;
        _rotation_input = _rotation_angle = _rotation_start = _rotation_sweep = 0.f;
        _start = screen;
        _center = position();
        set_hover(hit);
        return hit;
    }

    std::array<amal::vec2, 2> Manipulator::rotation_axes(const Frame &frame, u32 handle)
    {
        if (handle == view_ring_handle)
        {
            const f32 radius = frame.appearance.rotation_radius + frame.appearance.view_ring_gap;
            return {amal::vec2{radius, 0.f}, amal::vec2{0.f, radius}};
        }
        const u32 axis = std::countr_zero(handle);
        return {frame.axes[(axis + 1) % 3], frame.axes[(axis + 2) % 3]};
    }

    bool Manipulator::drag_delta(const amal::vec2 &screen, amal::vec3 &result)
    {
        return drag_delta(screen, result, false);
    }

    bool Manipulator::drag_delta(const amal::vec2 &screen, amal::vec3 &result, bool snap_rotation)
    {
        if (!_drag_mask) return false;
        auto movement = screen - _start;
        const bool started = !_dragging && movement != amal::vec2{};
        if (started)
        {
            _dragging = true;
            if (_projection && _drag_frame.operation != Operation::rotate) project_frame(*_projection, true);
        }
        result = {};
        auto axes = _drag_frame.axes;
        u32 axis = std::countr_zero(_drag_mask);
        if (_drag_frame.operation == Operation::move)
        {
            // Least-squares inverse of the allowed projected axes. One-dimensional
            // projections (including an edge-on plane) use the visible direction.
            f32 xx = 0, xy = 0, yy = 0;
            for (u32 i = 0; i < 3; ++i)
            {
                if (_drag_mask & (1u << i))
                {
                    xx += axes[i].x * axes[i].x;
                    xy += axes[i].x * axes[i].y;
                    yy += axes[i].y * axes[i].y;
                }
            }
            f32 determinant = xx * yy - xy * xy;
            auto inverse = movement / amal::max(xx + yy, 1.f);
            if (determinant > 1e-4f * (xx + yy) * (xx + yy))
                inverse =
                    amal::vec2{yy * movement.x - xy * movement.y, xx * movement.y - xy * movement.x} / determinant;
            for (u32 i = 0; i < 3; ++i)
                if (_drag_mask & (1u << i)) result[i] = amal::dot(axes[i], inverse) * _drag_world_radius;
        }
        else if (_drag_frame.operation == Operation::scale)
        {
            result = amal::vec3{1.f};
            auto direction = _start - _center;
            if (std::has_single_bit(_drag_mask))
                direction = (_drag_frame.reversed_handles & (1u << axis)) ? -axes[axis] : axes[axis];
            if (amal::length(direction) < 6.f) direction = {1.f, -1.f};
            f32 factor = 1.f + amal::dot(movement, amal::normalize(direction)) / _drag_frame.appearance.reach;
            for (u32 i = 0; i < 3; ++i)
                if (_drag_mask & (1u << i)) result[i] = factor;
        }
        else
        {
            const auto [u, v] = rotation_axes(_drag_frame, _drag_mask);
            auto cross = [](amal::vec2 a, amal::vec2 b) { return a.x * b.y - a.y * b.x; };
            const auto start = _start - _center, end = start + movement;
            const f32 determinant = cross(u, v);
            constexpr f32 turn = 2.f * amal::pi<f32>();
            if (amal::abs(determinant) > 150.f && amal::length(start) >= 6.f)
            {
                const auto angle_at = [&](amal::vec2 point) {
                    return amal::atan2(cross(u, point) / determinant, cross(point, v) / determinant);
                };
                _rotation_start = angle_at(start);
                if (amal::length(end) >= 2.f)
                {
                    const f32 input = angle_at(end) - _rotation_start;
                    _rotation_angle += std::remainder(input - _rotation_input, turn);
                    _rotation_input = input;
                }
            }
            else
            {
                const auto tangent = amal::length(u) > amal::length(v) ? u : v;
                if (amal::length(tangent) > 1e-4f)
                    _rotation_angle =
                        amal::dot(movement, amal::normalize(tangent)) / _drag_frame.appearance.rotation_radius;
            }
            f32 degrees = _rotation_angle * (360.f / turn);
            if (snap_rotation) degrees = amal::round(degrees / 5.f) * 5.f;
            result[_drag_mask == view_ring_handle ? 2u : axis] = degrees;
            const f32 sweep = amal::clamp(degrees * (turn / 360.f), -turn, turn);
            if (_projection && (started || _rotation_sweep != sweep))
            {
                _rotation_sweep = sweep;
                project_frame(*_projection, started);
            }
        }
        return true;
    }

    void Manipulator::set_hover(u32 handle)
    {
        if (_drag_mask) handle = _drag_handle ? _drag_handle : _drag_mask;
        if (_hover == handle) return;
        _hover = handle;
        rebuild_cached_visuals();
        if (is_attached() && is_visible()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
        mark_host_refresh_request();
    }

    void Manipulator::end_drag()
    {
        _drag_mask = 0;
        _drag_handle = 0;
        const bool was_dragging = std::exchange(_dragging, false);
        const bool restore_rotation = _frame.rotation_handle && _projection;
        if (_frame.guides || _frame.rotation_handle)
        {
            _frame.guides = 0;
            _frame.guide_length = 0.f;
            _frame.rotation_handle = 0u;
            _frame.rotation_start = _frame.rotation_sweep = 0.f;
            build_geometry(_hit_geometry);
            _hover_geometry_mask = 0u;
        }
        if (std::exchange(_hover, 0u) || was_dragging)
        {
            if (detail::get_context().dirty_flags & DirtyFlagBits::destroying) return;
            if (restore_rotation) project_frame(*_projection, true);
            else
            {
                rebuild_cached_visuals();
                if (is_attached() && is_visible()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
            }
            mark_host_refresh_request();
        }
    }

    void Manipulator::place(const amal::vec2 &pivot, const Frame &frame, bool visual_changed)
    {
        const bool changed = (_hit_geometry.segments.empty() && _hit_geometry.faces.empty()) || !(_frame == frame);
        if (changed)
        {
            _frame = frame;
            build_geometry(_hit_geometry);
            _hover_geometry_mask = 0u;
        }
        if (changed || visual_changed) rebuild_cached_visuals();
        else if (position() == pivot) return;
        set_position(pivot);
        if (is_attached()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
    }

    void Manipulator::build_geometry(Geometry &geometry, u32 hover) const
    {
        geometry.segments.clear();
        geometry.faces.clear();
        auto line = [&](amal::vec2 a, amal::vec2 b, f32 width, Palette color, u32 mask) {
            if (width > 0.f && amal::length(b - a) > 0.001f) geometry.segments.push_back({a, b, width, color, mask});
        };
        const auto &appearance = _frame.appearance;
        if (_frame.edge_direction != amal::vec2{})
        {
            const auto dir = _frame.edge_direction;
            const auto half = appearance.edge_size * 0.5f;
            if (half.x <= 0.f || half.y <= 0.f) return;
            const auto tip = dir * half.y;
            const amal::vec2 corners[] = {tip, -tip + perpendicular(dir) * half.x, -tip - perpendicular(dir) * half.x};
            const f32 rounding = amal::clamp(appearance.edge_rounding, 0.f, amal::min(half.x, half.y) * 0.5f);
            constexpr size_t arc_steps = 4;
            std::array<amal::vec2, 3 * (arc_steps + 1)> contour{};
            size_t count = 0;
            for (size_t i = 0; i < std::size(corners); ++i)
            {
                const auto corner = corners[i];
                if (rounding == 0.f)
                {
                    contour[count++] = corner;
                    continue;
                }
                const auto start = corner + amal::normalize(corners[(i + 2) % 3] - corner) * rounding;
                const auto end = corner + amal::normalize(corners[(i + 1) % 3] - corner) * rounding;
                for (size_t step = 0; step <= arc_steps; ++step)
                {
                    const f32 t = f32(step) / arc_steps;
                    contour[count++] =
                        (1.f - t) * ((1.f - t) * start + t * corner) + t * ((1.f - t) * corner + t * end);
                }
            }
            // Fade outside the contour without drawing a second layer over the translucent fill.
            const auto outer = [&](size_t i) {
                const auto p = contour[i];
                const auto before = amal::normalize(perpendicular(contour[(i + count - 1) % count] - p));
                const auto after = amal::normalize(perpendicular(p - contour[(i + 1) % count]));
                const auto join = amal::normalize(before + after);
                return p + join / amal::dot(join, after);
            };
            auto outer_p = outer(0);
            for (size_t i = 0; i < count; ++i)
            {
                const size_t next = (i + 1) % count;
                const auto p = contour[i], q = contour[next], outer_q = outer(next);
                geometry.faces.push_back({{}, p, q, appearance.edge, edge_handle});
                geometry.faces.push_back({p, outer_q, q, appearance.edge, edge_handle, 0b010});
                geometry.faces.push_back({p, outer_p, outer_q, appearance.edge, edge_handle, 0b110});
                outer_p = outer_q;
            }
            return;
        }
        if (_frame.operation == Operation::rotate)
        {
            constexpr f32 pi = amal::pi<f32>();
            constexpr f32 turn = 2.f * pi;
            constexpr int steps = 64;
            auto ring = [&](u32 handle, Palette color, u32 fill, f32 width, f32 start, f32 sweep) {
                const auto [u, v] = rotation_axes(_frame, handle);
                auto point = [&](f32 theta) { return u * amal::cos(theta) + v * amal::sin(theta); };
                if (handle != view_ring_handle)
                {
                    auto previous = point(start);
                    for (int step = 1; step <= steps; ++step)
                    {
                        const auto next = point(start + sweep * step / steps);
                        line(previous, next, width, color, handle);
                        previous = next;
                    }
                }
                if (_frame.rotation_handle != handle || amal::abs(_frame.rotation_sweep) < 1e-5f) return;

                const Palette sector{fill, fill, fill};
                const f32 begin = _frame.rotation_start, angle = _frame.rotation_sweep;
                const int sector_steps = amal::max(1, static_cast<int>(amal::ceil(steps * amal::abs(angle) / turn)));
                auto previous = point(begin);
                for (int step = 1; step <= sector_steps; ++step)
                {
                    const auto next = point(begin + angle * step / sector_steps);
                    geometry.faces.push_back({{}, previous, next, sector, 0u});
                    previous = next;
                }
            };
            for (u32 remaining = _frame.available; remaining; remaining &= remaining - 1u)
            {
                const u32 axis = std::countr_zero(remaining), bit = 1u << axis;
                if (_frame.rotation_handle && _frame.rotation_handle != bit) continue;
                auto color = appearance.axes[axis];
                if (!(_frame.enabled & bit))
                {
                    const u32 locked = (color.normal & 0x00ffffffu) | appearance.locked_alpha;
                    color = {locked, locked, locked};
                }
                const f32 a = _frame.view_direction[(axis + 1) % 3], b = _frame.view_direction[(axis + 2) % 3];
                const bool full = _frame.rotation_handle == bit || a * a + b * b < 1e-6f;
                const f32 width = hover == bit && (_frame.enabled & bit) ? appearance.hover_rotation_width
                                                                         : appearance.rotation_width;
                // view_direction points from the camera to the pivot. Keep the arc facing the camera.
                ring(bit, color, appearance.rotation_sector_colors[axis], width,
                     full ? 0.f : std::atan2(b, a) + pi * 0.5f, full ? turn : pi);
            }
            if (_frame.rotation_handle == view_ring_handle && has_view_ring())
                ring(view_ring_handle, appearance.view_ring, appearance.view_sector_color, 0.f, 0.f, turn);
            return;
        }

        for (u32 i = 0; i < 3; ++i)
        {
            u32 bit = 1u << i;
            const u32 j_axis = (i + 1) % 3, k_axis = (i + 2) % 3;
            auto oriented = [&](u32 index) {
                return (_frame.reversed_handles & (1u << index)) ? -_frame.axes[index] : _frame.axes[index];
            };
            if (_frame.available & bit)
            {
                auto axis = oriented(i);
                const bool axis_hovered = (hover & _frame.enabled & bit) != 0u;
                const f32 shaft_width = axis_hovered ? appearance.hover_shaft_width : appearance.shaft_width;
                auto color = appearance.axes[i];
                if ((_frame.enabled & bit) == 0u)
                {
                    const u32 locked = (color.normal & 0x00ffffffu) | appearance.locked_alpha;
                    color = {locked, locked, locked};
                }
                if ((_frame.guides & bit) && amal::length(axis) > 1.f)
                {
                    auto end = amal::normalize(axis) * _frame.guide_length;
                    const u32 guide_color = appearance.guide_colors[i];
                    line(-end, end, appearance.guide_width, {guide_color, guide_color, guide_color}, 0);
                }
                if (axis_hovered) axis *= appearance.hover_reach / appearance.reach;
                const f32 axis_length = amal::length(axis);
                if (axis_length > 4.f)
                {
                    auto dir = amal::normalize(axis), n = perpendicular(dir);
                    if (_frame.operation == Operation::move)
                    {
                        const auto size = axis_hovered ? appearance.hover_arrow_size : appearance.arrow_size;
                        const auto base = axis - dir * amal::min(size.y, axis_length);
                        const auto shoulder = n * (size.x * 0.5f);
                        const auto left = base + shoulder, right = base - shoulder;
                        line({}, base, shaft_width, color, bit);
                        geometry.faces.push_back({axis, left, right, color, bit});
                        line(axis, left, 0.5f, color, bit);
                        line(axis, right, 0.5f, color, bit);
                        line(left, right, 0.5f, color, bit);
                    }
                    else
                    {
                        line({}, axis, shaft_width, color, bit);
                        const auto size = axis_hovered ? appearance.hover_scale_size : appearance.scale_size;
                        const f32 half = size * 0.5f;
                        line(axis - dir * half, axis + dir * half, size, color, bit);
                        for (auto end : {axis - dir * half, axis + dir * half})
                            line(end - n * half, end + n * half, 0.5f, color, bit);
                    }
                }
            }
            // Offset plane handles leave the pivot and the axis tips accessible.
            const u32 plane = 7u ^ bit;
            if ((_frame.available & plane) != plane) continue;
            const auto u = oriented(j_axis), v = oriented(k_axis);
            if (amal::abs(u.x * v.y - u.y * v.x) < 100.f) continue;
            const auto &plane_style = (_frame.enabled & plane) ? appearance.planes[i] : appearance.locked_plane;
            const auto &size = plane_style.size;
            const f32 half_size = size.width * 0.5f;
            if (half_size <= 0.f) continue;
            const f32 low = 0.46f - half_size / appearance.reach;
            const f32 high = 0.46f + half_size / appearance.reach;
            const f32 corner_radius = amal::clamp(size.rounding, 0.f, half_size) / appearance.reach;
            constexpr int arc_steps = 5;
            const amal::vec2 corners[] = {{high - corner_radius, high - corner_radius},
                                          {low + corner_radius, high - corner_radius},
                                          {low + corner_radius, low + corner_radius},
                                          {high - corner_radius, low + corner_radius}};
            const auto center = (u + v) * ((high + low) * 0.5f);

            auto previous = u * high + v * (low + corner_radius);
            for (int corner = 0; corner < 4; ++corner)
            {
                for (int step = 0; step <= arc_steps; ++step)
                {
                    const f32 angle = (corner + f32(step) / arc_steps) * 1.570796327f;
                    const auto uv = corners[corner] + amal::vec2{std::cos(angle), std::sin(angle)} * corner_radius;
                    const auto point = u * uv.x + v * uv.y;
                    geometry.faces.push_back({center, previous, point, plane_style.fill, plane});
                    line(previous, point, size.border_width, plane_style.border, plane);
                    previous = point;
                }
            }
        }
    }

    bool Manipulator::has_view_ring() const
    {
        return _frame.operation == Operation::rotate && _frame.available == 7u &&
               _frame.edge_direction == amal::vec2{} && _frame.appearance.view_ring_width > 0.f &&
               (!_frame.rotation_handle || _frame.rotation_handle == view_ring_handle);
    }

    u32 Manipulator::pick(const amal::vec2 &screen) const
    {
        u32 hit = hit_handle(screen);
        if (hit == view_ring_handle) return _frame.enabled == 7u ? hit : 0u;
        return hit == edge_handle || (hit & _frame.enabled) != 0u ? hit : 0u;
    }

    u32 Manipulator::hit_handle(const amal::vec2 &screen) const
    {
        auto p = screen - position();
        auto side = [](amal::vec2 a, amal::vec2 b) { return a.x * b.y - a.y * b.x; };
        for (const auto &face : _hit_geometry.faces)
        {
            if (!face.handle || face.fringe_mask || amal::abs(side(face.b - face.a, face.c - face.a)) < 1e-5f) continue;
            auto a = side(face.b - face.a, p - face.a);
            auto b = side(face.c - face.b, p - face.b);
            auto c = side(face.a - face.c, p - face.c);
            // Include the shared diagonal even when rounding gives opposite edge tests.
            constexpr f32 tolerance = 0.001f;
            if (amal::min({a, b, c}) >= -tolerance || amal::max({a, b, c}) <= tolerance) return face.handle;
        }
        if (_frame.edge_direction != amal::vec2{} && !_hit_geometry.faces.empty() && amal::length(p) < 7.f)
            return edge_handle;
        f32 best = 7.f;
        u32 hit = 0;
        if (has_view_ring())
        {
            const auto &appearance = _frame.appearance;
            const f32 radius = appearance.rotation_radius + appearance.view_ring_gap;
            const f32 distance = std::abs(amal::length(p) - radius);
            if (distance < best)
            {
                best = distance;
                hit = view_ring_handle;
            }
        }
        for (auto &s : _hit_geometry.segments)
        {
            if (!s.handle) continue;
            auto ab = s.b - s.a;
            f32 t = amal::clamp(amal::dot(p - s.a, ab) / amal::dot(ab, ab), 0.f, 1.f);
            f32 distance = amal::length(p - s.a - ab * t);
            if (distance < best)
            {
                best = distance;
                hit = s.handle;
            }
        }
        return hit;
    }

    void Manipulator::reset_draw_records()
    {
        Widget::reset_draw_records();
        _record = {};
    }

    void Manipulator::set_clip_id(u16 id)
    {
        Widget::set_clip_id(id);
        sync_vertex_placement();
    }

    void Manipulator::rebuild_clip_rects() { sync_vertex_placement(); }

    void Manipulator::update_depth(const amal::vec2 &depth_range)
    {
        Widget::update_depth(depth_range);
        sync_vertex_placement();
    }

    void Manipulator::sync_vertex_placement()
    {
        const auto depth = get_z_order();
        const auto clip = clip_id();
        if (_depth == depth && _clip == clip) return;
        _depth = depth;
        _clip = clip;
        for (auto &vertex : _vertices)
        {
            vertex.z_order = depth;
            vertex.clip_id = clip;
        }
        _vertices_dirty = true;
    }

    void Manipulator::rebuild_cached_visuals()
    {
        const auto depth = get_z_order();
        // Keep picking on the normal geometry so changing size cannot toggle hover.
        const auto &appearance = _frame.appearance;
        bool resize_handles = false;
        if (!_dragging && (_hover & _frame.enabled) != 0u)
        {
            if (_frame.operation == Operation::rotate) resize_handles = appearance.hover_rotation_geometry;
            else
            {
                const bool tip_resizes = _frame.operation == Operation::move ? appearance.hover_arrow_geometry
                                                                             : appearance.hover_scale_geometry;
                resize_handles = appearance.hover_shaft_geometry || tip_resizes;
            }
        }
        if (resize_handles && _hover_geometry_mask != _hover)
        {
            build_geometry(_hover_geometry, _hover);
            _hover_geometry_mask = _hover;
        }
        const auto &geometry = resize_handles ? _hover_geometry : _hit_geometry;
        constexpr u32 pivot_segments = 20u;
        constexpr u32 ring_segments = 64u;
        constexpr u32 ring_samples = 8u;
        const bool show_view_ring = has_view_ring();
        const bool show_drag_pivot = _dragging && _drag_handle && _frame.operation != Operation::rotate &&
                                     _frame.edge_direction == amal::vec2{} && appearance.drag_pivot_diameter > 0.f;
        const size_t vertex_count = 3 * geometry.faces.size() + 8 * geometry.segments.size() +
                                    (show_drag_pivot ? 1u + 2u * pivot_segments : 0u) +
                                    (show_view_ring ? ring_segments * ring_samples : 0u);
        const size_t index_count = 3 * geometry.faces.size() + 18 * geometry.segments.size() +
                                   (show_drag_pivot ? 9u * pivot_segments : 0u) +
                                   (show_view_ring ? ring_segments * (ring_samples - 1u) * 6u : 0u);
        if ((_vertices.size() != vertex_count || _indices.size() != index_count) &&
            _record.render_id != AUIK_INVALID_DRAW_DATA_ID)
            invalidate_render_draw(get_primary_vertex_stream(), _record);
        _vertices.clear();
        _indices.clear();
        const u32 active_handle = _dragging ? _drag_handle : _hover;
        const u32 enabled_handles = _frame.enabled | (_frame.enabled == 7u ? view_ring_handle : 0u);
        const u32 mask = _dragging ? _drag_mask : (_hover & (enabled_handles | edge_handle));
        const bool dim_peers = (mask & (7u | view_ring_handle)) != 0u && mask != 7u;
        const u32 dimmed_alpha = _dragging ? appearance.dimmed_alpha : appearance.hover_dimmed_alpha;
        const auto tint = [&](Palette color, u32 handle) {
            const bool axis_selected = std::has_single_bit(handle) && (mask & handle) != 0u;
            const bool handle_selected = handle != 0u && handle == active_handle && (handle & mask) != 0u;
            if (!axis_selected && !handle_selected)
            {
                if (!dim_peers || (handle & enabled_handles) == 0u) return color.normal;
                const u32 alpha = ((color.normal >> 24u) * dimmed_alpha + 127u) / 255u;
                return (color.normal & 0x00ffffffu) | (alpha << 24u);
            }
            if (_dragging) return color.active;
            return color.hover;
        };
        // Emit guides first so the solid handles cover them at the same widget depth.
        for (const auto &s : geometry.segments)
            if (!s.handle) append_segment(_vertices, _indices, s.a, s.b, s.width, depth, s.color.normal, clip_id());
        for (const auto &face : geometry.faces)
        {
            const auto color = tint(face.color, face.handle);
            const amal::vec2 points[] = {face.a, face.b, face.c};
            for (u32 i = 0; i < std::size(points); ++i)
            {
                const u32 vertex_color = face.fringe_mask & (1u << i) ? color & 0x00ffffffu : color;
                _indices.push_back(static_cast<u32>(_vertices.size()));
                append_vertex(_vertices, points[i], depth, vertex_color, clip_id());
            }
        }
        for (const auto &s : geometry.segments)
        {
            if (s.handle)
                append_segment(_vertices, _indices, s.a, s.b, s.width, depth, tint(s.color, s.handle), clip_id());
        }
        if (show_view_ring)
        {
            const f32 radius = appearance.rotation_radius + appearance.view_ring_gap;
            const f32 half_width = appearance.view_ring_width * 0.5f;
            const f32 outline_width = amal::max(0.f, appearance.view_ring_outline_width);
            const f32 inner = radius - half_width, outer = radius + half_width;
            u32 color = tint(appearance.view_ring, view_ring_handle);
            const u32 border_color = appearance.view_ring_outline_color;
            u32 outline = tint({border_color, border_color, border_color}, view_ring_handle);
            if (_frame.enabled != 7u)
            {
                color = (color & 0x00ffffffu) | appearance.locked_alpha;
                outline = (outline & 0x00ffffffu) |
                          ((((outline >> 24u) * (appearance.locked_alpha >> 24u) + 127u) / 255u) << 24u);
            }
            if (outline_width == 0.f) outline = color;
            // One continuous strip keeps the two fringes and the colored band from overlapping.
            const f32 transition = amal::min({0.35f, outline_width * 0.5f, half_width * 0.5f});
            const f32 radii[] = {inner - outline_width - 1.f, inner - outline_width,      inner - transition,
                                 inner + transition,          outer - transition,         outer + transition,
                                 outer + outline_width,       outer + outline_width + 1.f};
            const u32 transparent = outline & 0x00ffffffu;
            const u32 colors[] = {transparent, outline, outline, color, color, outline, outline, transparent};
            const u32 first = static_cast<u32>(_vertices.size());
            for (u32 i = 0; i < ring_segments; ++i)
            {
                const f32 angle = 2.f * amal::pi<f32>() * i / ring_segments;
                const amal::vec2 dir{amal::cos(angle), amal::sin(angle)};
                for (u32 j = 0; j < ring_samples; ++j)
                    append_vertex(_vertices, dir * amal::max(0.f, radii[j]), depth, colors[j], clip_id());
                const u32 current = first + ring_samples * i;
                const u32 next = first + ring_samples * ((i + 1u) % ring_segments);
                for (u32 j = 0; j + 1u < ring_samples; ++j)
                    for (u32 index :
                         {current + j, current + j + 1u, next + j, current + j + 1u, next + j + 1u, next + j})
                        _indices.push_back(index);
            }
        }
        if (show_drag_pivot)
        {
            const u32 color = std::has_single_bit(_drag_handle)
                                  ? appearance.axes[std::countr_zero(_drag_handle)].active
                                  : appearance.planes[std::countr_zero(7u ^ _drag_handle)].fill.active;
            const f32 radius = appearance.drag_pivot_diameter * 0.5f;
            const u32 center = static_cast<u32>(_vertices.size());
            append_vertex(_vertices, {}, depth, color, clip_id());
            for (u32 i = 0; i < pivot_segments; ++i)
            {
                const f32 angle = 2.f * amal::pi<f32>() * i / pivot_segments;
                const amal::vec2 dir{amal::cos(angle), amal::sin(angle)};
                append_vertex(_vertices, dir * radius, depth, color, clip_id());
                append_vertex(_vertices, dir * (radius + 1.f), depth, color & 0x00ffffffu, clip_id());
                const u32 inner = center + 1u + 2u * i;
                const u32 next = center + 1u + 2u * ((i + 1u) % pivot_segments);
                for (u32 index : {center, inner, next, inner, inner + 1u, next, next, inner + 1u, next + 1u})
                    _indices.push_back(index);
            }
        }
        _depth = depth;
        _clip = clip_id();
        _vertices_dirty = true;
    }

    void Manipulator::draw(DrawCtx &ctx)
    {
        auto *stream = get_primary_vertex_stream();
        // Layout can update the projection and draw before the record pass reaches this widget.
        if ((ctx.reason & DrawReasonBits::record) && _record.render_id != AUIK_INVALID_DRAW_DATA_ID)
            invalidate_render_draw(stream, _record);
        VertexStreamBatchData batch{_vertices.data(), _indices.data(), static_cast<u32>(_vertices.size()),
                                    static_cast<u32>(_indices.size()), position()};
        emit_vertex_stream_batch(ctx, stream, _record, batch, get_rect(), false, !_vertices_dirty);
        _vertices_dirty = false;
    }
} // namespace auik::guizmo
