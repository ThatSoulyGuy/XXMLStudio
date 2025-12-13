#ifndef ICONUTILS_H
#define ICONUTILS_H

#include <QIcon>
#include <QString>
#include <QColor>

namespace XXMLStudio {

/**
 * Utility class for loading and adjusting icons based on background brightness.
 * Automatically inverts/adjusts icon colors to ensure proper contrast.
 */
class IconUtils
{
public:
    /**
     * Load an icon and adjust it for a dark background (toolbar/menu).
     * Dark icon colors will be inverted to light colors.
     */
    static QIcon loadForDarkBackground(const QString& resourcePath);

    /**
     * Load an icon for a specific background color.
     * Calculates brightness difference and adjusts icon colors to ensure contrast.
     */
    static QIcon loadForBackground(const QString& resourcePath, const QColor& backgroundColor);

    /**
     * Check if a color is considered "dark" (brightness < 128).
     */
    static bool isDarkColor(const QColor& color);

private:
    /**
     * Process SVG content to adjust colors based on background brightness.
     * Colors with insufficient contrast are inverted.
     */
    static QByteArray processSvgForBackground(const QByteArray& svgContent, int backgroundBrightness);

    /**
     * Process SVG content for dark theme (convenience wrapper).
     */
    static QByteArray processSvgForDarkTheme(const QByteArray& svgContent);

    /**
     * Lighten a hex color string by a fixed amount.
     */
    static QString lightenColor(const QString& hexColor, int amount = 180);
};

} // namespace XXMLStudio

#endif // ICONUTILS_H
