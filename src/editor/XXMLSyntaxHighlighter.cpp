#include "XXMLSyntaxHighlighter.h"

namespace XXMLStudio {

XXMLSyntaxHighlighter::XXMLSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    applyTheme();
    setupRules();
}

void XXMLSyntaxHighlighter::setTheme(SyntaxTheme theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        applyTheme();
        rehighlight();
    }
}

const QTextCharFormat& XXMLSyntaxHighlighter::formatFor(FormatType type) const
{
    switch (type) {
    case FormatType::Keyword:       return m_keywordFormat;
    case FormatType::Type:          return m_typeFormat;
    case FormatType::AngleBracketId: return m_angleBracketIdFormat;
    case FormatType::String:        return m_stringFormat;
    case FormatType::Comment:       return m_commentFormat;
    case FormatType::Number:        return m_numberFormat;
    case FormatType::Operator:      return m_operatorFormat;
    case FormatType::Bracket:       return m_bracketFormat;
    case FormatType::Ownership:     return m_ownershipFormat;
    case FormatType::Import:        return m_importFormat;
    case FormatType::TemplateInst:  return m_templateInstFormat;
    default:                        return m_keywordFormat;
    }
}

void XXMLSyntaxHighlighter::applyTheme()
{
    switch (m_theme) {
    case SyntaxTheme::Darcula:
        // IntelliJ IDEA Darcula theme - refined colors, minimal bold
        m_keywordFormat.setForeground(QColor("#CC7832"));  // Orange - control flow
        m_keywordFormat.setFontWeight(QFont::Normal);

        m_typeFormat.setForeground(QColor("#6897BB"));     // Blue - types stand out
        m_typeFormat.setFontWeight(QFont::Normal);

        m_angleBracketIdFormat.setForeground(QColor("#FFC66D"));  // Yellow - identifiers
        m_angleBracketIdFormat.setFontWeight(QFont::Normal);

        m_stringFormat.setForeground(QColor("#6A8759"));   // Green - strings
        m_stringFormat.setFontWeight(QFont::Normal);

        m_commentFormat.setForeground(QColor("#808080"));  // Gray - comments
        m_commentFormat.setFontItalic(true);
        m_commentFormat.setFontWeight(QFont::Normal);

        m_numberFormat.setForeground(QColor("#6897BB"));   // Blue - numbers
        m_numberFormat.setFontWeight(QFont::Normal);

        m_operatorFormat.setForeground(QColor("#CC7832")); // Orange - operators
        m_operatorFormat.setFontWeight(QFont::Normal);

        m_bracketFormat.setForeground(QColor("#A9B7C6"));  // Gray-blue - brackets
        m_bracketFormat.setFontWeight(QFont::Normal);

        m_ownershipFormat.setForeground(QColor("#9876AA")); // Purple - ownership
        m_ownershipFormat.setFontWeight(QFont::Normal);

        m_importFormat.setForeground(QColor("#BBB529"));    // Yellow-green - imports
        m_importFormat.setFontWeight(QFont::Normal);

        m_templateInstFormat.setForeground(QColor("#A9B7C6")); // Gray-blue - templates
        m_templateInstFormat.setFontWeight(QFont::Normal);
        break;

    case SyntaxTheme::QtCreator:
        // Qt Creator Dark theme - vibrant, no bold
        m_keywordFormat.setForeground(QColor("#FFCB6B"));  // Gold - keywords
        m_keywordFormat.setFontWeight(QFont::Normal);

        m_typeFormat.setForeground(QColor("#82AAFF"));     // Light blue - types
        m_typeFormat.setFontWeight(QFont::Normal);

        m_angleBracketIdFormat.setForeground(QColor("#F78C6C"));  // Orange - identifiers
        m_angleBracketIdFormat.setFontWeight(QFont::Normal);

        m_stringFormat.setForeground(QColor("#C3E88D"));   // Light green - strings
        m_stringFormat.setFontWeight(QFont::Normal);

        m_commentFormat.setForeground(QColor("#546E7A"));  // Gray-blue - comments
        m_commentFormat.setFontItalic(true);
        m_commentFormat.setFontWeight(QFont::Normal);

        m_numberFormat.setForeground(QColor("#F78C6C"));   // Orange - numbers
        m_numberFormat.setFontWeight(QFont::Normal);

        m_operatorFormat.setForeground(QColor("#89DDFF")); // Cyan - operators
        m_operatorFormat.setFontWeight(QFont::Normal);

        m_bracketFormat.setForeground(QColor("#89DDFF"));  // Cyan - brackets
        m_bracketFormat.setFontWeight(QFont::Normal);

        m_ownershipFormat.setForeground(QColor("#C792EA")); // Purple - ownership
        m_ownershipFormat.setFontWeight(QFont::Normal);

        m_importFormat.setForeground(QColor("#82AAFF"));    // Light blue - imports
        m_importFormat.setFontWeight(QFont::Normal);

        m_templateInstFormat.setForeground(QColor("#FFCB6B")); // Gold - templates
        m_templateInstFormat.setFontWeight(QFont::Normal);
        break;

    case SyntaxTheme::VSCodeDark:
    default:
        // Visual Studio Code Dark+ theme - professional, no bold
        m_keywordFormat.setForeground(QColor("#C586C0"));  // Purple - keywords
        m_keywordFormat.setFontWeight(QFont::Normal);

        m_typeFormat.setForeground(QColor("#4EC9B0"));     // Teal - types
        m_typeFormat.setFontWeight(QFont::Normal);

        m_angleBracketIdFormat.setForeground(QColor("#DCDCAA"));  // Yellow - identifiers
        m_angleBracketIdFormat.setFontWeight(QFont::Normal);

        m_stringFormat.setForeground(QColor("#CE9178"));   // Orange-brown - strings
        m_stringFormat.setFontWeight(QFont::Normal);

        m_commentFormat.setForeground(QColor("#6A9955"));  // Green - comments
        m_commentFormat.setFontItalic(true);
        m_commentFormat.setFontWeight(QFont::Normal);

        m_numberFormat.setForeground(QColor("#B5CEA8"));   // Light green - numbers
        m_numberFormat.setFontWeight(QFont::Normal);

        m_operatorFormat.setForeground(QColor("#D4D4D4")); // Light gray - operators
        m_operatorFormat.setFontWeight(QFont::Normal);

        m_bracketFormat.setForeground(QColor("#FFD700"));  // Gold - brackets
        m_bracketFormat.setFontWeight(QFont::Normal);

        m_ownershipFormat.setForeground(QColor("#569CD6")); // Blue - ownership
        m_ownershipFormat.setFontWeight(QFont::Normal);

        m_importFormat.setForeground(QColor("#9CDCFE"));    // Cyan - imports
        m_importFormat.setFontWeight(QFont::Normal);

        m_templateInstFormat.setForeground(QColor("#D7BA7D")); // Tan - templates
        m_templateInstFormat.setFontWeight(QFont::Normal);
        break;
    }
}

