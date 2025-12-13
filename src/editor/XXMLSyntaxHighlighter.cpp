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
    case FormatType::MethodCall:    return m_methodCallFormat;
    case FormatType::Identifier:    return m_identifierFormat;
    case FormatType::Variable:      return m_variableFormat;
    case FormatType::ImportPath:    return m_importPathFormat;
    case FormatType::This:          return m_thisFormat;
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

        m_operatorFormat.setForeground(QColor("#A9B7C6")); // Light gray - operators (< > etc)
        m_operatorFormat.setFontWeight(QFont::Normal);

        m_bracketFormat.setForeground(QColor("#A9B7C6"));  // Gray-blue - brackets
        m_bracketFormat.setFontWeight(QFont::Normal);

        m_ownershipFormat.setForeground(QColor("#9876AA")); // Purple - ownership
        m_ownershipFormat.setFontWeight(QFont::Normal);

        m_importFormat.setForeground(QColor("#BBB529"));    // Yellow-green - imports
        m_importFormat.setFontWeight(QFont::Normal);

        m_templateInstFormat.setForeground(QColor("#A9B7C6")); // Gray-blue - templates
        m_templateInstFormat.setFontWeight(QFont::Normal);

        m_methodCallFormat.setForeground(QColor("#FFC66D"));  // Yellow - method calls
        m_methodCallFormat.setFontWeight(QFont::Normal);

        m_identifierFormat.setForeground(QColor("#D0D0D0"));  // Light gray - identifiers
        m_identifierFormat.setFontWeight(QFont::Normal);

        m_variableFormat.setForeground(QColor("#9CDCFE"));    // Light blue - variables
        m_variableFormat.setFontWeight(QFont::Normal);

        m_importPathFormat.setForeground(QColor("#E0E0E0"));  // Light gray - import paths
        m_importPathFormat.setFontWeight(QFont::Normal);

        m_thisFormat.setForeground(QColor("#268BD2"));        // Unique blue - this keyword
        m_thisFormat.setFontWeight(QFont::Normal);
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

        m_methodCallFormat.setForeground(QColor("#82AAFF"));  // Light blue - method calls
        m_methodCallFormat.setFontWeight(QFont::Normal);

        m_identifierFormat.setForeground(QColor("#EEFFFF"));  // White - identifiers
        m_identifierFormat.setFontWeight(QFont::Normal);

        m_variableFormat.setForeground(QColor("#89DDFF"));    // Cyan - variables
        m_variableFormat.setFontWeight(QFont::Normal);

        m_importPathFormat.setForeground(QColor("#FFFFFF"));  // White - import paths
        m_importPathFormat.setFontWeight(QFont::Normal);

        m_thisFormat.setForeground(QColor("#569CD6"));        // Unique blue - this keyword
        m_thisFormat.setFontWeight(QFont::Normal);
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

        m_methodCallFormat.setForeground(QColor("#DCDCAA"));  // Yellow - method calls
        m_methodCallFormat.setFontWeight(QFont::Normal);

        m_identifierFormat.setForeground(QColor("#E0E0E0"));  // Light gray - identifiers
        m_identifierFormat.setFontWeight(QFont::Normal);

        m_variableFormat.setForeground(QColor("#9CDCFE"));    // Light blue - variables
        m_variableFormat.setFontWeight(QFont::Normal);

        m_importPathFormat.setForeground(QColor("#FFFFFF"));  // White - import paths
        m_importPathFormat.setFontWeight(QFont::Normal);

        m_thisFormat.setForeground(QColor("#569CD6"));        // Unique blue - this keyword
        m_thisFormat.setFontWeight(QFont::Normal);
        break;
    }
}

