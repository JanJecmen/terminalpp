#pragma once

#include "stamp.h"

#include "ui/widgets/panel.h"
#include "ui/traits/modal.h"

namespace tpp {

    class AboutBox : public ui::CustomPanel, public Modal<AboutBox> {
    public:
        AboutBox():
            CustomPanel{},
            Modal{true} {
            setBorder(Border{Color::White}.setAll(Border::Kind::Thin));
            setBackground(Color::Blue);
            setWidthHint(SizeHint::Manual());
            setHeightHint(SizeHint::Manual());
            resize(65,10);
            setFocusable(true);  
        }

    protected:

        void paint(Canvas & canvas) override {
            CustomPanel::paint(canvas);
            canvas.setFg(Color::White);
            canvas.setFont(ui::Font{}.setSize(2));
            canvas.textOut(Point{20,1}, "Terminal++");
            canvas.setFont(ui::Font{});
            if (stamp::version.empty()) {
                canvas.textOut(Point{3,3}, STR("commit:   " << stamp::commit << (stamp::dirty ? "*" : "")));
                canvas.textOut(Point{13,4}, stamp::build_time);
            } else {
                canvas.textOut(Point{3,3}, STR("version:  " << stamp::version));
                canvas.textOut(Point{13,4}, STR(stamp::commit << (stamp::dirty ? "*" : "")));
                canvas.textOut(Point{13,5}, stamp::build_time);
            }
#if (defined RENDERER_QT)
            canvas.textOut(Point{3, 7}, STR("platform: " << ARCH << "(Qt) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build));
#else
            canvas.textOut(Point{3, 7}, STR("platform: " << ARCH << "(native) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build));
#endif
            canvas.setFont(canvas.font().setBlink(true));
            canvas.textOut(Point{20, 9}, "Hit a key to dismiss");
            canvas.setFont(ui::Font{});
        }

        void mouseClick(UIEvent<MouseButtonEvent>::Payload & event) override {
            MARK_AS_UNUSED(event);
            dismiss(this);
        }

        void keyDown(UIEvent<Key>::Payload & event) override {
            MARK_AS_UNUSED(event);
            dismiss(this);
        }

    }; // tpp::AboutBox

} // namespace tpp
