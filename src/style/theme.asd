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
guizmo-manipulator-shaft:
    width: @(108px * @dpi)
    height: @(2.5px * @dpi)

guizmo-manipulator-arrow:
    width: @(10px * @dpi)
    height: @(15px * @dpi)

guizmo-manipulator-plane:
    width: @(16px * @dpi)
    radius: @(4px * @dpi)
    border-thickness: @(1px * @dpi)

guizmo-manipulator-plane-xy extends @guizmo-manipulator-plane:
    bg-color: (65, 130, 230, 0.25)
    border-color: (65, 130, 230, 0.85)
    @hover:
        bg-color: (71, 143, 253, 0.42)
        border-color: (71, 143, 253)

guizmo-manipulator-plane-xz extends @guizmo-manipulator-plane:
    bg-color: (130, 205, 35, 0.25)
    border-color: (130, 205, 35, 0.85)
    @hover:
        bg-color: (143, 225, 39, 0.42)
        border-color: (143, 225, 39)

guizmo-manipulator-plane-yz extends @guizmo-manipulator-plane:
    bg-color: (220, 67, 60, 0.25)
    border-color: (220, 67, 60, 0.85)
    @hover:
        bg-color: (242, 73, 66, 0.42)
        border-color: (242, 73, 66)

guizmo-manipulator-plane-locked extends @guizmo-manipulator-plane:
    bg-color: (133, 133, 133, 0.18)
    border-color: (133, 133, 133, 0.58)

guizmo-manipulator-x:
    bg-color: (220, 67, 60)
    @hover:
        bg-color: (242, 73, 66)

guizmo-manipulator-y:
    bg-color: (130, 205, 35)
    @hover:
        bg-color: (124, 211, 67)

guizmo-manipulator-z:
    bg-color: (65, 130, 230)
    @hover:
        bg-color: (71, 143, 253)

guizmo-manipulator-locked:
    bg-color: (133, 133, 133, 0.745)

guizmo-manipulator-guide:
    height: @(1px * @dpi)
    bg-color: (255, 255, 255, 0.565)

guizmo-manipulator-rotation:
    height: @(2.4px * @dpi)

guizmo-manipulator-scale:
    width: @(8px * @dpi)

guizmo-manipulator-edge:
    size: (@(14px * @dpi), @(13px * @dpi))
    padding: @(12px * @dpi)
    border-thickness: @(3px * @dpi)
    border-color: (238, 238, 238)
    @hover:
        border-color: (255, 255, 255)
