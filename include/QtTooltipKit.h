#pragma once

#include <QColor>
#include <cmath>
#include <type_traits>
#include <QList>
#include <QStringList>
#include <QWidget>
#include <algorithm>
#include <limits>

namespace qttooltipkit
{
struct Section
{
    QString heading;
    QStringList paragraphs;
};

struct Table
{
    QString heading;
    QStringList columns;
    QList<QStringList> rows;
};

struct BarChart
{
    QString heading;
    QStringList labels;
    QList<double> values;
    QColor color = QColor("#357ec7");
};

struct Options
{
    int widthPixels = 320;
    int fontPoints = 10;
    int durationMs = std::numeric_limits<int>::max();
    QList<Section> sections;
    QString shortcut;
    QString note;
    QList<Table> tables;
    QList<BarChart> charts;
    QColor accent;
    int maxRows = 12;
    QColor background;
    QColor foreground;
    QColor tableHeaderBackground;
    QColor alternateRowBackground;
    int paddingPixels = 10;
    int tableCellPadding = 5;
    bool tableBorders = false;
};

inline QString markup(const QString& heading, const QString& description, const Options& options)
{
    const auto escape = [](QString text) {
        return text.toHtmlEscaped().replace('\n', QStringLiteral("<br>"));
    };
    const auto background = [](const QColor& color) {
        return color.isValid() ? QStringLiteral(" bgcolor='%1'").arg(color.name()) : QString{};
    };
    QString content;
    if (!heading.isEmpty())
        content += options.accent.isValid()
            ? QStringLiteral("<b style='color:%1'>%2</b>").arg(options.accent.name(), escape(heading))
            : QStringLiteral("<b>%1</b>").arg(escape(heading));
    if (!description.isEmpty())
        content += QStringLiteral("<p>%1</p>").arg(escape(description));
    for (const auto& section : options.sections) {
        if (!section.heading.isEmpty())
            content += QStringLiteral("<b>%1</b>").arg(escape(section.heading));
        for (const auto& paragraph : section.paragraphs)
            content += QStringLiteral("<p>%1</p>").arg(escape(paragraph));
    }
    const int maxRows = std::clamp(options.maxRows, 1, 50);
    for (const auto& table : options.tables) {
        if (!table.heading.isEmpty())
            content += QStringLiteral("<p><b>%1</b></p>").arg(escape(table.heading));
        content += QStringLiteral("<table border='%1' cellspacing='0' cellpadding='%2'><tr%3>")
            .arg(options.tableBorders ? 1 : 0).arg(std::clamp(options.tableCellPadding, 0, 24))
            .arg(background(options.tableHeaderBackground));
        for (const auto& column : table.columns)
            content += QStringLiteral("<th>%1</th>").arg(escape(column));
        content += QStringLiteral("</tr>");
        for (qsizetype row = 0; row < std::min<qsizetype>(maxRows, table.rows.size()); ++row) {
            content += QStringLiteral("<tr%1>").arg(row % 2 ? background(options.alternateRowBackground) : QString{});
            for (qsizetype column = 0; column < table.columns.size(); ++column)
                content += QStringLiteral("<td>%1</td>").arg(escape(table.rows[row].value(column)));
            content += QStringLiteral("</tr>");
        }
        content += QStringLiteral("</table>");
    }
    for (const auto& chart : options.charts) {
        if (!chart.heading.isEmpty())
            content += QStringLiteral("<p><b>%1</b></p>").arg(escape(chart.heading));
        const auto count = std::min({qsizetype(maxRows), chart.labels.size(), chart.values.size()});
        double peak = 0;
        for (qsizetype i = 0; i < count; ++i)
            if (std::isfinite(chart.values[i]))
                peak = std::max(peak, std::abs(chart.values[i]));
        content += QStringLiteral("<table cellpadding='3'>");
        for (qsizetype i = 0; i < count; ++i) {
            const auto value = chart.values[i];
            if (!std::isfinite(value))
                continue;
            const int blocks = peak > 0 ? int(std::ceil(10 * (std::abs(value) / peak))) : 0;
            content += QStringLiteral("<tr><td>%1</td><td align='right'>%2</td><td style='color:%3'>%4</td></tr>")
                .arg(escape(chart.labels[i]), QString::number(value, 'g', 5),
                     chart.color.name(), QString(blocks, QChar(0x2588)));
        }
        content += QStringLiteral("</table>");
    }
    if (!options.shortcut.isEmpty())
        content += QStringLiteral("<p><kbd>%1</kbd></p>").arg(escape(options.shortcut));
    if (!options.note.isEmpty())
        content += QStringLiteral("<p><i>%1</i></p>").arg(escape(options.note));
    const QString foreground = options.foreground.isValid()
        ? QStringLiteral("; color: %1").arg(options.foreground.name()) : QString{};
    return QStringLiteral("<table width='%1' cellpadding='%2' cellspacing='0'%3><tr>"
                          "<td style='font-size: %4pt%5'>%6</td></tr></table>")
        .arg(std::clamp(options.widthPixels, 120, 900))
        .arg(std::clamp(options.paddingPixels, 0, 32))
        .arg(background(options.background))
        .arg(std::clamp(options.fontPoints, 8, 28))
        .arg(foreground, content);
}

inline void set(QWidget& widget, const QString& heading, const QString& description, const Options& options = {})
{
    widget.setToolTip(markup(heading, description, options));
    widget.setToolTipDuration(options.durationMs);
}

template<class Action, std::enable_if_t<!std::is_base_of_v<QWidget, Action>, int> = 0>
inline void set(Action& action, const QString& heading, const QString& description, const Options& options = {})
{
    action.setToolTip(markup(heading, description, options));
    const auto configure = action.getWidgetConfigurationFunction();
    action.setWidgetConfigurationFunction([configure, duration = options.durationMs](auto* owner, QWidget* widget) {
        if (configure)
            configure(owner, widget);
        widget->setToolTipDuration(duration);
        for (auto* child : widget->findChildren<QWidget*>())
            child->setToolTipDuration(duration);
    });
}
}
