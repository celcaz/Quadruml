#include "Vdf.h"

#include <QList>

namespace Vdf {

namespace {

class Parser
{
public:
    explicit Parser(const QByteArray &text)
        : m_text(text)
    {
    }

    QVariantMap parseBlock(int depth = 0)
    {
        QVariantMap map;
        while (true) {
            skipSpaceAndComments();
            if (atEnd())
                return map;
            if (peek() == '}') {
                ++m_pos;
                return map;
            }
            const QString key = readToken();
            if (key.isNull())
                return map; // texto malformado: devolve o que já leu
            skipSpaceAndComments();
            if (!atEnd() && peek() == '{') {
                ++m_pos;
                if (depth < 32)
                    map.insert(key, parseBlock(depth + 1));
            } else {
                map.insert(key, readToken());
            }
        }
    }

private:
    bool atEnd() const { return m_pos >= m_text.size(); }
    char peek() const { return m_text.at(m_pos); }

    void skipSpaceAndComments()
    {
        while (!atEnd()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++m_pos;
            } else if (c == '/' && m_pos + 1 < m_text.size() && m_text.at(m_pos + 1) == '/') {
                while (!atEnd() && peek() != '\n')
                    ++m_pos;
            } else {
                return;
            }
        }
    }

    // Texto entre aspas (com \\, \", \n, \t) ou palavra sem aspas.
    QString readToken()
    {
        if (atEnd())
            return {};
        QByteArray out;
        if (peek() == '"') {
            ++m_pos;
            while (!atEnd() && peek() != '"') {
                char c = peek();
                if (c == '\\' && m_pos + 1 < m_text.size()) {
                    ++m_pos;
                    c = peek();
                    if (c == 'n')
                        c = '\n';
                    else if (c == 't')
                        c = '\t';
                }
                out.append(c);
                ++m_pos;
            }
            ++m_pos; // aspas de fechamento
            return QString::fromUtf8(out);
        }
        while (!atEnd()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '{' || c == '}' || c == '"')
                break;
            out.append(c);
            ++m_pos;
        }
        return out.isEmpty() ? QString() : QString::fromUtf8(out);
    }

    const QByteArray &m_text;
    qsizetype m_pos = 0;
};

}

QVariantMap parse(const QByteArray &text)
{
    return Parser(text).parseBlock();
}

}
