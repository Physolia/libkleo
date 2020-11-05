/* -*- mode: c++; c-basic-offset:4 -*-
    models/useridlistmodel.cpp

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2007 Klarälvdalens Datakonsult AB
    SPDX-FileCopyrightText: 2016 Andre Heinecke <aheinecke@gnupg.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "useridlistmodel.h"
#include "utils/formatting.h"

#include <gpgme++/key.h>

#include <KLocalizedString>

#include <QVariant>
#include <QIcon>

#include "keycache.h"

#include <gpgme++/gpgmepp_version.h>
#if GPGMEPP_VERSION >= 0x10E00 // 1.14.0
# define GPGME_HAS_REMARKS
#endif
#if GPGMEPP_VERSION >= 0x10E01 // 1.14.1
# define GPGME_USERID_SIGNATURES_ARE_SORTABLE
#endif

using namespace GpgME;
using namespace Kleo;

class UIDModelItem
{
    // A uid model item can either be a UserID::Signature or a UserID.
    // you can find out which it is if the uid or the signature return
    // null values. (Not null but isNull)
    //
public:
    explicit UIDModelItem(const UserID::Signature &sig, UIDModelItem *parentItem, bool showRemarks)
    {
        mItemData  << QString::fromUtf8(sig.signerKeyID())
                   << Formatting::prettyName(sig)
                   << Formatting::prettyEMail(sig)
                   << Formatting::creationDateString(sig)
                   << Formatting::expirationDateString(sig)
                   << Formatting::validityShort(sig)
                   << (sig.isExportable() ? QStringLiteral("✓") : QString());
        mSig = sig;
        mParentItem = parentItem;

        if (showRemarks && parentItem) {
            QString lastNotation;
            for (const auto &notation: sig.notations()) {
                if (notation.name() && !strcmp(notation.name(), "rem@gnupg.org")) {
                    lastNotation = QString::fromUtf8(notation.value());
                }
            }
            mItemData << lastNotation;
        }
    }

    explicit UIDModelItem(const UserID &uid, UIDModelItem *parentItem)
    {
        mItemData << Formatting::prettyUserID(uid);
        mUid = uid;
        mParentItem = parentItem;
    }

    // The root item
    explicit UIDModelItem(bool showRemarks) : mParentItem(nullptr)
    {
        mItemData << i18n("ID")
                  << i18n("Name")
                  << i18n("E-Mail")
                  << i18n("Valid From")
                  << i18n("Valid Until")
                  << i18n("Status")
                  << i18n("Exportable");

        if (showRemarks) {
            mItemData << i18n("Tags");
        }
    }

    ~UIDModelItem()
    {
        qDeleteAll(mChildItems);
    }

    void appendChild(UIDModelItem *child)
    {
        mChildItems << child;
    }

    UIDModelItem *child(int row) const
    {
        return mChildItems.value(row);
    }

    const UIDModelItem *constChild(int row) const
    {
        return mChildItems.value(row);
    }

    int childCount() const
    {
        return mChildItems.count();
    }

    int columnCount() const
    {
        if (childCount()) {
            // We take the value from the first child
            // as we are likely a UID and our children
            // are UID Signatures.
            return constChild(0)->columnCount();
        }
        return mItemData.count();
    }

    QVariant data(int column) const
    {
        return mItemData.value(column);
    }

    QVariant toolTip(int column) const
    {
        if (column == 5 /*Status*/) {
            return i18n("class %1", mSig.certClass());
        }
        return mItemData.value(column);
    }

    QVariant icon(int column) const
    {
        if (!mSig.isNull() && column == 5 /*Status*/) {
            return Formatting::validityIcon(mSig);
        }
        return QVariant();
    }

    int row() const
    {
        if (mParentItem) {
            return mParentItem->mChildItems.indexOf(const_cast<UIDModelItem*>(this));
        }
        return 0;
    }

    UIDModelItem *parentItem() const
    {
        return mParentItem;
    }

    UserID::Signature signature() const
    {
        return mSig;
    }

    UserID uid() const
    {
        return mUid;
    }