void XXMLSyntaxHighlighter::setupRules()
{
    HighlightingRule rule;

    // XXML Keywords (from TokenType.h)
    QStringList keywordPatterns;
    keywordPatterns
        // Namespace and class declarations
        << "\\bNamespace\\b" << "\\bClass\\b" << "\\bStructure\\b"
        << "\\bFinal\\b" << "\\bExtends\\b" << "\\bNone\\b"
        // Access modifiers
        << "\\bPublic\\b" << "\\bPrivate\\b" << "\\bProtected\\b" << "\\bStatic\\b"
        // Properties and types
        << "\\bProperty\\b" << "\\bTypes\\b" << "\\bNativeType\\b" << "\\bNativeStructure\\b"
        // Constructors and methods
        << "\\bConstructor\\b" << "\\bDestructor\\b" << "\\bdefault\\b"
        << "\\bMethod\\b" << "\\bReturns\\b" << "\\bParameters\\b" << "\\bParameter\\b"
        // Entry point and execution
        << "\\bEntrypoint\\b" << "\\bInstantiate\\b" << "\\bLet\\b" << "\\bAs\\b" << "\\bRun\\b"
        // Control flow
        << "\\bFor\\b" << "\\bWhile\\b" << "\\bIf\\b" << "\\bElse\\b"
        << "\\bExit\\b" << "\\bReturn\\b" << "\\bBreak\\b" << "\\bContinue\\b"
        // Constraints and templates
        << "\\bConstrains\\b" << "\\bConstraint\\b" << "\\bRequire\\b"
        << "\\bTruth\\b" << "\\bTypeOf\\b" << "\\bOn\\b"
        << "\\bTemplates\\b" << "\\bCompiletime\\b"
        // Do, this, Set
        << "\\bDo\\b" << "\\bthis\\b" << "\\bSet\\b"
        // Lambda
        << "\\bLambda\\b"
        // Annotations
        << "\\bAnnotation\\b" << "\\bAnnotate\\b" << "\\bAllows\\b"
        << "\\bProcessor\\b" << "\\bRetain\\b" << "\\bAnnotationAllow\\b"
        // Memory alignment and callbacks
        << "\\bAligns\\b" << "\\bCallbackType\\b" << "\\bConvention\\b"
        // Enumerations
        << "\\bEnumeration\\b" << "\\bValue\\b";

    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.formatType = FormatType::Keyword;
        m_rules.append(rule);
    }

    // Boolean literals
    rule.pattern = QRegularExpression("\\b(true|false)\\b");
    rule.formatType = FormatType::Keyword;
    m_rules.append(rule);

    // Import directive: #import
    rule.pattern = QRegularExpression("#import\\b");
    rule.formatType = FormatType::Import;
    m_rules.append(rule);

    // Angle bracket identifiers with qualified names: <name> or <Namespace::Class::Member>
    rule.pattern = QRegularExpression("<[a-zA-Z_][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*>");
    rule.formatType = FormatType::AngleBracketId;
    m_rules.append(rule);

    // Function type pattern: F(ReturnType)(ParamTypes)
    rule.pattern = QRegularExpression("\\bF\\s*\\([^)]*\\)\\s*\\([^)]*\\)");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Template instantiation with @: Type@T or Type@T1@T2
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*(?:@[A-Z][a-zA-Z0-9_]*)+");
    rule.formatType = FormatType::TemplateInst;
    m_rules.append(rule);

    // Type names with template parameters: Type<T>
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*<[^>]+>");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Qualified names (Namespace::Class::Member) - types start with capital
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*(?:::[A-Z][a-zA-Z0-9_]*)+");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Ownership markers: ^ (owned), & (reference), % (copy) after type names
    rule.pattern = QRegularExpression("[\\^&%]");
    rule.formatType = FormatType::Ownership;
    m_rules.append(rule);

    // Square brackets for declaration blocks
    rule.pattern = QRegularExpression("[\\[\\]]");
    rule.formatType = FormatType::Bracket;
    m_rules.append(rule);

    // Arrow operator: ->
    rule.pattern = QRegularExpression("->");
    rule.formatType = FormatType::Operator;
    m_rules.append(rule);

    // Range operator: ..
    rule.pattern = QRegularExpression("\\.\\.");
    rule.formatType = FormatType::Operator;
    m_rules.append(rule);

    // Scope resolution operator: ::
    rule.pattern = QRegularExpression("::");
    rule.formatType = FormatType::Operator;
    m_rules.append(rule);

    // Numbers (integers and floats)
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?[fFdDlLuU]*\\b");
    rule.formatType = FormatType::Number;
    m_rules.append(rule);

    // Hexadecimal numbers
    rule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+[uUlL]*\\b");
    rule.formatType = FormatType::Number;
    m_rules.append(rule);

    // Binary numbers
    rule.pattern = QRegularExpression("\\b0[bB][01]+[uUlL]*\\b");
    rule.formatType = FormatType::Number;
    m_rules.append(rule);

    // String literals
    rule.pattern = QRegularExpression("\"(?:[^\"\\\\]|\\\\.)*\"");
    rule.formatType = FormatType::String;
    m_rules.append(rule);

    // Character literals
    rule.pattern = QRegularExpression("'(?:[^'\\\\]|\\\\.)'");
    rule.formatType = FormatType::String;
    m_rules.append(rule);

    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.formatType = FormatType::Comment;
    m_rules.append(rule);

    // Multi-line comment markers
    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void XXMLSyntaxHighlighter::highlightBlock(const QString& text)
{
    // Apply regular highlighting rules
    for (const HighlightingRule& rule : m_rules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), formatFor(rule.formatType));
        }
    }

    // Handle multi-line comments
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        QRegularExpressionMatch startMatch = m_commentStartExpression.match(text);
        startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = m_commentEndExpression.match(text, startIndex);
        int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
        int commentLength;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, m_commentFormat);

        QRegularExpressionMatch nextStartMatch = m_commentStartExpression.match(text, startIndex + commentLength);
        startIndex = nextStartMatch.hasMatch() ? nextStartMatch.capturedStart() : -1;
    }
}

} // namespace XXMLStudio
