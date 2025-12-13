#include "IconUtils.h"

#include <QFile>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <cmath>

namespace XXMLStudio {

// Calculate perceived brightness (0-255) using standard formula
static int calculateBrightness(const QColor& color)
{
    return (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
}

// Parse a hex color string to QColor
static QColor parseHexColor(const QString& hex)
{
    QString h = hex.trimmed();
    if (h.startsWith('#')) {
        h = h.mid(1);
    }

    if (h.length() == 3) {
        // Expand #RGB to #RRGGBB
        h = QString("%1%1%2%2%3%3").arg(h[0]).arg(h[1]).arg(h[2]);
    }

    if (h.length() != 6) {
        return QColor();
    }

    bool ok;
    int r = h.mid(0, 2).toInt(&ok, 16);
    int g = h.mid(2, 2).toInt(&ok, 16);
    int b = h.mid(4, 2).toInt(&ok, 16);

    return QColor(r, g, b);
}

// Adjust a color's brightness to achieve target brightness
static QColor adjustBrightnessTo(const QColor& color, int targetBrightness)
{
    int currentBrightness = calculateBrightness(color);

    if (currentBrightness == targetBrightness) {
        return color;
    }

    // Calculate how much we need to shift
    int diff = targetBrightness - currentBrightness;

    int r = qBound(0, color.red() + diff, 255);
    int g = qBound(0, color.green() + diff, 255);
    int b = qBound(0, color.blue() + diff, 255);

    return QColor(r, g, b);
}

// Invert brightness while preserving hue/saturation
static QColor invertBrightness(const QColor& color, int backgroundBrightness)
{
    int colorBrightness = calculateBrightness(color);

    // Calculate what the brightness should be on the other side of the background
    // If background is dark (e.g., 45), and icon is dark (e.g., 33),
    // we want icon to be light (e.g., 255 - 33 = 222)
    int targetBrightness;

    if (backgroundBrightness < 128) {
        // Dark background - icon colors should be light
        // Map dark colors to light: 0->255, 128->128, 255->0 (but we mainly care about dark colors)
        targetBrightness = 255 - colorBrightness;

        // Ensure minimum contrast
        if (targetBrightness < backgroundBrightness + 80) {
            targetBrightness = qMin(255, backgroundBrightness + 120);
        }
    } else {
        // Light background - icon colors should be dark
        targetBrightness = 255 - colorBrightness;

        if (targetBrightness > backgroundBrightness - 80) {
            targetBrightness = qMax(0, backgroundBrightness - 120);
        }
    }

    return adjustBrightnessTo(color, targetBrightness);
}

QIcon IconUtils::loadForDarkBackground(const QString& resourcePath)
{
    // Toolbar background is #2D2D30 = rgb(45, 45, 48)
    return loadForBackground(resourcePath, QColor(45, 45, 48));
}

QIcon IconUtils::loadForBackground(const QString& resourcePath, const QColor& backgroundColor)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QIcon(resourcePath);
    }

    QByteArray svgContent = file.readAll();
    file.close();

    int bgBrightness = calculateBrightness(backgroundColor);

    // Process SVG to adjust colors based on background brightness
    QByteArray processedSvg = processSvgForBackground(svgContent, bgBrightness);

    QSvgRenderer renderer(processedSvg);
    if (!renderer.isValid()) {
        return QIcon(resourcePath);
    }

    QIcon icon;
    for (int size : {16, 20, 24, 32, 48}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
        painter.end();
        icon.addPixmap(pixmap);
    }

    return icon;
}

bool IconUtils::isDarkColor(const QColor& color)
{
    return calculateBrightness(color) < 128;
}

QByteArray IconUtils::processSvgForBackground(const QByteArray& svgContent, int backgroundBrightness)
{
    QString svg = QString::fromUtf8(svgContent);

    // Find all hex colors in the SVG and adjust them
    QRegularExpression hexColorRe("#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})\\b");

    QRegularExpressionMatchIterator it = hexColorRe.globalMatch(svg);
    QVector<QPair<QString, QString>> replacements;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString originalColor = match.captured(0);
        QColor color = parseHexColor(originalColor);

        if (!color.isValid()) {
            continue;
        }

        int colorBrightness = calculateBrightness(color);

        // Calculate contrast with background
        int contrast = std::abs(colorBrightness - backgroundBrightness);

        // If contrast is too low, adjust the color
        if (contrast < 100) {
            QColor adjusted = invertBrightness(color, backgroundBrightness);
            QString newColor = QString("#%1%2%3")
                .arg(adjusted.red(), 2, 16, QChar('0'))
                .arg(adjusted.green(), 2, 16, QChar('0'))
                .arg(adjusted.blue(), 2, 16, QChar('0'));

            replacements.append({originalColor, newColor});
        }
    }

    // Apply replacements (case-insensitive)
    for (const auto& pair : replacements) {
        QRegularExpression re(QRegularExpression::escape(pair.first),
                             QRegularExpression::CaseInsensitiveOption);
        svg.replace(re, pair.second);
    }

    // Also handle rgb() format
    QRegularExpression rgbRe(R"(rgb\s*\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\))");
    it = rgbRe.globalMatch(svg);

    QVector<QPair<QString, QString>> rgbReplacements;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int r = match.captured(1).toInt();
        int g = match.captured(2).toInt();
        int b = match.captured(3).toInt();

        QColor color(r, g, b);
        int colorBrightness = calculateBrightness(color);
        int contrast = std::abs(colorBrightness - backgroundBrightness);

        if (contrast < 100) {
            QColor adjusted = invertBrightness(color, backgroundBrightness);
            QString newColor = QString("rgb(%1,%2,%3)")
                .arg(adjusted.red())
                .arg(adjusted.green())
                .arg(adjusted.blue());

            rgbReplacements.append({match.captured(0), newColor});
        }
    }

    for (const auto& pair : rgbReplacements) {
        svg.replace(pair.first, pair.second);
    }

    return svg.toUtf8();
}

QByteArray IconUtils::processSvgForDarkTheme(const QByteArray& svgContent)
{
    // Dark theme background brightness is around 30-50
    return processSvgForBackground(svgContent, 45);
}

QString IconUtils::lightenColor(const QString& hexColor, int amount)
{
    QColor color = parseHexColor(hexColor);
    if (!color.isValid()) {
        return hexColor;
    }

    int r = qMin(255, color.red() + amount);
    int g = qMin(255, color.green() + amount);
    int b = qMin(255, color.blue() + amount);

    return QString("#%1%2%3")
        .arg(r, 2, 16, QChar('0'))
        .arg(g, 2, 16, QChar('0'))
        .arg(b, 2, 16, QChar('0'));
}

} // namespace XXMLStudio