private:
    QList<UIDModelItem*> mChildItems;
    QList<QVariant> mItemData;
    UIDModelItem *mParentItem = nullptr;
    UserID::Signature mSig;
    UserID mUid;
};

UserIDListModel::UserIDListModel(QObject *p)
    : QAbstractItemModel(p), mRootItem(nullptr)
{
}

UserIDListModel::~UserIDListModel()
{
    delete mRootItem;
}

Key UserIDListModel::key() const
{
    return mKey;
}

void UserIDListModel::setKey(const Key &key)
{
    beginResetModel();
    delete mRootItem;
    mKey = key;

    mRootItem = new UIDModelItem(mRemarksEnabled);
    for (int i = 0, ids = key.numUserIDs(); i < ids; ++i) {
        UserID uid = key.userID(i);
        auto *uidItem = new UIDModelItem(uid, mRootItem);
        mRootItem->appendChild(uidItem);
        std::vector<UserID::Signature> sigs = uid.signatures();
#ifdef GPGME_USERID_SIGNATURES_ARE_SORTABLE
        std::sort(sigs.begin(), sigs.end());
#endif
        for (const auto &sig : sigs) {
            auto *sigItem = new UIDModelItem(sig, uidItem, mRemarksEnabled);
            uidItem->appendChild(sigItem);
        }
    }

    endResetModel();
}

int UserIDListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return static_cast<UIDModelItem*>(parent.internalPointer())->columnCount();
    }

    if (!mRootItem) {
        return 0;
    }

    return mRootItem->columnCount();
}

int UserIDListModel::rowCount(const QModelIndex &parent) const
{
    UIDModelItem *parentItem = nullptr;
    if (parent.column() > 0 || !mRootItem) {
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = mRootItem;
    } else {
        parentItem = static_cast<UIDModelItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

QModelIndex UserIDListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    UIDModelItem *parentItem = nullptr;

    if (!parent.isValid()) {
        parentItem = mRootItem;
    } else {
        parentItem = static_cast<UIDModelItem*>(parent.internalPointer());
    }

    UIDModelItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex UserIDListModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }
    auto *childItem = static_cast<UIDModelItem*>(index.internalPointer());
    UIDModelItem *parentItem = childItem->parentItem();

    if (parentItem == mRootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

QVariant UserIDListModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (o == Qt::Horizontal && mRootItem) {
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
            return mRootItem->data(section);
        }
    }
    return QVariant();
}

QVariant UserIDListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole &&
        role != Qt::DecorationRole) {
        return QVariant();
    }

    auto *item = static_cast<UIDModelItem*>(index.internalPointer());

    if (role == Qt::ToolTipRole) {
        return item->toolTip(index.column());
    }

    if (role == Qt::DecorationRole) {
        return item->icon(index.column());
    }

    return item->data(index.column());
}

UserID UserIDListModel::userID(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return UserID();
    }
    UIDModelItem *item = static_cast<UIDModelItem*>(index.internalPointer());
    return item->uid();
}

QVector<UserID> UserIDListModel::userIDs(const QModelIndexList &indexs) const
{
    QVector<GpgME::UserID> ret;
    for (const QModelIndex &idx : indexs) {
        if (!idx.isValid()) {
            continue;
        }
        auto *item = static_cast<UIDModelItem*>(idx.internalPointer());
        if (!item->uid().isNull()) {
            ret << item->uid();
        }
    }
    return ret;
}

UserID::Signature UserIDListModel::signature(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return UserID::Signature();
    }
    UIDModelItem *item = static_cast<UIDModelItem*>(index.internalPointer());
    return item->signature();
}

QVector<UserID::Signature> UserIDListModel::signatures(const QModelIndexList &indexs) const
{
    QVector<GpgME::UserID::Signature> ret;
    for (const QModelIndex &idx : indexs) {
        if (!idx.isValid()) {
            continue;
        }
        auto *item = static_cast<UIDModelItem*>(idx.internalPointer());
        if (!item->signature().isNull()) {
            ret << item->signature();
        }
    }
    return ret;
}

void UserIDListModel::enableRemarks(bool value)
{
    mRemarksEnabled = value;
}
