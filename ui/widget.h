#pragma once

#include "helpers/helpers.h"

#include "canvas.h"

namespace ui {

	/**

	    Generally, all getters are public
	
	    Events:

		onMouseClick
		onMouseDown
		onMouseUp
		onMouseMove
		onMouseEnter
		onMouseLeave
		onKeyDown
		onKeyUp
		onChar
		onFocusIn
		onFocusOut
		onPaste
		onSizeChange,
		onPositionChange
	    

	
	 */

	template<typename T>
	using Event = helpers::Event<T>;

	typedef vterm::MouseButton MouseButton;
	typedef vterm::Key Key;

	class Widget;

	class MouseButtonPayload {
	public:
		int x;
		int y;
		MouseButton button;
		Key modifiers;
	};

	class MouseWheelPayload {
	public:
		int x;
		int y;
		int by;
		Key modifiers;
	};

	class MouseMovePayload {
	public:
		int x;
		int y;
		Key modifiers;
	};

	typedef helpers::EventPayload<void, ui::Widget> NoPayloadEvent;

	typedef helpers::EventPayload<MouseButtonPayload, ui::Widget> MouseButtonEvent;
	typedef helpers::EventPayload<MouseWheelPayload, ui::Widget> MouseWheelEvent;
	typedef helpers::EventPayload<MouseMovePayload, ui::Widget> MouseMoveEvent;

	/** Base class for all UI widgets. 

	    The widget manages the basic properties of every ui element, namely the position, size, visibility, drawing of its contents and events corresponding to this functionality as well as basic input & output events from the terminal (mouse, keyboard and clipboard). 
	 */
	class Widget : virtual public helpers::Object {
	protected:

		// events

		/** Triggered when visibility changes to true. 
		 */
		Event<NoPayloadEvent> onShow;

		/** Triggered when visibility changes to false. 
		 */
		Event<NoPayloadEvent> onHide;

		/** Triggered when the widget's size has been updated. 
		 */
		Event<NoPayloadEvent> onResize;

		/** Triggered when the widget's position has been updated. 
		 */
		Event<NoPayloadEvent> onMove;

		/** Triggered when the widget has obtained focus, i.e. it will receive keyboard events. 
		 */
		Event<NoPayloadEvent> onFocusIn;

		/** Triggered when the widget has lost focus, i.e. it will no longer receive keyboard events. 
		 */
		Event<NoPayloadEvent> onFocusOut;

		Event<MouseButtonEvent> onMouseDown;
		Event<MouseButtonEvent> onMouseUp;
		Event<MouseWheelEvent> onMouseWheel;
		Event<MouseMoveEvent> onMouseMove;
		Event<NoPayloadEvent> onMouseEnter;
		Event<NoPayloadEvent> onMouseLeave;


	public:

		Widget(int x = 0, int y = 0, unsigned width = 1, unsigned height = 1):
			parent_(nullptr),
			visibleRegion_(nullptr),
			overlay_(false),
			visible_(true),
			x_(x),
			y_(y),
			width_(width),
			height_(height) {
		}

		Widget* parent() const {
			return parent_;
		}

		bool visible() const {
			return visible_;
		}

		/** Returns the x and y coordinates of the top-left corner of the widget in its parent 
		 */
		int x() const {
			return x_;
		}

		int y() const {
			return y_;
		}

		unsigned width() const {
			return width_;
		}

		unsigned height() const {
			return height_;
		}

		Rect rect() const {
			return Rect(x_, y_, x_ + width_, y_ + height_);
		}

		// all this should be protected and should then be enabled in respective subclasses, such as control

		/** Sets the widget as visible or hidden.

		    Also triggers the repaint of entire parent, because the widget may interfere with other children of its own parent.
		 */
		void setVisible(bool value) {
			if (visible_ != value) {
				if (value) {
					visible_ = true;
					if (parent_ != nullptr)
					    parent_->childInvalidated(this);
					trigger(onShow);
				} else {
					visible_ = false;
					if (parent_ != nullptr)
						parent_->childInvalidated(this);
					trigger(onHide);
				}
			}
		}

		/** Moves the widget to the given coordinates relative to its parent.
		 */
		void move(int x, int y) {
			if (x_ != x || y_ != y)
				updatePosition(x, y);
		}

		/** Resizes the widget. 
		 */
		void resize(unsigned width, unsigned height) {
			if (width_ != width || height_ != height)
				updateSize(width, height);
		}

		/** Repaints the widget. 

		    Only repaints the widget if the visibleRegion is valid. If the visible region is invalid, does nothing because when the region was invalidated, the repaint was automatically triggered, so there is either repaint pending, or in progress. 
		 */
		void repaint();

		void setOverlay(bool value) {
			if (value != overlay_)
				updateOverlay(value);
		}

	protected:

