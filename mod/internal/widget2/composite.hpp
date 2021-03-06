/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen,
 *              Meng Tan, Raphael Zhou
 */

#if !defined(REDEMPTION_MOD_WIDGET2_WIDGET_COMPOSITE_HPP_)
#define REDEMPTION_MOD_WIDGET2_WIDGET_COMPOSITE_HPP_

#include "widget.hpp"
#include "keymap2.hpp"
#include "region.hpp"
#include "draw_api.hpp"
#include "colors.hpp"
#include "RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"

inline void fill_region(DrawApi & drawable, const Region & region, int bg_color) {
    for (const Rect & rect : region.rects) {
        drawable.draw(RDPOpaqueRect(rect, bg_color), rect);
    }
}

class CompositeContainer {
public:
    virtual ~CompositeContainer() {}

    enum { invalid_iterator = 0 };

    typedef void * iterator;

    virtual iterator add(Widget2 * w) = 0;
    virtual void remove(const Widget2 * w) = 0;

    virtual Widget2 * get(iterator iter) const = 0;

    virtual iterator get_first() = 0;
    virtual iterator get_last() = 0;

    virtual iterator get_previous(iterator iter, bool loop = false) = 0;
    virtual iterator get_next(iterator iter, bool loop = false) = 0;

    virtual iterator find(const Widget2 * w) = 0;

    virtual void clear() = 0;
};

class CompositeArray : public CompositeContainer {
    enum {
        MAX_CHILDREN_COUNT = 256
    };

    Widget2 * child_table[MAX_CHILDREN_COUNT];
    size_t    children_count;

public:
    CompositeArray() : child_table(), children_count(0) {
        for (int i = 0; i < MAX_CHILDREN_COUNT; i++) {
            REDASSERT(!child_table[i]);
        }
    }

    virtual iterator add(Widget2 * w) {
        REDASSERT(w);
        REDASSERT(this->children_count < MAX_CHILDREN_COUNT);
        this->child_table[this->children_count] = w;
        return static_cast<iterator>(&this->child_table[this->children_count++]);
    }
    virtual void remove(const Widget2 * w) {
        REDASSERT(w);
        REDASSERT(this->children_count);
        bool children_found = false;
        for (size_t i = 0; i < this->children_count; i++) {
            if (!children_found) {
                if (this->child_table[i] == w) {
                    children_found = true;
                    this->child_table[i] = NULL;
                }
            }
            else {
                this->child_table[i - 1] = this->child_table[i];
            }
        }
        REDASSERT(children_found);
        if (children_found) {
            this->child_table[this->children_count] = NULL;
            this->children_count--;
        }
    }

    virtual Widget2 * get(iterator iter) const {
        return *(static_cast<Widget2 **>(iter));
    }

    virtual iterator get_first() {
        if (!this->children_count) {
            return reinterpret_cast<iterator>(invalid_iterator);
        }

        return static_cast<iterator>(&this->child_table[0]);
    }
    virtual iterator get_last() {
        if (!this->children_count) {
            return reinterpret_cast<iterator>(invalid_iterator);
        }

        return static_cast<iterator>(&this->child_table[this->children_count - 1]);
    }

    virtual iterator get_previous(iterator iter, bool loop = false) {
        if (iter == this->get_first()) {
            if (loop) {
                iterator last;
                if ((last = this->get_last()) != iter) {
                    return last;
                }
            }
            return reinterpret_cast<iterator>(invalid_iterator);
        }

        return (static_cast<Widget2 **>(iter)) - 1;
    }
    virtual iterator get_next(iterator iter, bool loop = false) {
        if (iter == this->get_last()) {
            if (loop) {
                iterator frist;
                if ((frist = this->get_first()) != iter) {
                    return frist;
                }
            }
            return reinterpret_cast<iterator>(invalid_iterator);
        }

        return (static_cast<Widget2 **>(iter)) + 1;
    }

