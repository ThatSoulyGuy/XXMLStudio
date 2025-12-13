#ifndef XXMLSYNTAXHIGHLIGHTER_H
#define XXMLSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

namespace XXMLStudio {

/**
 * Available syntax highlighting themes.
 */
enum class SyntaxTheme {
    Darcula,      // IntelliJ IDEA dark theme
    QtCreator,    // Qt Creator dark theme
    VSCodeDark    // Visual Studio Code Dark+ theme
};

/**
 * Format types for syntax highlighting rules.
 */
enum class FormatType {
    Keyword,
    Type,
    AngleBracketId,
    String,
    Comment,
    Number,
    Operator,
    Bracket,
    Ownership,
    Import,
    TemplateInst,
    MethodCall,
    Identifier,
    Variable,
    ImportPath,
    This
};

/**
 * Syntax highlighter for the XXML programming language.
 * Highlights keywords, types, strings, comments, and angle bracket identifiers.
 * Supports multiple color themes.
 */
class XXMLSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit XXMLSyntaxHighlighter(QTextDocument* parent = nullptr);

    // Theme management
    void setTheme(SyntaxTheme theme);
    SyntaxTheme theme() const { return m_theme; }

protected:
    void highlightBlock(const QString& text) override;

private:
    void setupRules();
    void applyTheme();
    const QTextCharFormat& formatFor(FormatType type) const;

    struct HighlightingRule {
        QRegularExpression pattern;
        FormatType formatType;
    };
    QVector<HighlightingRule> m_rules;

    // Text formats - updated by applyTheme()
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_angleBracketIdFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_bracketFormat;
    QTextCharFormat m_ownershipFormat;
    QTextCharFormat m_importFormat;
    QTextCharFormat m_templateInstFormat;
    QTextCharFormat m_methodCallFormat;
    QTextCharFormat m_identifierFormat;
    QTextCharFormat m_variableFormat;
    QTextCharFormat m_importPathFormat;
    QTextCharFormat m_thisFormat;

    // Multi-line comment state
    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;

    // Current theme
    SyntaxTheme m_theme = SyntaxTheme::VSCodeDark;
};

} // namespace XXMLStudio

#endif // XXMLSYNTAXHIGHLIGHTER_H
