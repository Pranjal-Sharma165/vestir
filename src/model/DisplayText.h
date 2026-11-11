#ifndef DISPLAYTEXT_H
#define DISPLAYTEXT_H

#include <QRegularExpression>
#include <QString>
#include <string_view>

namespace DisplayText {
/*! "NavyBlue" -> "Navy Blue", "TShirt" -> "T-shirt": for labels shown to the user. */
inline QString spaced(std::string_view name) {
  QString text = QString::fromUtf8(name.data(), qsizetype(name.size()));
  if (text == "TShirt")
    return QStringLiteral("T-shirt");
  static const QRegularExpression camel("([a-z])([A-Z])");
  return text.replace(camel, "\\1 \\2");
}
} // namespace DisplayText

#endif // DISPLAYTEXT_H