    virtual iterator find(const Widget2 * w) {
        for (size_t i = 0; i < this->children_count; i++) {
            if (this->child_table[i] == w) {
                return static_cast<iterator>(&this->child_table[i]);
            }
        }

        return reinterpret_cast<iterator>(invalid_iterator);
    }

    virtual void clear()  {
        this->children_count = 0;
    }
};  // class CompositeArray

class WidgetParent : public Widget2 {
    Widget2 * pressed;

    int bg_color;

protected:
    CompositeContainer * impl;

public:
    Widget2 * current_focus;

    WidgetParent(DrawApi & drawable, const Rect & rect, Widget2 & parent,
                 NotifyApi * notifier, int group_id = 0)
        : Widget2(drawable, rect, parent, notifier, group_id)
        , pressed(NULL)
        , bg_color(BLACK)
        , impl(NULL)
        , current_focus(NULL) {}

    virtual ~WidgetParent() {}

    void set_widget_focus(Widget2 * new_focused, int reason) {
        REDASSERT(new_focused);
        if (new_focused != this->current_focus) {
            if (this->current_focus) {
                this->current_focus->blur();
            }
            this->current_focus = new_focused;
        }

        this->current_focus->focus(reason);
    }

    virtual void focus(int reason) {
        const bool tmp_has_focus = this->has_focus;
        if (!this->has_focus) {
            this->has_focus = true;
            this->send_notify(NOTIFY_FOCUS_BEGIN);

            if (reason == focus_reason_tabkey) {
                this->current_focus = this->get_next_focus(NULL, false);
            }
            else if (reason == focus_reason_backtabkey) {
                this->current_focus = this->get_previous_focus(NULL, false);
            }
        }
        if (this->current_focus) {
            this->current_focus->focus(reason);
        }
        if (!tmp_has_focus) {
            this->refresh(this->rect);
        }
    }
    virtual void blur() {
        if (this->has_focus) {
            this->has_focus = false;
            this->send_notify(NOTIFY_FOCUS_END);
        }
        if (this->current_focus) {
            this->current_focus->blur();
        }
        this->refresh(this->rect);
    }

    Widget2 * get_next_focus(Widget2 * w, bool loop) {
        CompositeContainer::iterator iter;
        if (!w) {
            REDASSERT(!loop);
            if ((iter = this->impl->get_first()) == reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
                return NULL;
            }

            w = this->impl->get(iter);
            if ((w->tab_flag != Widget2::IGNORE_TAB) && (w->focus_flag != Widget2::IGNORE_FOCUS)) {
                return w;
            }
        }
        else {
            iter = this->impl->find(w);
            REDASSERT(iter != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator));
        }

        CompositeContainer::iterator future_focus_iter;
        while ((future_focus_iter = this->impl->get_next(iter, loop)) != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            Widget2 * future_focus_w = this->impl->get(future_focus_iter);
            if ((future_focus_w->tab_flag != Widget2::IGNORE_TAB) && (future_focus_w->focus_flag != Widget2::IGNORE_FOCUS)) {
                return future_focus_w;
            }

            if (future_focus_w == w) {
                break;
            }

            iter = future_focus_iter;
        }

