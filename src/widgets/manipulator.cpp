#include <amal/geometric.hpp>
#include <auik/auik.hpp>
#include <auik/ext/guizmo/manipulator.hpp>
#include <auik/ext/guizmo/widget_tags.hpp>
#include <bit>
#include <cmath>
#include <utility>
#include "line_geometry.hpp"

namespace auik::guizmo
{
    namespace
    {
        amal::vec2 normal(amal::vec2 p) { return {-p.y, p.x}; }
    } // namespace

    Manipulator::Manipulator(u32 id)
        : Widget(id, WidgetFlagBits::visible | WidgetFlagBits::attachable, EventFlagBits::none, {{}, {1.f, 1.f}}, 0)
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
        appearance.guide_colors = {style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE_X).background_color(),
                                   style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE_Y).background_color(),
                                   style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_GUIDE_Z).background_color()};
        const auto &rotation = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION);
        const auto &rotation_hover = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_ROTATION, StyleState::hover);
        const auto &scale = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_SCALE);
        const auto &scale_hover = style(AUIK_STYLE_TAG_GUIZMO_MANIPULATOR_SCALE, StyleState::hover);
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

    bool Manipulator::project_frame(const Projection &projection)
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
        frame.enabled = options.enabled_axes & 7u;
        if (frame.operation != Operation::rotate)
        {
            frame.reversed_handles = options.reversed_handles;
            for (u32 axis = 0; axis < 3; ++axis)
                if (options.view_direction[axis] < 0.f) frame.reversed_handles ^= 1u << axis;
        }
        if (!_appearance) _appearance = read_appearance();
        frame.appearance = *_appearance;
        if (frame.appearance.reach <= 0.f) return false;
        auto inset = bounds.size * 0.5f - amal::vec2{frame.appearance.edge_padding};
        if (inset.x <= 0.f || inset.y <= 0.f) return false;
        auto edge_center = bounds.offset + bounds.size * 0.5f;
        auto direction = point - edge_center;
        if (clip.w <= 0 || std::abs(direction.x) > inset.x || std::abs(direction.y) > inset.y)
        {
            if (!options.show_offscreen) return false;
            if (amal::length(direction) < 0.01f) direction = {0.f, -1.f};
            frame.edge_direction = amal::normalize(direction);
            point =
                edge_center + direction / std::max(std::abs(direction.x) / inset.x, std::abs(direction.y) / inset.y);
        }
        else
        {
            // Differentiate perspective division at the pivot to get pixels per world unit.
            float horizontal_squared = 0;
            for (int i = 0; i < 3; ++i)
            {
                auto column = matrix[i];
                frame.axes[i] = (amal::vec2{column.x, column.y} - ndc * column.w) * half / w;
                horizontal_squared += frame.axes[i].x * frame.axes[i].x;
            }
            _world_radius = frame.appearance.reach / std::max(std::sqrt(horizontal_squared), 1e-6f);
            for (auto &axis : frame.axes)
            {
                axis *= _world_radius;
                // Subpixel noise during a translation need not invalidate the vertex stream.
                axis.x = std::round(axis.x * 64.f) / 64.f;
                axis.y = std::round(axis.y * 64.f) / 64.f;
            }
            if (_dragging && _drag_mask != 7u)
            {
                frame.guides = _drag_mask;
                frame.guide_length = amal::length(bounds.size);
            }
        }
        place(point, frame);
        return true;
    }

    u32 Manipulator::begin_drag(const amal::vec2 &screen, bool drag_background)
    {
        if (_drag_mask) end_drag();
        const auto handle = hit_handle(screen);
        if (handle == edge_handle) return handle;
        auto hit = handle & _frame.enabled;
        if (handle && !hit) return 0;
        if (!handle && drag_background && _frame.edge_direction == amal::vec2{})
        {
            hit = _frame.enabled;
            if (_frame.operation == Operation::rotate)
            {
                f32 best = -1.f;
                for (u32 i = 0; i < 3; ++i)
                    if ((_frame.enabled & (1u << i)) && std::abs(_frame.view_direction[i]) > best)
                    {
                        best = std::abs(_frame.view_direction[i]);
                        hit = 1u << i;
                    }
            }
        }
        _drag_mask = hit;
        _drag_handle = handle;
        _drag_frame = _frame;
        _drag_world_radius = _world_radius;
        _start = screen;
        _center = position();
        set_hover(hit);
        return hit;
    }

    bool Manipulator::drag_delta(const amal::vec2 &screen, amal::vec3 &result)
    {
        if (!_drag_mask) return false;
        auto movement = screen - _start;
        if (!_dragging && movement != amal::vec2{})
        {
            _dragging = true;
            _dirty = true;
            if (_projection) project_frame(*_projection);
        }
        result = {};
        auto axes = _drag_frame.axes;
        u32 axis = std::countr_zero(_drag_mask);
        if (_drag_frame.operation == Operation::move)
        {
            // Least-squares inverse of the allowed projected axes. One-dimensional
            // projections (including an edge-on plane) use the visible direction.
            float xx = 0, xy = 0, yy = 0;
            for (u32 i = 0; i < 3; ++i)
                if (_drag_mask & (1u << i))
                {
                    xx += axes[i].x * axes[i].x;
                    xy += axes[i].x * axes[i].y;
                    yy += axes[i].y * axes[i].y;
                }
            float determinant = xx * yy - xy * xy;
            auto inverse = movement / std::max(xx + yy, 1.f);
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
            float factor = 1.f + amal::dot(movement, amal::normalize(direction)) / _drag_frame.appearance.reach;
            for (u32 i = 0; i < 3; ++i)
                if (_drag_mask & (1u << i)) result[i] = factor;
        }
        else
        {
            auto u = axes[(axis + 1) % 3], v = axes[(axis + 2) % 3];
            auto cross = [](amal::vec2 a, amal::vec2 b) { return a.x * b.y - a.y * b.x; };
            auto start = _start - _center, end = start + movement;
            float radians;
            if (std::abs(cross(u, v)) > 150.f)
                radians = std::atan2(cross(u, end), cross(end, v)) - std::atan2(cross(u, start), cross(start, v));
            else
            {
                auto tangent = amal::length(u) > amal::length(v) ? u : v;
                radians = amal::dot(movement, amal::normalize(tangent)) / _drag_frame.appearance.reach;
            }
            result[axis] = radians * 57.2957795f;
        }
        return true;
    }

    void Manipulator::set_hover(u32 handle)
    {
        if (_drag_mask) handle = _drag_handle ? _drag_handle : _drag_mask;
        if (_hover == handle) return;
        _hover = handle;
        _dirty = true;
        if (is_attached() && is_visible()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
        mark_host_refresh_request();
    }

    void Manipulator::end_drag()
    {
        _drag_mask = 0;
        _drag_handle = 0;
        const bool was_dragging = std::exchange(_dragging, false);
        if (_frame.guides)
        {
            _frame.guides = 0;
            _frame.guide_length = 0.f;
            build_geometry(_hit_geometry);
            _hover_geometry_mask = 0u;
        }
        if (std::exchange(_hover, 0u) || was_dragging)
        {
            _dirty = true;
            if (detail::get_context().dirty_flags & DirtyFlagBits::destroying) return;
            if (is_attached() && is_visible()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
            mark_host_refresh_request();
        }
    }

    void Manipulator::place(const amal::vec2 &pivot, const Frame &frame)
    {
        const bool changed = (_hit_geometry.segments.empty() && _hit_geometry.faces.empty()) || !(_frame == frame);
        if (changed)
        {
            _frame = frame;
            build_geometry(_hit_geometry);
            _hover_geometry_mask = 0u;
            _dirty = true;
        }
        if (!changed && !_dirty && position() == pivot) return;
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
            const amal::vec2 corners[] = {tip, -tip + normal(dir) * half.x, -tip - normal(dir) * half.x};
            const f32 rounding = std::clamp(appearance.edge_rounding, 0.f, std::min(half.x, half.y) * 0.5f);
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
                    contour[count++] = (1.f - t) * ((1.f - t) * start + t * corner) +
                                       t * ((1.f - t) * corner + t * end);
                }
            }
            // Fade outside the contour without drawing a second layer over the translucent fill.
            const auto outer = [&](size_t i) {
                const auto p = contour[i];
                const auto before = amal::normalize(normal(contour[(i + count - 1) % count] - p));
                const auto after = amal::normalize(normal(p - contour[(i + 1) % count]));
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
        for (u32 i = 0; i < 3; ++i)
        {
            u32 bit = 1u << i;
            const u32 j_axis = (i + 1) % 3, k_axis = (i + 2) % 3;
            auto oriented = [&](u32 index) {
                return (_frame.reversed_handles & (1u << index)) ? -_frame.axes[index] : _frame.axes[index];
            };
            auto axis = oriented(i);
            const bool axis_hovered = (hover & _frame.enabled & bit) != 0u;
            const f32 shaft_width = axis_hovered ? appearance.hover_shaft_width : appearance.shaft_width;
            auto u = _frame.axes[j_axis], v = _frame.axes[k_axis];
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
            if (_frame.operation == Operation::rotate)
            {
                // Start at the silhouette and traverse the half-ring on the far side of the pivot.
                const f32 a = _frame.view_direction[j_axis], b = _frame.view_direction[k_axis];
                const bool face_on = a * a + b * b < 1e-6f;
                const f32 start = face_on ? 0.f : std::atan2(b, a) - 1.570796327f;
                const f32 sweep = face_on ? 6.283185307f : 3.141592654f;
                auto last = u * std::cos(start) + v * std::sin(start);
                for (int j = 1; j <= 72; ++j)
                {
                    f32 angle = start + j * (sweep / 72.f);
                    auto next = u * std::cos(angle) + v * std::sin(angle);
                    line(last, next, axis_hovered ? appearance.hover_rotation_width : appearance.rotation_width, color,
                         bit);
                    last = next;
                }
                continue;
            }
            if (axis_hovered) axis *= appearance.hover_reach / appearance.reach;
            const f32 axis_length = amal::length(axis);
            if (axis_length > 4.f)
            {
                auto dir = amal::normalize(axis), n = normal(dir);
                if (_frame.operation == Operation::move)
                {
                    const auto size = axis_hovered ? appearance.hover_arrow_size : appearance.arrow_size;
                    const auto base = axis - dir * std::min(size.y, axis_length);
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
            // Offset plane handles leave the pivot and the axis tips accessible.
            u = oriented(j_axis);
            v = oriented(k_axis);
            u32 plane = 7u ^ bit;
            if (std::abs(u.x * v.y - u.y * v.x) < 100.f) continue;
            const auto &plane_style = (_frame.enabled & plane) ? appearance.planes[i] : appearance.locked_plane;
            const auto &size = plane_style.size;
            const f32 half_size = size.width * 0.5f;
            if (half_size <= 0.f) continue;
            const f32 low = 0.46f - half_size / appearance.reach;
            const f32 high = 0.46f + half_size / appearance.reach;
            const f32 corner_radius = std::clamp(size.rounding, 0.f, half_size) / appearance.reach;
            constexpr int arc_steps = 5;
            const amal::vec2 corners[] = {{high - corner_radius, high - corner_radius},
                                          {low + corner_radius, high - corner_radius},
                                          {low + corner_radius, low + corner_radius},
                                          {high - corner_radius, low + corner_radius}};
            const auto center = (u + v) * ((high + low) * 0.5f);

            auto previous = u * high + v * (low + corner_radius);
            for (int corner = 0; corner < 4; ++corner)
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

    u32 Manipulator::pick(const amal::vec2 &screen) const
    {
        u32 hit = hit_handle(screen);
        return hit == edge_handle || (hit & _frame.enabled) != 0u ? hit : 0u;
    }

    u32 Manipulator::hit_handle(const amal::vec2 &screen) const
    {
        auto p = screen - position();
        auto side = [](amal::vec2 a, amal::vec2 b) { return a.x * b.y - a.y * b.x; };
        for (const auto &face : _hit_geometry.faces)
        {
            if (face.fringe_mask || std::abs(side(face.b - face.a, face.c - face.a)) < 1e-5f) continue;
            auto a = side(face.b - face.a, p - face.a);
            auto b = side(face.c - face.b, p - face.b);
            auto c = side(face.a - face.c, p - face.c);
            // Include the shared diagonal even when rounding gives opposite edge tests.
            constexpr f32 tolerance = 0.001f;
            if (std::min({a, b, c}) >= -tolerance || std::max({a, b, c}) <= tolerance) return face.handle;
        }
        if (_frame.edge_direction != amal::vec2{} && !_hit_geometry.faces.empty() && amal::length(p) < 7.f)
            return edge_handle;
        f32 best = 7.f;
        u32 hit = 0;
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

    void Manipulator::draw(DrawCtx &ctx)
    {
        const auto depth = get_z_order();
        bool rebuild = _dirty || _depth != depth || _clip != clip_id();
        if (rebuild)
        {
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
            const size_t vertex_count = 3 * geometry.faces.size() + 8 * geometry.segments.size();
            const size_t index_count = 3 * geometry.faces.size() + 18 * geometry.segments.size();
            if ((_vertices.size() != vertex_count || _indices.size() != index_count) &&
                _record.render_id != AUIK_INVALID_DRAW_DATA_ID)
                invalidate_render_draw(get_primary_vertex_stream(), _record);
            _vertices.clear();
            _indices.clear();
            const u32 active_handle = _dragging ? _drag_handle : _hover;
            const u32 mask = _dragging ? _drag_mask : (_hover & (_frame.enabled | edge_handle));
            const bool dim_peers = (mask & 7u) != 0u && mask != 7u;
            const u32 dimmed_alpha = _dragging ? appearance.dimmed_alpha : appearance.hover_dimmed_alpha;
            const auto tint = [&](Palette color, u32 handle) {
                const bool axis_selected = std::has_single_bit(handle) && (mask & handle) != 0u;
                const bool handle_selected = handle != 0u && handle == active_handle && (handle & mask) != 0u;
                if (!axis_selected && !handle_selected)
                {
                    if (!dim_peers || (handle & _frame.enabled) == 0u) return color.normal;
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
                if (s.handle)
                    append_segment(_vertices, _indices, s.a, s.b, s.width, depth, tint(s.color, s.handle), clip_id());
            _depth = depth;
            _clip = clip_id();
            _dirty = false;
        }
        VertexStreamBatchData batch{_vertices.data(), _indices.data(), static_cast<u32>(_vertices.size()),
                                    static_cast<u32>(_indices.size()), position()};
        emit_vertex_stream_batch(ctx, get_primary_vertex_stream(), _record, batch, get_rect(), false, !rebuild);
    }
} // namespace auik::guizmo
