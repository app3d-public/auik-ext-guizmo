#include <amal/geometric.hpp>
#include <auik/auik.hpp>
#include <auik/ext/guizmo/manipulator.hpp>
#include <bit>
#include <cmath>
#include "line_geometry.hpp"

namespace auik::guizmo
{
    namespace
    {
        constexpr u32 colors[] = {detail::pack_rgba8(220, 67, 60, 255), detail::pack_rgba8(113, 192, 61, 255),
                                  detail::pack_rgba8(65, 130, 230, 255)};
        constexpr u32 disabled = detail::pack_rgba8(133, 133, 133, 190);
        amal::vec2 normal(amal::vec2 p) { return {-p.y, p.x}; }
    } // namespace

    Manipulator::Manipulator(u32 id)
        : Widget(id, WidgetFlagBits::visible | WidgetFlagBits::attachable, EventFlagBits::none, {{}, {1.f, 1.f}}, 0)
    {
    }

    bool Manipulator::project(const amal::mat4 &matrix, const amal::vec3 &pivot, const amal::rect &viewport,
                              const amal::rect &bounds, const Options &options)
    {
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
        frame.appearance = options.appearance;
        frame.appearance.reach = std::max(frame.appearance.reach, 1.f);
        auto inset = bounds.size * 0.5f - amal::vec2{12.f, 12.f};
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
            if (options.show_guides)
            {
                frame.guides = _drag_mask ? _drag_mask : frame.enabled;
                frame.guide_length = amal::length(bounds.size);
            }
        }
        place(point, frame);
        return true;
    }

    u32 Manipulator::begin_drag(const amal::vec2 &screen, bool drag_background)
    {
        auto hit = hit_handle(screen);
        _drag_mask = 0;
        if (hit == edge_handle) return hit;
        if ((hit & _frame.enabled) != hit) return 0;
        if (!hit && drag_background && _frame.edge_direction == amal::vec2{})
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
        _drag_frame = _frame;
        _drag_world_radius = _world_radius;
        _start = screen;
        _center = position();
        set_hover(hit);
        return hit;
    }

    bool Manipulator::drag_delta(const amal::vec2 &screen, amal::vec3 &result) const
    {
        if (!_drag_mask) return false;
        auto movement = screen - _start;
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
        if (_drag_mask) handle = _drag_mask;
        if (_hover == handle) return;
        _hover = handle;
        _dirty = true;
        if (is_attached() && is_visible()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
        mark_host_refresh_request();
    }

    void Manipulator::end_drag()
    {
        _drag_mask = 0;
        set_hover(0);
    }

    void Manipulator::place(const amal::vec2 &pivot, const Frame &frame)
    {
        const bool changed = _segments.empty() || !(_frame == frame);
        if (changed)
        {
            _frame = frame;
            build_segments();
            _dirty = true;
        }
        if (!changed && position() == pivot) return;
        set_position(pivot);
        if (is_attached()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
    }

    void Manipulator::build_segments()
    {
        _segments.clear();
        _faces.clear();
        auto line = [&](amal::vec2 a, amal::vec2 b, f32 width, u32 color, u32 mask) {
            if (amal::length(b - a) > 0.001f) _segments.push_back({a, b, width, color, mask});
        };
        if (_frame.edge_direction != amal::vec2{})
        {
            auto dir = _frame.edge_direction;
            for (float side : {-1.f, 1.f})
                line(-dir * 7.f + normal(dir) * (side * 7.f), dir * 6.f, 3.f, 0xffeeeeeeu, edge_handle);
            return;
        }
        const auto &appearance = _frame.appearance;
        for (u32 i = 0; i < 3; ++i)
        {
            u32 bit = 1u << i;
            const u32 j_axis = (i + 1) % 3, k_axis = (i + 2) % 3;
            auto oriented = [&](u32 index) {
                return (_frame.reversed_handles & (1u << index)) ? -_frame.axes[index] : _frame.axes[index];
            };
            auto axis = oriented(i);
            auto u = _frame.axes[j_axis], v = _frame.axes[k_axis];
            u32 color = (_frame.enabled & bit) ? colors[i] : disabled;
            if ((_frame.guides & bit) && amal::length(axis) > 1.f)
            {
                auto end = amal::normalize(axis) * _frame.guide_length;
                line(-end, end, 1.f, (color & 0xffffffu) | 0x90000000u, 0);
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
                    line(last, next, 2.4f, color, bit);
                    last = next;
                }
                continue;
            }
            const f32 axis_length = amal::length(axis);
            if (axis_length > appearance.center_radius + 4.f)
            {
                auto dir = amal::normalize(axis), n = normal(dir);
                const auto start = dir * appearance.center_radius;
                if (_frame.operation == Operation::move)
                {
                    const auto base =
                        axis - dir * std::min(appearance.arrow_size.y, axis_length - appearance.center_radius);
                    const auto shoulder = n * (appearance.arrow_size.x * 0.5f);
                    const auto left = base + shoulder, right = base - shoulder;
                    line(start, base, appearance.shaft_width, color, bit);
                    _faces.push_back({axis, left, right, color, bit});
                    line(axis, left, 0.5f, color, bit);
                    line(axis, right, 0.5f, color, bit);
                    line(left, right, 0.5f, color, bit);
                }
                else
                {
                    line(start, axis, appearance.shaft_width, color, bit);
                    line(axis - dir * 4.f, axis + dir * 4.f, 8.f, color, bit);
                    for (auto end : {axis - dir * 4.f, axis + dir * 4.f})
                        line(end - n * 4.f, end + n * 4.f, 0.5f, color, bit);
                }
            }
            // Offset plane handles leave the pivot and the axis tips accessible.
            u = oriented(j_axis);
            v = oriented(k_axis);
            u32 plane = 7u ^ bit;
            if (std::abs(u.x * v.y - u.y * v.x) < 100.f) continue;
            color = (_frame.enabled & plane) == plane ? colors[i] : disabled;
            const f32 half_size = appearance.plane_size * 0.5f;
            const f32 low = 0.46f - half_size / appearance.reach;
            const f32 high = 0.46f + half_size / appearance.reach;
            const f32 corner_radius = std::min(appearance.plane_rounding, half_size) / appearance.reach;
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
                    _faces.push_back({center, previous, point, color, plane});
                    line(previous, point, 0.5f, color, plane);
                    previous = point;
                }
        }
        if (_frame.operation != Operation::rotate)
        {
            auto previous = amal::vec2{appearance.center_radius, 0.f};
            for (int i = 1; i <= 40; ++i)
            {
                const f32 angle = i * (6.283185307f / 40.f);
                const auto point = amal::vec2{std::cos(angle), std::sin(angle)} * appearance.center_radius;
                line(previous, point, appearance.center_width, _frame.enabled ? appearance.center_color : disabled, 0);
                previous = point;
            }
        }
    }

    u32 Manipulator::pick(const amal::vec2 &screen) const
    {
        u32 hit = hit_handle(screen);
        return hit == edge_handle || (hit & _frame.enabled) == hit ? hit : 0u;
    }

    u32 Manipulator::hit_handle(const amal::vec2 &screen) const
    {
        auto p = screen - position();
        if (_frame.edge_direction == amal::vec2{} && _frame.operation != Operation::rotate &&
            amal::length(p) <= _frame.appearance.center_radius)
            return _frame.enabled;
        auto side = [](amal::vec2 a, amal::vec2 b) { return a.x * b.y - a.y * b.x; };
        for (const auto &face : _faces)
        {
            if (std::abs(side(face.b - face.a, face.c - face.a)) < 1e-5f) continue;
            auto a = side(face.b - face.a, p - face.a);
            auto b = side(face.c - face.b, p - face.b);
            auto c = side(face.a - face.c, p - face.c);
            // Include the shared diagonal even when rounding gives opposite edge tests.
            constexpr f32 tolerance = 0.001f;
            if (std::min({a, b, c}) >= -tolerance || std::max({a, b, c}) <= tolerance) return face.handle;
        }
        f32 best = 7.f;
        u32 hit = 0;
        for (auto &s : _segments)
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
            const size_t vertex_count = 3 * _faces.size() + 8 * _segments.size();
            const size_t index_count = 3 * _faces.size() + 18 * _segments.size();
            if ((_vertices.size() != vertex_count || _indices.size() != index_count) &&
                _record.render_id != AUIK_INVALID_DRAW_DATA_ID)
                invalidate_render_draw(get_primary_vertex_stream(), _record);
            _vertices.clear();
            _indices.clear();
            const auto tint = [this](u32 color, u32 handle) {
                const bool highlighted =
                    handle && (handle == _hover || (std::has_single_bit(handle) && (_hover & handle) != 0u));
                if (!highlighted) return color;
                auto boost = [](u32 channel) { return std::min(channel * 11u / 10u, 255u); };
                return detail::pack_rgba8(boost(color & 255u), boost((color >> 8u) & 255u),
                                          boost((color >> 16u) & 255u), color >> 24u);
            };
            for (const auto &face : _faces)
            {
                for (auto p : {face.a, face.b, face.c})
                {
                    _indices.push_back(static_cast<u32>(_vertices.size()));
                    _vertices.push_back({.position = p,
                                         .z_order = depth,
                                         .color = tint(face.color, face.handle),
                                         .clip_id = clip_id()});
                }
            }
            for (const auto &s : _segments)
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
