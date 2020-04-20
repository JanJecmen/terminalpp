#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    /** Vertical Stack layout. 
     
        Stacks the widgets next to each other using the entire height of the parent. 
     */
    class RowLayout : public Layout {
    public:

        explicit RowLayout(HorizontalAlign hAlign = HorizontalAlign::Left, VerticalAlign vAlign = VerticalAlign::Middle):
            hAlign_{hAlign},
            vAlign_{vAlign} {
        }

        HorizontalAlign horizontalAlign() const {
            return hAlign_;
        }

        virtual void setHorizontalAlign(HorizontalAlign value) {
            if (hAlign_ != value) {
                hAlign_ = value;
            }
        }

        VerticalAlign verticalAlign() const {
            return vAlign_;
        }

        virtual void setVerticalAlign(VerticalAlign value) {
            if (vAlign_ != value) {
                vAlign_ = value;
            }
        }

    protected:

        int getStartX(int actualWidth, int fullWidth) {
            switch (hAlign_) {
                case HorizontalAlign::Left:
                    return 0;
                case HorizontalAlign::Center:
                    return (fullWidth - actualWidth) / 2;
                case HorizontalAlign::Right:
                    return fullWidth - actualWidth;
                default:
                    UNREACHABLE;
                    return 0;
            }
        }

        void relayout(Container * widget, Canvas const & contentsCanvas) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            ASSERT(! children.empty());
            int autoHeight = contentsCanvas.height();
            int availableWidth = contentsCanvas.width();
            // determine fixed sized widgets and remove them from the available width
            // determine the actual width that will be used and set the height of the child to the autoHeight
            int autoElements = 0;
            for (Widget * child : children) {
                int h = calculateChildHeight(child, autoHeight, autoHeight);
                setChildRect(child, Rect::FromTopLeftWH(0,0, child->width(), h));
                // now that the height of the child is correct, we determine its width if fixed
                if (child->widthHint() == SizeHint::Kind::Fixed)
                    availableWidth -= child->width();
                else if (child->widthHint() == SizeHint::Kind::Auto)
                    ++autoElements;
            }
            int autoWidth = availableWidth / static_cast<int>(children.size());
            // once we know the available width, calculate the actual width by doing a dry run
            int actualWidth = 0;
            for (Widget * child : children)
                actualWidth += calculateChildWidth(child, autoWidth, availableWidth);
            // determine the difference if the autoWidth elements won't fit the whole width precisely (and if there are elements whose size can actually be adjusted)
            int diff = (autoElements != 0 && availableWidth > actualWidth) ? availableWidth - actualWidth : 0;
            actualWidth += diff;
            // get the left placement
            int left = getStartX(actualWidth, contentsCanvas.width());
            // and finally set the widget sizes appropriately, centering them horizontally, apply the diff to the first non-fixed element
            for (Widget * child : children) {
                int w = calculateChildWidth(child, autoWidth, availableWidth);
                int h = child->height();
                if (diff > 0 && child->widthHint() == SizeHint::Kind::Auto) {
                    w += diff;
                    diff = 0;
                }
                Rect r{Rect::FromTopLeftWH(left, 0, w, h)};
                r = align(r, autoHeight, vAlign_);
                setChildRect(child, r);
                setChildOverlay(child, false);
                left += w;
            }
        }

        /** Stacked widgets have no overlay. 
         */
        void recalculateOverlay(Container * widget) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            for (auto i = children.rbegin(), e = children.rend(); i != e; ++i)
                setChildOverlay(*i, false);
        }

        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;
    }; // ui::RowLayout



} // namespace ui