        return NULL;
    }
    Widget2 * get_previous_focus(Widget2 * w, bool loop) {
        CompositeContainer::iterator iter;
        if (!w) {
            REDASSERT(!loop);
            if ((iter = this->impl->get_last()) == reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
                return NULL;
            }

            w = this->impl->get(iter);
            if ((w->tab_flag != Widget2::IGNORE_TAB) && (w->focus_flag != Widget2::IGNORE_FOCUS)) {
                return w;
            }
        }
        else {
            iter = this->impl->find(w);
            REDASSERT(iter != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator));
        }

        CompositeContainer::iterator future_focus_iter;
        while ((future_focus_iter = this->impl->get_previous(iter, loop)) != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            Widget2 * future_focus_w = this->impl->get(future_focus_iter);
            if ((future_focus_w->tab_flag != Widget2::IGNORE_TAB) && (future_focus_w->focus_flag != Widget2::IGNORE_FOCUS)) {
                return future_focus_w;
            }

            if (future_focus_w == w) {
                break;
            }

            iter = future_focus_iter;
        }

        return NULL;
    }

    virtual void add_widget(Widget2 * w) {
        this->impl->add(w);

        if (!this->current_focus &&
            (w->tab_flag != Widget2::IGNORE_TAB) && (w->focus_flag != Widget2::IGNORE_FOCUS)) {
            this->current_focus = w;
        }
    }
    virtual void remove_widget(Widget2 * w) {
        if (this->current_focus == w) {
            Widget2 * future_focus_w;
            if ((future_focus_w = this->get_next_focus(w, false)) != NULL) {
                this->current_focus = future_focus_w;
            }
            else if ((future_focus_w = this->get_previous_focus(w, false)) != NULL) {
                this->current_focus = future_focus_w;
            }
            else {
                this->current_focus = NULL;
            }
        }

        this->impl->remove(w);
    }
    virtual void clear() {
        this->impl->clear();

        this->current_focus = NULL;
    }

    virtual void draw(const Rect & clip) {
        Rect rect_intersect = clip.intersect(this->rect);
        this->draw_inner_free(rect_intersect, this->get_bg_color());
        this->draw_children(rect_intersect);
    }
    virtual void draw_children(const Rect & clip) {
        CompositeContainer::iterator iter_w_current = this->impl->get_first();
        while (iter_w_current != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            Widget2 * w = this->impl->get(iter_w_current);
            REDASSERT(w);

            w->refresh(clip.intersect(w->rect));

            iter_w_current = this->impl->get_next(iter_w_current);
        }
    }
    virtual void draw_inner_free(const Rect & clip, int bg_color) {
        Region region;
        region.rects.push_back(clip.intersect(this->rect));

        CompositeContainer::iterator iter_w_current = this->impl->get_first();
        while (iter_w_current != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            Widget2 * w = this->impl->get(iter_w_current);
            REDASSERT(w);

            Rect rect_widget = clip.intersect(w->rect);
            if (!rect_widget.isempty()) {
                region.subtract_rect(rect_widget);
            }

            iter_w_current = this->impl->get_next(iter_w_current);
        }

        ::fill_region(this->drawable, region, bg_color);
    }
    virtual void hide_child(const Rect & clip, int bg_color) {
        Region region;

        CompositeContainer::iterator iter_w_current = this->impl->get_first();
        while (iter_w_current != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            Widget2 * w = this->impl->get(iter_w_current);
            REDASSERT(w);

            Rect rect_widget = clip.intersect(w->rect);
            if (!rect_widget.isempty()) {
                region.add_rect(rect_widget);
            }

            iter_w_current = this->impl->get_next(iter_w_current);
        }

        if (!region.rects.empty()) {
            ::fill_region(this->drawable, region, bg_color);
        }
    }

    virtual int get_bg_color() const {
        return this->bg_color;
    }

    virtual void set_bg_color(int color) {
        this->bg_color = color;
    }

    void move_xy(int16_t x, int16_t y) {
        CompositeContainer::iterator iter_w_first = this->impl->get_first();
        if (iter_w_first != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            CompositeContainer::iterator iter_w_current = iter_w_first;
            do {
                Widget2 * w = this->impl->get(iter_w_current);
                REDASSERT(w);
                w->set_xy(x + w->dx(), y + w->dy());

                iter_w_current = this->impl->get_next(iter_w_current);
            }
            while ((iter_w_current != iter_w_first) &&
                   (iter_w_current != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)));
        }
    }

    virtual bool next_focus() {
        if (this->current_focus) {
            if (this->current_focus->next_focus()) {
                return true;
            }

            Widget2 * future_focus_w = this->get_next_focus(this->current_focus, false);

            if (future_focus_w) {
                this->set_widget_focus(future_focus_w, focus_reason_tabkey);

                return true;
            }

            this->current_focus->blur();
            this->current_focus = this->get_next_focus(NULL, false);
            REDASSERT(this->current_focus);
        }

        return false;
    }
    virtual bool previous_focus() {
        if (this->current_focus) {
            if (this->current_focus->previous_focus()) {
                return true;
            }

            Widget2 * future_focus_w = this->get_previous_focus(this->current_focus, false);

            if (future_focus_w) {
                this->set_widget_focus(future_focus_w, focus_reason_backtabkey);

                return true;
            }

            this->current_focus->blur();
            this->current_focus = this->get_previous_focus(NULL, false);
            REDASSERT(this->current_focus);
        }

        return false;
    }

    virtual Widget2 * widget_at_pos(int16_t x, int16_t y) {
        if (!this->rect.contains_pt(x, y)) {
            return NULL;
        }
        if (this->current_focus) {
            if (this->current_focus->rect.contains_pt(x, y)) {
                return this->current_focus;
            }
        }
        CompositeContainer::iterator iter_w_current = this->impl->get_first();
        while (iter_w_current != reinterpret_cast<CompositeContainer::iterator>(CompositeContainer::invalid_iterator)) {
            Widget2 * w = this->impl->get(iter_w_current);
            REDASSERT(w);
            if (w->rect.contains_pt(x, y)) {
                return w;
            }

            iter_w_current = this->impl->get_next(iter_w_current);
        }

        return NULL;
    }

    virtual void rdp_input_scancode( long param1, long param2, long param3
                                   , long param4, Keymap2 * keymap) {
        while (keymap->nb_kevent_available() > 0) {
            uint32_t nb_kevent = keymap->nb_kevent_available();
            switch (keymap->top_kevent()) {
            case Keymap2::KEVENT_TAB:
                //std::cout << ("tab") << '\n';
                keymap->get_kevent();
                this->next_focus();
                break;
            case Keymap2::KEVENT_BACKTAB:
                //std::cout << ("backtab") << '\n';
                keymap->get_kevent();
                this->previous_focus();
                break;
            default:
                if (this->current_focus) {
                    this->current_focus->rdp_input_scancode(param1, param2, param3, param4, keymap);
                }
                break;
            }
            if (nb_kevent == keymap->nb_kevent_available()) {
                // this is to prevent infinite loop if the kevent is not consummed
                keymap->get_kevent();
            }
        }
    }

    virtual void rdp_input_mouse(int device_flags, int x, int y, Keymap2 * keymap) {
        Widget2 * w = this->widget_at_pos(x, y);

        // Mouse clic release
        // w could be null if mouse is located at an empty space
        if (device_flags == MOUSE_FLAG_BUTTON1) {
            if (this->pressed
                && (w != this->pressed)) {
                this->pressed->rdp_input_mouse(device_flags, x, y, keymap);
            }
            this->pressed = NULL;
        }
        if (w) {
            // get focus when mouse clic
            if (device_flags == (MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN)) {
                this->pressed = w;
                if (/*(*/w->focus_flag != IGNORE_FOCUS/*) && (w != this->current_focus)*/) {
                    this->set_widget_focus(w, focus_reason_mousebutton1);
                }
            }
            w->rdp_input_mouse(device_flags, x, y, keymap);
        }
        else {
            Widget2::rdp_input_mouse(device_flags, x, y, keymap);
        }
        if (device_flags == MOUSE_FLAG_MOVE && this->pressed) {
            this->pressed->rdp_input_mouse(device_flags, x, y, keymap);
        }
    }
};

// WidgetComposite is a WidgetParent and use Delegation to an implementation of CompositeInterface
class WidgetComposite: public WidgetParent {
    CompositeArray composite_array;

public:
    WidgetComposite(DrawApi & drawable, const Rect & rect, Widget2 & parent,
                    NotifyApi * notifier, int group_id = 0)
    : WidgetParent(drawable, rect, parent, notifier, group_id) {
        this->impl = & composite_array;
    }

    virtual ~WidgetComposite() {}

    virtual void draw(const Rect & clip) {
        Rect rect_intersect = clip.intersect(this->rect);
        this->draw_children(rect_intersect);
    }
};

#endif
