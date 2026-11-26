#include <amal/geometric.hpp>
#include <amal/trigonometric.hpp>
#include <auik/auik.hpp>
#include <auik/detail/vertex_draw/lines.hpp>
#include <auik/ext/guizmo/manipulator.hpp>

namespace auik::guizmo::detail
{
    using auik::detail::append_vertex;

    ManipulatorWidget::ManipulatorWidget(u32 id)
        : Widget(id, WidgetFlagBits::visible | WidgetFlagBits::attachable, EventFlagBits::none, {{}, {1.f, 1.f}}, 0)
    {
    }

    void ManipulatorWidget::prepare_geometry(size_t vertex_count, size_t index_count)
    {
        if ((_vertices.size() != vertex_count || _indices.size() != index_count) &&
            _record.render_id != AUIK_INVALID_DRAW_DATA_ID)
            invalidate_render_draw(get_primary_vertex_stream(), _record);
        _vertices.clear();
        _indices.clear();
        _depth = get_z_order();
        _clip = clip_id();
        _vertices_dirty = true;
    }

    void ManipulatorWidget::refresh_visual(bool request_refresh)
    {
        if (is_attached() && is_visible()) redraw_external(_record.render_id != AUIK_INVALID_DRAW_DATA_ID);
        if (request_refresh) mark_host_refresh_request();
    }

    void ManipulatorWidget::reset_draw_records()
    {
        Widget::reset_draw_records();
        _record = {};
    }

    void ManipulatorWidget::set_clip_id(u16 id)
    {
        Widget::set_clip_id(id);
        sync_vertex_placement();
    }

    void ManipulatorWidget::rebuild_clip_rects() { sync_vertex_placement(); }

    void ManipulatorWidget::update_depth(const amal::vec2 &depth_range)
    {
        Widget::update_depth(depth_range);
        sync_vertex_placement();
    }

    void ManipulatorWidget::sync_vertex_placement()
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

    void ManipulatorWidget::append_disk(amal::vec2 point, f32 radius, u32 color)
    {
        const auto depth = get_z_order();
        const u32 center = static_cast<u32>(_vertices.size());
        append_vertex(_vertices, point, depth, color, clip_id());
        for (u32 i = 0; i < disk_segments; ++i)
        {
            const f32 angle = 2.f * amal::pi<f32>() * i / disk_segments;
            const amal::vec2 dir{amal::cos(angle), amal::sin(angle)};
            append_vertex(_vertices, point + dir * radius, depth, color, clip_id());
            append_vertex(_vertices, point + dir * (radius + 1.f), depth, color & 0x00ffffffu, clip_id());
            const u32 inner = center + 1u + 2u * i;
            const u32 next = center + 1u + 2u * ((i + 1u) % disk_segments);
            for (u32 index : {center, inner, next, inner, inner + 1u, next, next, inner + 1u, next + 1u})
                _indices.push_back(index);
        }
    }

    void ManipulatorWidget::draw(DrawCtx &ctx)
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
}
