#include <amal/geometric.hpp>
#include <auik/auik.hpp>
#include <auik/ext/guizmo/manipulator.hpp>
#include <bit>
#include <cmath>

namespace auik::guizmo
{
    namespace
    {
        constexpr u32 colors[] = {detail::pack_rgba8(245, 79, 70, 255), detail::pack_rgba8(125, 212, 67, 255),
                                  detail::pack_rgba8(72, 143, 255, 255)};
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
        frame.enabled = options.enabled_axes & 7u;
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
            _world_radius = 72.f / std::max(std::sqrt(horizontal_squared), 1e-6f);
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
        if (!hit && drag_background && _frame.edge_direction == amal::vec2{}) hit = _frame.enabled;
        _drag_mask = hit;
        _drag_frame = _frame;
        _drag_world_radius = _world_radius;
        _start = screen;
        _center = position();
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
            if (std::has_single_bit(_drag_mask)) direction = axes[axis];
            if (amal::length(direction) < 6.f) direction = {1.f, -1.f};
            float factor = 1.f + amal::dot(movement, amal::normalize(direction)) / 72.f;
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
                radians = amal::dot(movement, amal::normalize(tangent)) / 72.f;
            }
            result[axis] = radians * 57.2957795f;
        }
        return true;
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
        for (u32 i = 0; i < 3; ++i)
        {
            u32 bit = 1u << i;
            auto axis = _frame.axes[i];
            auto u = _frame.axes[(i + 1) % 3], v = _frame.axes[(i + 2) % 3];
            u32 color = (_frame.enabled & bit) ? colors[i] : disabled;
            if ((_frame.guides & bit) && amal::length(axis) > 1.f)
            {
                auto end = amal::normalize(axis) * _frame.guide_length;
                line(-end, end, 1.f, (color & 0xffffffu) | 0x90000000u, 0);
            }
            if (_frame.operation == Operation::rotate)
            {
                auto last = u;
                for (int j = 1; j <= 72; ++j)
                {
                    f32 angle = j * (6.283185307f / 72.f);
                    auto next = u * std::cos(angle) + v * std::sin(angle);
                    line(last, next, 2.4f, color, bit);
                    last = next;
                }
                continue;
            }
            if (amal::length(axis) > 3.f)
            {
                auto dir = amal::normalize(axis), n = normal(dir);
                line(dir * 7.f, axis, 2.6f, color, bit);
                if (_frame.operation == Operation::move)
                {
                    auto base = axis - dir * 11.f;
                    _faces.push_back({axis, base + n * 4.6f, base - n * 4.6f, color, bit});
                }
                else line(axis - dir * 4.f, axis + dir * 4.f, 8.f, color, bit);
            }
            // Offset plane handles leave the pivot and the axis tips accessible.
            u32 plane = 7u ^ bit;
            if (std::abs(u.x * v.y - u.y * v.x) < 100.f) continue;
            color = (_frame.enabled & plane) == plane ? colors[i] : disabled;
            // Round the square in its own plane, then project the contour. Two stronger
            // outer edges preserve the L silhouette at oblique viewing angles.
            constexpr float low = 0.28f, high = 0.56f, radius = 5.f / 72.f;
            constexpr int arc_steps = 5;
            constexpr amal::vec2 corners[] = {{high - radius, high - radius},
                                              {low + radius, high - radius},
                                              {low + radius, low + radius},
                                              {high - radius, low + radius}};
            const auto center = (u + v) * ((high + low) * 0.5f);
            const u32 fill = (color & 0x00ffffffu) | 0x78000000u;
            auto previous = u * high + v * (low + radius);
            for (int corner = 0; corner < 4; ++corner)
                for (int step = 0; step <= arc_steps; ++step)
                {
                    const f32 angle = (corner + f32(step) / arc_steps) * 1.570796327f;
                    const auto uv = corners[corner] + amal::vec2{std::cos(angle), std::sin(angle)} * radius;
                    const auto point = u * uv.x + v * uv.y;
                    _faces.push_back({center, previous, point, fill, plane});
                    const bool outer = corner == 0 || (corner == 1 && step == 0);
                    line(previous, point, outer ? 2.0f : 1.0f, outer ? color : fill, plane);
                    previous = point;
                }
        }
        if (_frame.operation != Operation::rotate) line({-3, 0}, {3, 0}, 6, 0xffeeeeeeu, _frame.enabled);
    }

    u32 Manipulator::pick(const amal::vec2 &screen) const
    {
        u32 hit = hit_handle(screen);
        return hit == edge_handle || (hit & _frame.enabled) == hit ? hit : 0u;
    }

    u32 Manipulator::hit_handle(const amal::vec2 &screen) const
    {
        auto p = screen - position();
        auto side = [](amal::vec2 a, amal::vec2 b) { return a.x * b.y - a.y * b.x; };
        for (const auto &face : _faces)
        {
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
            const size_t vertex_count = 3 * _faces.size() + 4 * _segments.size();
            const size_t index_count = 3 * _faces.size() + 6 * _segments.size();
            if ((_vertices.size() != vertex_count || _indices.size() != index_count) &&
                _record.render_id != AUIK_INVALID_DRAW_DATA_ID)
                invalidate_render_draw(get_primary_vertex_stream(), _record);
            _vertices.clear();
            _indices.clear();
            for (const auto &face : _faces)
            {
                for (auto p : {face.a, face.b, face.c})
                {
                    _indices.push_back(static_cast<u32>(_vertices.size()));
                    _vertices.push_back({.position = p, .z_order = depth, .color = face.color, .clip_id = clip_id()});
                }
            }
            for (const auto &s : _segments)
            {
                auto n = normal(amal::normalize(s.b - s.a)) * (s.width * 0.5f);
                auto base = static_cast<u32>(_vertices.size());
                for (auto p : {s.a + n, s.a - n, s.b - n, s.b + n})
                    _vertices.push_back({.position = p, .z_order = depth, .color = s.color, .clip_id = clip_id()});
                for (u32 i : {0u, 1u, 2u, 0u, 2u, 3u}) _indices.push_back(base + i);
            }
            _depth = depth;
            _clip = clip_id();
            _dirty = false;
        }
        VertexStreamBatchData batch{_vertices.data(), _indices.data(), static_cast<u32>(_vertices.size()),
                                    static_cast<u32>(_indices.size()), position()};
        emit_vertex_stream_batch(ctx, get_primary_vertex_stream(), _record, batch, get_rect(), false, !rebuild);
    }
} // namespace auik::guizmo