void XXMLSyntaxHighlighter::setupRules()
{
    HighlightingRule rule;

    // Variables: lowercase identifiers (general local variables)
    // This is applied FIRST so that more specific rules (keywords, method calls) override it
    rule.pattern = QRegularExpression("\\b[a-z][a-zA-Z0-9_]*\\b");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // XXML Keywords (from TokenType.h)
    // Note: Constructor and Destructor are NOT included here - they are context-dependent
    // They are keywords only in declaration contexts like [ Constructor default ]
    // In expressions like String::Constructor, Constructor is a method name
    QStringList keywordPatterns;
    keywordPatterns
        // Namespace and class declarations
        << "\\bNamespace\\b" << "\\bClass\\b" << "\\bStructure\\b"
        << "\\bFinal\\b" << "\\bExtends\\b" << "\\bNone\\b"
        // Access modifiers
        << "\\bPublic\\b" << "\\bPrivate\\b" << "\\bProtected\\b" << "\\bStatic\\b"
        // Properties and types
        << "\\bProperty\\b" << "\\bTypes\\b" << "\\bNativeType\\b" << "\\bNativeStructure\\b"
        // Method declarations (not Constructor/Destructor - they're context-dependent)
        << "\\bdefault\\b"
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
        // Do, Set (this is handled separately with unique color)
        << "\\bDo\\b" << "\\bSet\\b"
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

    // Constructor and Destructor as keywords ONLY in declaration context
    // Match after [ with optional whitespace (simple lookbehind)
    rule.pattern = QRegularExpression("(?<=\\[\\s)(Constructor|Destructor)\\b");
    rule.formatType = FormatType::Keyword;
    m_rules.append(rule);

    // Also match after access modifiers in declaration context
    rule.pattern = QRegularExpression("(?<=(Public|Private|Protected)\\s)(Constructor|Destructor)\\b");
    rule.formatType = FormatType::Keyword;
    m_rules.append(rule);

    // Boolean literals
    rule.pattern = QRegularExpression("\\b(true|false)\\b");
    rule.formatType = FormatType::Keyword;
    m_rules.append(rule);

    // 'this' keyword - unique blue color
    rule.pattern = QRegularExpression("\\bthis\\b");
    rule.formatType = FormatType::This;
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

    // Standalone type names (capitalized identifiers) - only when followed by ownership or in type contexts
    // This helps distinguish types from general identifiers
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*(?=[\\^&%])");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Type after Types keyword
    rule.pattern = QRegularExpression("(?<=Types\\s{1,20})[A-Z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Type after Returns keyword
    rule.pattern = QRegularExpression("(?<=Returns\\s{1,20})[A-Z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Type after Extends keyword (but not None which is a keyword)
    rule.pattern = QRegularExpression("(?<=Extends\\s{1,20})[A-Z][a-zA-Z0-9_]*(?!one)");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Qualified namespace/type access: Namespace::Type or Type::StaticMember
    // The left part (before ::) should be highlighted as Type
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*(?=::)");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Static method/member after :: (including Constructor/Destructor as method names)
    // This captures the method name part after ::
    rule.pattern = QRegularExpression("(?<=::)[A-Z][a-zA-Z0-9_]*|(?<=::)[a-z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::MethodCall;
    m_rules.append(rule);

    // Instance method calls: identifier followed by ( - but only lowercase starting identifiers
    rule.pattern = QRegularExpression("\\b[a-z][a-zA-Z0-9_]*(?=\\s*\\()");
    rule.formatType = FormatType::MethodCall;
    m_rules.append(rule);

    // After Run keyword: if lowercase, it's a variable; if uppercase, it's a type
    // Run varName.method() - varName is a variable
    rule.pattern = QRegularExpression("(?<=Run\\s)[a-z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // Run TypeName::method() - TypeName is a type
    rule.pattern = QRegularExpression("(?<=Run\\s)[A-Z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Method calls after Do keyword: Do methodName
    rule.pattern = QRegularExpression("(?<=Do\\s)[a-zA-Z_][a-zA-Z0-9_]*");
    rule.formatType = FormatType::MethodCall;
    m_rules.append(rule);

    // Member access after . (dot operator) - general case for properties
    rule.pattern = QRegularExpression("(?<=\\.)[a-zA-Z_][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Identifier;
    m_rules.append(rule);

    // Method calls after . (dot operator) - overrides above when followed by (
    rule.pattern = QRegularExpression("(?<=\\.)[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()");
    rule.formatType = FormatType::MethodCall;
    m_rules.append(rule);

    // Member access after this. - should be variable colored (overrides Identifier)
    rule.pattern = QRegularExpression("(?<=this\\.)[a-zA-Z_][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // Method calls after this. - still gold (overrides Variable when followed by ()
    rule.pattern = QRegularExpression("(?<=this\\.)[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()");
    rule.formatType = FormatType::MethodCall;
    m_rules.append(rule);

    // Variable after For keyword: For varName In ...
    rule.pattern = QRegularExpression("(?<=For\\s)[a-z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // Variable after Let keyword: Let varName = ...
    rule.pattern = QRegularExpression("(?<=Let\\s)[a-z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // Variable after Set keyword: Set varName = ...
    rule.pattern = QRegularExpression("(?<=Set\\s)[a-z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // Type after As keyword: ... As TypeName
    rule.pattern = QRegularExpression("(?<=As\\s)[A-Z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Type after Instantiate keyword: Instantiate TypeName^ ...
    rule.pattern = QRegularExpression("(?<=Instantiate\\s)[A-Z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Variable after If/While/Else keywords (condition variables)
    rule.pattern = QRegularExpression("(?<=(If|While)\\s)[a-z][a-zA-Z0-9_]*");
    rule.formatType = FormatType::Variable;
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

    // Import path (what comes after #import) - applied late to override Type/MethodCall rules
    // Matches the path portion after #import (e.g., "Namespace::Type")
    rule.pattern = QRegularExpression("(?<=#import\\s)[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*");
    rule.formatType = FormatType::ImportPath;
    m_rules.append(rule);

    // Import directive: #import
    rule.pattern = QRegularExpression("#import\\b");
    rule.formatType = FormatType::Import;
    m_rules.append(rule);

    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.formatType = FormatType::Comment;
    m_rules.append(rule);

    // Angle brackets < > should ALWAYS be white - applied last to override other rules
    rule.pattern = QRegularExpression("[<>]");
    rule.formatType = FormatType::Operator;
    m_rules.append(rule);

    // Variable names inside angle brackets: <varName> - applied after < > rule
    rule.pattern = QRegularExpression("(?<=<)[a-z][a-zA-Z0-9_]*(?=>)");
    rule.formatType = FormatType::Variable;
    m_rules.append(rule);

    // Type names inside angle brackets: <TypeName>
    rule.pattern = QRegularExpression("(?<=<)[A-Z][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*(?=>)");
    rule.formatType = FormatType::Type;
    m_rules.append(rule);

    // Method names after Method keyword: Method <methodName>
    rule.pattern = QRegularExpression("(?<=Method\\s<)[a-zA-Z_][a-zA-Z0-9_]*(?=>)");
    rule.formatType = FormatType::MethodCall;
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
