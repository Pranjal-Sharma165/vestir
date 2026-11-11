#ifndef EVENTFORMALITY_H
#define EVENTFORMALITY_H

#include "../model/OutfitFormality.h"

#include <QString>

/*!
 * The formality an event calls for: the dress code wins; otherwise the event
 * type the ML chose (school, work, party, hangout, gym). Either may be empty.
 */
inline OutfitFormality::Value formalityForEvent(const QString &dressCode,
                                                const QString &eventType) {
  using Value = OutfitFormality::Value;
  if (dressCode == "smart-casual")
    return Value::SmartCasual;
  if (dressCode == "formal")
    return Value::Formal;
  if (dressCode == "sporty")
    return Value::Sporty;
  if (dressCode == "casual")
    return Value::Casual;
  if (eventType == "work")
    return Value::SmartCasual;
  if (eventType == "party")
    return Value::Formal;
  if (eventType == "gym")
    return Value::Sporty;
  return Value::Casual;
}

#endif // EVENTFORMALITY_H
