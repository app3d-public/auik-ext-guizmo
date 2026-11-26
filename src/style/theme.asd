guizmo-grid:
    margin: 0px
    size: (fill, fill)
    min-size: (100px, 100px)
    bg-color: @color-surface-dark
    border: 1px @color-border
    radius: 10px

guizmo-grid-line:
    size: (1px, 1px)
    bg-color: (255, 255, 255, 0.1)

guizmo-grid-baseline:
    size: (1px, 1px)
    bg-color: (255, 255, 255, 0.35)

guizmo-grid-label:
    margin: 4px
    size: (fit, fit)
    color: (255, 255, 255, 0.65)
    font-size: 9pt

guizmo-grid-label-background:
    bg-color: (31, 31, 31)

guizmo-curve:
    margin: 0px
    padding: 0px
    size: (fill, fill)
    min-size: (100px, 100px)
    bg-color: (31, 31, 31)

guizmo-curve-line:
    height: 2px
    bg-color: @color-accent

guizmo-control-curve-line:
    height: 2px
    bg-color: (255, 255, 255, 0.35)

guizmo-control-curve-point:
    size: (10px, 10px)
    radius: 5px
    bg-color: (230, 230, 230)
    @hover:
        bg-color: (255, 191, 66)
    @active:
        bg-color: (255, 191, 66)

guizmo-control-curve-handle:
    size: (8px, 8px)
    radius: 4px
    bg-color: (204, 204, 204)
    @hover:
        bg-color: (255, 191, 66)
    @active:
        bg-color: (255, 191, 66)

@guizmo-x-hover: (246, 91, 83)
@guizmo-y-hover: (142, 228, 94)
@guizmo-z-hover: (89, 153, 250)

guizmo-manipulator-shaft:
    width: @(120px * @dpi)
    height: 2px

guizmo-manipulator-arrow:
    width: @(10px * @dpi)
    height: @(15px * @dpi)

guizmo-manipulator-plane:
    width: @(16px * @dpi)
    radius: @(4px * @dpi)

guizmo-manipulator-plane-xy extends @guizmo-manipulator-plane:
    bg-color: (65, 130, 230, 0.9)
    @hover:
        bg-color: @guizmo-z-hover
        border-color: @guizmo-z-hover
    @active:
        bg-color: (89, 153, 250, 0.94)

guizmo-manipulator-plane-xz extends @guizmo-manipulator-plane:
    bg-color: (120, 210, 70, 0.9)
    @hover:
        bg-color: @guizmo-y-hover
        border-color: @guizmo-y-hover
    @active:
        bg-color: (142, 228, 94, 0.94)

guizmo-manipulator-plane-yz extends @guizmo-manipulator-plane:
    bg-color: (220, 67, 60, 0.9)
    @hover:
        bg-color: @guizmo-x-hover
        border-color: @guizmo-x-hover
    @active:
        bg-color: (246, 91, 83, 0.94)

guizmo-manipulator-plane-locked extends @guizmo-manipulator-plane:
    bg-color: (133, 133, 133, 0.12)
    border-color: (133, 133, 133, 0.38)

guizmo-manipulator-x:
    bg-color: (220, 67, 60)
    @hover:
        bg-color: @guizmo-x-hover
    @active:
        bg-color: (246, 91, 83)

guizmo-manipulator-y:
    bg-color: (120, 210, 70)
    @hover:
        bg-color: @guizmo-y-hover
    @active:
        bg-color: (142, 228, 94)

guizmo-manipulator-z:
    bg-color: (65, 130, 230)
    @hover:
        bg-color: @guizmo-z-hover
    @active:
        bg-color: (89, 153, 250)

guizmo-manipulator-dimmed:
    bg-color: (255, 255, 255, 0.52)
    @hover:
        bg-color: (255, 255, 255, 0.76)

guizmo-manipulator-locked:
    bg-color: (255, 255, 255, 0.52)

guizmo-manipulator-drag-pivot:
    width: @(8px * @dpi)

guizmo-manipulator-guide:
    height: @(1px * @dpi)

guizmo-manipulator-guide-x extends @guizmo-manipulator-guide:
    bg-color: (230, 85, 85, 0.8)

guizmo-manipulator-guide-y extends @guizmo-manipulator-guide:
    bg-color: (140, 230, 80, 0.8)

guizmo-manipulator-guide-z extends @guizmo-manipulator-guide:
    bg-color: (80, 140, 230, 0.8)

guizmo-manipulator-rotation:
    width: @(70px * @dpi)
    height: @(2px * @dpi)
    @hover:
        height: @(2.5px * @dpi)

guizmo-manipulator-view-ring:
    height: @(1.5px * @dpi)
    padding: @(11px * @dpi)
    border-thickness: 1px
    border-color: (180, 180, 180, 0.90)
    bg-color: (240, 240, 240)
    @hover:
        bg-color: (255, 255, 255)
    @active:
        bg-color: (255, 255, 255)

guizmo-manipulator-rotation-sector:
    bg-color: (245, 245, 245, 0.80)

guizmo-manipulator-rotation-sector-x extends @guizmo-manipulator-rotation-sector:
    bg-color: (246, 91, 83, 0.60)

guizmo-manipulator-rotation-sector-y extends @guizmo-manipulator-rotation-sector:
    bg-color: (142, 228, 94, 0.60)

guizmo-manipulator-rotation-sector-z extends @guizmo-manipulator-rotation-sector:
    bg-color: (89, 153, 250, 0.60)

guizmo-manipulator-scale:
    width: @(8px * @dpi)

guizmo-manipulator-edge:
    size: (@(13px * @dpi), @(12px * @dpi))
    radius: @(2.5px * @dpi)
    padding: @(12px * @dpi)
    bg-color: (72, 114, 255, 0.74)
    @hover:
        bg-color: (91, 130, 255, 0.9)

guizmo-control:
    width: @(10px * @dpi)
    bg-color: (231, 202, 97)
    @hover:
        bg-color: (255, 240, 173)
    @active:
        bg-color: (255, 184, 66)

guizmo-control-line:
    height: @(1px * @dpi)
    bg-color: (231, 202, 97, 0.85)
