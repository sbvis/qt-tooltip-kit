#include <QtTooltipKitPopup.h>
#include <QtTooltipKit.h>

int main()
{
    using namespace qttooltipkit;
    Options options;
    options.accent = QColor("#357ec7");
    options.sections = {{ "Inputs", { "Selected files" } }};
    options.tables = {{ "Summary", { "Group", "Files" }, {{ "A", "240" }, { "B", "180" }} }};
    options.charts = {{ "Counts", { "A", "Missing" }, { -240, std::numeric_limits<double>::quiet_NaN() } }};
    options.shortcut = "Ctrl+Enter";
    options.note = "First update";
    const auto html = markup("Update <all>", "Read & rank", options);
    return !(html.contains("Update &lt;all&gt;") && html.contains("Read &amp; rank") &&
             html.contains("#357ec7") && html.contains("<th>Files</th>") &&
             html.contains("-240") && !html.contains("Missing") &&
             html.contains("Ctrl+Enter") && html.contains("First update"));
}
