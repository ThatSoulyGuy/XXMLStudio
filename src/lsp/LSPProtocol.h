#ifndef LSPPROTOCOL_H
#define LSPPROTOCOL_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <optional>

namespace XXMLStudio {

// Position in a text document (0-indexed)
struct LSPPosition {
    int line = 0;
    int character = 0;

    QJsonObject toJson() const {
        return {{"line", line}, {"character", character}};
    }

    static LSPPosition fromJson(const QJsonObject& j) {
        return {j["line"].toInt(), j["character"].toInt()};
    }
};

// Range in a text document
struct LSPRange {
    LSPPosition start;
    LSPPosition end;

    QJsonObject toJson() const {
        return {{"start", start.toJson()}, {"end", end.toJson()}};
    }

    static LSPRange fromJson(const QJsonObject& j) {
        return {
            LSPPosition::fromJson(j["start"].toObject()),
            LSPPosition::fromJson(j["end"].toObject())
        };
    }
};

// Location represents a location in a document
struct LSPLocation {
    QString uri;
    LSPRange range;

    static LSPLocation fromJson(const QJsonObject& j) {
        LSPLocation loc;
        loc.uri = j["uri"].toString();
        loc.range = LSPRange::fromJson(j["range"].toObject());
        return loc;
    }
};

// Diagnostic severity levels
enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

// Diagnostic message
struct LSPDiagnostic {
    LSPRange range;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    QString code;
    QString source;
    QString message;

    static LSPDiagnostic fromJson(const QJsonObject& j) {
        LSPDiagnostic diag;
        diag.range = LSPRange::fromJson(j["range"].toObject());
        diag.severity = static_cast<DiagnosticSeverity>(j["severity"].toInt(1));
        diag.code = j["code"].toString();
        diag.source = j["source"].toString();
        diag.message = j["message"].toString();
        return diag;
    }
};

// Completion item kinds
enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Struct = 22,
    Event = 23
};

// Completion item
struct LSPCompletionItem {
    QString label;
    CompletionItemKind kind = CompletionItemKind::Text;
    QString detail;
    QString documentation;
    QString insertText;

    static LSPCompletionItem fromJson(const QJsonObject& j) {
        LSPCompletionItem item;
        item.label = j["label"].toString();
        item.kind = static_cast<CompletionItemKind>(j["kind"].toInt(1));
        item.detail = j["detail"].toString();
        if (j["documentation"].isString()) {
            item.documentation = j["documentation"].toString();
        } else if (j["documentation"].isObject()) {
            item.documentation = j["documentation"].toObject()["value"].toString();
        }
        item.insertText = j["insertText"].toString();
        if (item.insertText.isEmpty()) {
            item.insertText = item.label;
        }
        return item;
    }
};

// Hover content
struct LSPHover {
    QString contents;  // Markdown content
    std::optional<LSPRange> range;

    static LSPHover fromJson(const QJsonObject& j) {
        LSPHover hover;
        if (j["contents"].isString()) {
            hover.contents = j["contents"].toString();
        } else if (j["contents"].isObject()) {
            hover.contents = j["contents"].toObject()["value"].toString();
        } else if (j["contents"].isArray()) {
            QJsonArray arr = j["contents"].toArray();
            for (const auto& item : arr) {
                if (item.isString()) {
                    hover.contents += item.toString() + "\n";
                } else if (item.isObject()) {
                    hover.contents += item.toObject()["value"].toString() + "\n";
                }
            }
        }
        if (j.contains("range")) {
            hover.range = LSPRange::fromJson(j["range"].toObject());
        }
        return hover;
    }
};

// Symbol kinds
enum class LSPSymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    Struct = 23,
    Event = 24
};

// Document symbol
struct LSPDocumentSymbol {
    QString name;
    QString detail;
    LSPSymbolKind kind;
    LSPRange range;
    LSPRange selectionRange;
    QList<LSPDocumentSymbol> children;

    static LSPDocumentSymbol fromJson(const QJsonObject& j) {
        LSPDocumentSymbol sym;
        sym.name = j["name"].toString();
        sym.detail = j["detail"].toString();
        sym.kind = static_cast<LSPSymbolKind>(j["kind"].toInt(1));
        sym.range = LSPRange::fromJson(j["range"].toObject());
        sym.selectionRange = LSPRange::fromJson(j["selectionRange"].toObject());

        if (j.contains("children")) {
            QJsonArray children = j["children"].toArray();
            for (const auto& child : children) {
                sym.children.append(fromJson(child.toObject()));
            }
        }
        return sym;
    }
};

} // namespace XXMLStudio

#endif // LSPPROTOCOL_H
