#pragma once

#include <QEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QWidget>

namespace qttooltipkit
{
enum class Placement { Above, Below, Left, Right };
enum class Alignment { Start, Center, End };

struct PopupOptions
{
    Placement placement = Placement::Below;
    Alignment alignment = Alignment::Center;
    int hoverDelayMs = 350;
    int leaveDelayMs = 250;
    int fadeMs = 120;
    int gap = 4;
    bool arrow = true;
};

// Owns content. Keep the anchor alive for the lifetime of the popup.
class Popup final : public QObject
{
public:
    Popup(QWidget& anchor, QWidget* content, PopupOptions options = {})
        : QObject(&anchor), _anchor(anchor), _options(options),
          _embedded(QGuiApplication::platformName().startsWith("wayland")),
          _panel(new QFrame(anchor.window(), _embedded ? Qt::Widget : Qt::ToolTip | Qt::WindowDoesNotAcceptFocus))
    {
        _panel->setObjectName(QStringLiteral("qtTooltipKitPopup"));
        _panel->setFrameShape(QFrame::StyledPanel);
        _panel->setAutoFillBackground(true);
        _panel->setAttribute(Qt::WA_ShowWithoutActivating);
        auto* layout = new QBoxLayout(QBoxLayout::TopToBottom, _panel);
        _arrow = new QLabel(_panel);
        layout->addWidget(_arrow, 0, Qt::AlignCenter);
        layout->addWidget(content);
        _enterTimer.setSingleShot(true);
        _leaveTimer.setSingleShot(true);
        connect(&_enterTimer, &QTimer::timeout, this, &Popup::show);
        connect(&_leaveTimer, &QTimer::timeout, this, &Popup::hide);
        _anchor.installEventFilter(this);
        _panel->installEventFilter(this);
        _panel->hide();
    }

    ~Popup() override { delete _panel.data(); }

    void show()
    {
        if (!_panel || !_anchor.isVisible())
            return;
        _leaveTimer.stop();
        const QRect anchorRect(_anchor.mapToGlobal(QPoint()), _anchor.size());
        auto* screen = QGuiApplication::screenAt(anchorRect.center());
        if (!screen)
            screen = _anchor.screen();
        const QRect bounds = _embedded
            ? QRect(_anchor.window()->mapToGlobal(QPoint()), _anchor.window()->size())
            : screen->availableGeometry();
        auto side = _options.placement;
        const auto opposite = [](Placement p) {
            switch (p) {
            case Placement::Above: return Placement::Below;
            case Placement::Below: return Placement::Above;
            case Placement::Left: return Placement::Right;
            default: return Placement::Left;
            }
        };
        const auto place = [&](Placement p) {
            auto* layout = static_cast<QBoxLayout*>(_panel->layout());
            const bool vertical = p == Placement::Above || p == Placement::Below;
            layout->setDirection(p == Placement::Above ? QBoxLayout::BottomToTop :
                p == Placement::Left ? QBoxLayout::RightToLeft :
                vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
            _arrow->setText(QString(QChar(p == Placement::Above ? 0x25BC :
                p == Placement::Below ? 0x25B2 : p == Placement::Left ? 0x25B6 : 0x25C0)));
            _arrow->setVisible(_options.arrow);
            _panel->adjustSize();
            const QSize size = _panel->size();
            const auto align = [&](int start, int span, int extent) {
                return start + (_options.alignment == Alignment::Start ? 0 :
                    _options.alignment == Alignment::End ? span - extent : (span - extent) / 2);
            };
            const int gap = qMax(0, _options.gap);
            return vertical
                ? QPoint(align(anchorRect.left(), anchorRect.width(), size.width()),
                    p == Placement::Above ? anchorRect.top() - size.height() - gap : anchorRect.bottom() + 1 + gap)
                : QPoint(p == Placement::Left ? anchorRect.left() - size.width() - gap : anchorRect.right() + 1 + gap,
                    align(anchorRect.top(), anchorRect.height(), size.height()));
        };
        QPoint position = place(side);
        if (!bounds.contains(QRect(position, _panel->size()))) {
            const QPoint alternative = place(opposite(side));
            position = bounds.contains(QRect(alternative, _panel->size())) ? alternative : place(side);
        }
        position.setX(qBound(bounds.left(), position.x(), qMax(bounds.left(), bounds.right() + 1 - _panel->width())));
        position.setY(qBound(bounds.top(), position.y(), qMax(bounds.top(), bounds.bottom() + 1 - _panel->height())));
        _panel->move(_embedded ? _anchor.window()->mapFromGlobal(position) : position);
        if (_panel->isVisible())
            return;
        _panel->show();
        _panel->raise();
        if (_options.fadeMs > 0) {
            auto* effect = new QGraphicsOpacityEffect(_panel);
            effect->setOpacity(0);
            _panel->setGraphicsEffect(effect);
            auto* animation = new QPropertyAnimation(effect, "opacity", effect);
            animation->setDuration(_options.fadeMs);
            animation->setStartValue(0.0);
            animation->setEndValue(1.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }

    void hide()
    {
        _enterTimer.stop();
        _leaveTimer.stop();
        if (_panel)
            _panel->hide();
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override
    {
        if (event->type() == QEvent::Enter) {
            _leaveTimer.stop();
            if (object == &_anchor)
                _enterTimer.start(qMax(0, _options.hoverDelayMs));
        } else if (event->type() == QEvent::Leave) {
            _enterTimer.stop();
            _leaveTimer.start(qMax(0, _options.leaveDelayMs));
        } else if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            hide();
        } else if (object == &_anchor && event->type() == QEvent::Hide) {
            hide();
        }
        return QObject::eventFilter(object, event);
    }

private:
    QWidget& _anchor;
    PopupOptions _options;
    bool _embedded;
    QPointer<QFrame> _panel;
    QLabel* _arrow = nullptr;
    QTimer _enterTimer;
    QTimer _leaveTimer;
};
}
