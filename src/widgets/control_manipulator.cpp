#include <amal/geometric.hpp>
#include <auik/auik.hpp>
#include <auik/detail/vertex_draw/lines.hpp>
#include <auik/ext/guizmo/control_manipulator.hpp>
#include <auik/ext/guizmo/widget_tags.hpp>
#include <bit>

namespace auik::guizmo
{
    ControlManipulator::ControlManipulator(u32 id, ManipulatorCreateFlags flags)
        : ManipulatorWidget(id), _axis_mask(flags & ManipulatorCreateFlagBits::all_axes)
    {
    }

    StyleUpdateFlags ControlManipulator::update_style()
    {
        auto *theme = get_theme();
        if (!theme) return {};
        auto style = [&](u32 tag, StyleState state = StyleState::normal) -> const Style & {
            return theme->get_style(theme->get_resolved_style(tag, 0u, 0u, state));
        };
        constexpr u32 control = AUIK_STYLE_TAG_GUIZMO_CONTROL;
        const auto &line = style(AUIK_STYLE_TAG_GUIZMO_CONTROL_LINE);
        _diameter = style(control).width();
        _color = style(control).background_color();
        _hover_color = style(control, StyleState::hover).background_color();
        _active_color = style(control, StyleState::active).background_color();
        _line_width = line.height();
        _line_color = line.background_color();
        rebuild_geometry();
        return {};
    }

    bool ControlManipulator::project(const amal::mat4 &matrix, const amal::vec3 &point, const amal::rect &viewport,
                                     const amal::rect &bounds, const ControlManipulatorOptions &options)
    {
        if (!std::has_single_bit(_axis_mask) || viewport.size.x <= 0.f || viewport.size.y <= 0.f) return false;
        if (_diameter <= 0.f) update_style();
        if (_diameter <= 0.f) return false;
        const auto clip = matrix * amal::vec4(point, 1.f);
        if (clip.w <= 1e-6f || clip.z < 0.f || clip.z > clip.w) return false;
        const auto half = viewport.size * 0.5f;
        const amal::vec2 ndc = amal::vec2{clip.x, clip.y} / clip.w;
        const auto screen = viewport.offset + half + ndc * half;
        const auto relative = screen - bounds.offset;
        const f32 radius = _diameter * 0.5f;
        if (relative.x < radius || relative.y < radius ||
            relative.x > bounds.size.x - radius || relative.y > bounds.size.y - radius) return false;
        const auto &column = matrix[std::countr_zero(_axis_mask)];
        _projected_axis = (amal::vec2{column.x, column.y} - ndc * column.w) * half / clip.w;
        std::optional<amal::vec2> anchor;
        if (options.line_anchor)
        {
            auto target = matrix * amal::vec4(*options.line_anchor, 1.f);
            if (target.z < 0.f) target = clip + (target - clip) * (clip.z / (clip.z - target.z));
            if (target.w > 1e-6f)
                anchor = (amal::vec2{target.x, target.y} / target.w - ndc) * half;
        }
        const bool had_line = _anchor && (_dragging || _line_visibility == ControlLineVisibility::always);
        const bool has_line = anchor && (_dragging || options.line_visibility == ControlLineVisibility::always);
        const bool line_changed = had_line != has_line || (has_line && _anchor != anchor);
        _anchor = anchor;
        _line_visibility = options.line_visibility;
        if (line_changed) rebuild_geometry();
        if (position() != screen)
        {
            set_position(screen);
            refresh_visual(false);
        }
        return true;
    }

    bool ControlManipulator::pick(amal::vec2 screen) const
    {
        return _diameter > 0.f && amal::length(screen - position()) <= _diameter * 0.5f;
    }

    void ControlManipulator::set_hover(bool hovered)
    {
        if (_pressed || _hovered == hovered) return;
        _hovered = hovered;
        rebuild_geometry();
    }

    bool ControlManipulator::begin_drag(amal::vec2 screen)
    {
        if (!pick(screen) || amal::length(_projected_axis) <= 1e-6f) return false;
        _press = screen;
        _drag_axis = _projected_axis;
        set_hover(true);
        _pressed = true;
        return true;
    }

    bool ControlManipulator::drag_delta(amal::vec2 screen, f32 &delta)
    {
        if (!_pressed) return false;
        const auto movement = screen - _press;
        if (!_dragging && movement != amal::vec2{})
        {
            _dragging = true;
            rebuild_geometry();
        }
        delta = amal::dot(movement, _drag_axis) / amal::dot(_drag_axis, _drag_axis);
        return true;
    }

    void ControlManipulator::end_drag()
    {
        const bool refresh = _hovered || _dragging;
        _pressed = _hovered = _dragging = false;
        if (refresh && !(auik::detail::get_context().dirty_flags & DirtyFlagBits::destroying)) rebuild_geometry();
    }

    void ControlManipulator::rebuild_geometry()
    {
        const bool line = _anchor && _line_width > 0.f && amal::length(*_anchor) > 0.001f &&
                          (_dragging || _line_visibility == ControlLineVisibility::always);
        const u32 circles = line && _dragging ? 2u : 1u;
        prepare_geometry(circles * (1u + 2u * disk_segments) + (line ? 8u : 0u),
                         circles * 9u * disk_segments + (line ? 18u : 0u));
        const f32 radius = _diameter * 0.5f;
        if (line)
        {
            auik::detail::append_segment(_vertices, _indices, {}, *_anchor, _line_width, get_z_order(), _line_color,
                                         clip_id());
            if (_dragging) append_disk(*_anchor, radius, _line_color);
        }
        append_disk({}, radius, _dragging ? _active_color : _hovered ? _hover_color : _color);
        refresh_visual(true);
    }
}
