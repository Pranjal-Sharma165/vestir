#include "ClothingItem.h"

#include <QObject>
#include <QQmlEngine>

ClothingItem::ClothingItem(QObject *parent)
    : m_name("Shirt 1"), m_category(ClothingCategory::Top) {}
