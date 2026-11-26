#pragma once

#include <acul/enum.hpp>
#include <amal/matrix.hpp>
#include <array>
#include <auik-ext-guizmo/symbol_export.h>
#include <auik/pipelines.hpp>
#include <auik/widgets/widget.hpp>
#include <optional>

namespace auik::guizmo
{
    struct ManipulatorCreateFlagBits
    {
        enum enum_type : u32
        {
            none = 0,
            axis_x = 1u << 0,
            axis_y = 1u << 1,
            axis_z = 1u << 2,
            axis_guides = 1u << 3,
            out_of_bounds_marker = 1u << 4,
            all_axes = axis_x | axis_y | axis_z
        };
        using flag_bitmask = std::true_type;
    };
    using ManipulatorCreateFlags = acul::flags<ManipulatorCreateFlagBits>;

    namespace detail
    {
        class AUIK_EXT_GUIZMO_EXPORT ManipulatorWidget : public Widget
        {
        public:
            void set_clip_id(u16 id);
            void rebuild_clip_rects() override;
            void update_depth(const amal::vec2 &depth_range) override;
            void draw(DrawCtx &ctx) override;
            void reset_draw_records() override;

        protected:
            explicit ManipulatorWidget(u32 id);
            static constexpr u32 disk_segments = 20;
            void append_disk(amal::vec2 center, f32 radius, u32 color);
            void prepare_geometry(size_t vertex_count, size_t index_count);
            void refresh_visual(bool request_refresh);
            DrawDataID _record{};
            acul::vector<VertexStreamVertex> _vertices;
            acul::vector<VertexStreamIndex> _indices;

        private:
            f32 _depth = -1;
            u32 _clip = ~0u;
            bool _vertices_dirty = true;
            void sync_vertex_placement();
        };
    }
} // namespace auik::guizmo