		// https://docs.microsoft.com/en-us/dotnet/framework/winforms/how-to-distinguish-between-clicks-and-double-clicks

		virtual void mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) {
			MARK_AS_UNUSED(col);
			MARK_AS_UNUSED(row);
			MARK_AS_UNUSED(button);
			MARK_AS_UNUSED(modifiers);
		}

		virtual void mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) {
			MARK_AS_UNUSED(col);
			MARK_AS_UNUSED(row);
			MARK_AS_UNUSED(button);
			MARK_AS_UNUSED(modifiers);
		}

		virtual void mouseWheel(unsigned col, unsigned row, int by, Key modifiers) {
			MARK_AS_UNUSED(col);
			MARK_AS_UNUSED(row);
			MARK_AS_UNUSED(by);
			MARK_AS_UNUSED(modifiers);
		}

		virtual void mouseMove(unsigned col, unsigned row, Key modifiers) {
			MARK_AS_UNUSED(col);
			MARK_AS_UNUSED(row);
			MARK_AS_UNUSED(modifiers);
		}

		virtual void mouseEnter() {

		}

		virtual void mouseLeave() {

		}


		/** Paints given child. 
		 */
		void paintChild(Widget * child, Canvas& canvas);

		/** Invalidates the widget and request its parent repaint,

		    If the widget is valid, invalidates its visible region and informs its parent that a child was invalidated. If the widget is already not valid, does nothing because the parent has already been notified. 

		 */
		void invalidate() {
			if (visibleRegion_.isValid()) {
				invalidateContents();
				if (parent_ != nullptr)
					parent_->childInvalidated(this);
			}
		}

		/** Invalidates the contents of the widget. 
		
		    For simple widgets this only means invalidating the widget's visible region, for more complex widgets all their children must be invalidated too. 
		 */
		virtual void invalidateContents() {
			visibleRegion_.invalidate();
		}

		/** Action to take when child is invalidated. 

		    This method is called whenever a child is invalidated. The default action is to repaint the widget.
		 */
		virtual void childInvalidated(Widget* child) {
			MARK_AS_UNUSED(child);
			repaint();
		}

		/** Updates the position of the widget. 
		
		    Already assumes that the new position is different from the current position. However, in case the requested position is invalid, the widget may adjust the size before setting it. After the size is updated, the control is invalidated and onMove event triggered. 
		 */
		virtual void updatePosition(int x, int y) {
			x_ = x;
			y_ = y;
			invalidate();
			trigger(onMove);
		}

		/** Updates the size of the widget. 
		
		    Assumes the size is different that current size. However, if the size is invalid, the widget may choose to update the requested width and height accordingly. Invalidates the widget and triggers the onResize event. 
		 */
		virtual void updateSize(unsigned width, unsigned height) {
			width_ = width;
			height_ = height;
			invalidate();
			trigger(onResize);
		}

		/** Paints the widget's contents on the provided canvas. 
		 */
		virtual void paint(Canvas& canvas) = 0;

		virtual void updateParent(Widget* parent) {
			if (parent == nullptr) {
				parent_ = nullptr;
				if (overlay_ != false)
				    updateOverlay(false);
			} else {
				ASSERT(parent_ == nullptr);
				parent_ = parent;
				// parent's repaint will eventually trigger the overlay update
			}
		}

		virtual void updateOverlay(bool value) {
			overlay_ = value;
		}

		virtual Widget* getMouseTarget(unsigned col, unsigned row) {
			ASSERT(visibleRegion_.contains(col, row));
			return this;
		}

		/** Updated trigger function for events which takes the Widget as base class for event sender.
		 */
		template<typename EVENT>
		void trigger(EVENT& e) {
			static_assert(std::is_same<typename EVENT::Payload::Sender, Widget >::value, "Only events with sender being ui::Widget should be used");
			typename EVENT::Payload p(this);
			e.trigger(p);
		}

		/** Updated trigger function for events which takes the Widget as base class for event sender.
		 */
		template<typename EVENT>
		void trigger(EVENT& e, typename EVENT::Payload::Payload const& payload) {
			static_assert(std::is_same<typename EVENT::Payload::Sender, Object >::value, "Only events with sender being ui::Widget should be used");
			typename EVENT::Payload p(this, payload);
			e.trigger(p);
		}

	private:

		friend class Layout;
		friend class Container;
		friend class RootWindow;

		/* Parent widget or none. 
		 */
		Widget* parent_;

		/* Visible region of the canvas. 
		 */
		Canvas::VisibleRegion visibleRegion_;

		/* If true, the rectangle of the widget is shared with other widgets, i.e. when the widget is to be repainted, its parent must be repainted instead. */
		bool overlay_;

		/* Visibility */
		bool visible_;

		/* Position */
		int x_;
		int y_;

		/* Size */
		unsigned width_;
		unsigned height_;

	};





} // namespace ui


