/* -*- mode: c++; c-basic-offset:4 -*-
    models/keylistmodel.h

    This file is part of libkleopatra, the KDE keymanagement library
    SPDX-FileCopyrightText: 2007 Klarälvdalens Datakonsult AB
    SPDX-FileCopyrightText: 2021 g10 Code GmbH
    SPDX-FileContributor: Ingo Klöcker <dev@ingo-kloecker.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __KLEOPATRA_MODELS_KEYLISTMODEL_H__
#define __KLEOPATRA_MODELS_KEYLISTMODEL_H__

#include <QAbstractItemModel>

#include <kleo_export.h>

#include "keylistmodelinterface.h"

#include <vector>

namespace GpgME
{
class Key;
}

namespace Kleo
{

class KLEO_EXPORT AbstractKeyListModel : public QAbstractItemModel
                                       , public KeyListModelInterface
{
    Q_OBJECT
public:
    enum ItemType {
        Keys = 0x01,
        Groups = 0x02,
        All = Keys | Groups
    };
    Q_DECLARE_FLAGS(ItemTypes, ItemType)

    explicit AbstractKeyListModel(QObject *parent = nullptr);
    ~AbstractKeyListModel() override;

    static AbstractKeyListModel *createFlatKeyListModel(QObject *parent = nullptr);
    static AbstractKeyListModel *createHierarchicalKeyListModel(QObject *parent = nullptr);

    GpgME::Key key(const QModelIndex &idx) const override;
    std::vector<GpgME::Key> keys(const QList<QModelIndex> &indexes) const override;

    KeyGroup group(const QModelIndex &idx) const override;

    using QAbstractItemModel::index;
    QModelIndex index(const GpgME::Key &key) const override;
    QModelIndex index(const GpgME::Key &key, int col) const;
    QList<QModelIndex> indexes(const std::vector<GpgME::Key> &keys) const override;

    QModelIndex index(const KeyGroup &group) const override;
    QModelIndex index(const KeyGroup &group, int col) const;

Q_SIGNALS:
    void rowAboutToBeMoved(const QModelIndex &old_parent, int old_row);
    void rowMoved(const QModelIndex &new_parent, int new_row);

public Q_SLOTS:
    void setKeys(const std::vector<GpgME::Key> &keys);
    /* Set this to set all or only secret keys from the keycache. */
    void useKeyCache(bool value, bool secretOnly);
    QModelIndex addKey(const GpgME::Key &key);
    QList<QModelIndex> addKeys(const std::vector<GpgME::Key> &keys);
    void removeKey(const GpgME::Key &key);

    void clear(ItemTypes types = All);

public:
    int columnCount(const QModelIndex &pidx) const override;
    QVariant headerData(int section, Qt::Orientation o, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * defines which information is displayed in tooltips
     * see Kleo::Formatting::ToolTipOption
     */
    int toolTipOptions() const;

    void setToolTipOptions(int opts);

    /**
     * Set the keys to use for KeyListModelInterface::Remark column
     * to obtain remarks from this keys signature notations.
     * Needs at least GpgME 1.14 to work properly. Remarks are
     * joined by a semicolon and a space. */
    void setRemarkKeys(const std::vector<GpgME::Key> &remarkKeys);
    std::vector<GpgME::Key> remarkKeys() const;

private:
    virtual GpgME::Key doMapToKey(const QModelIndex &index) const = 0;
    virtual QModelIndex doMapFromKey(const GpgME::Key &key, int column) const = 0;
    virtual QList<QModelIndex> doAddKeys(const std::vector<GpgME::Key> &keys) = 0;
    virtual void doRemoveKey(const GpgME::Key &key) = 0;

    virtual KeyGroup doMapToGroup(const QModelIndex &index) const = 0;
    virtual QModelIndex doMapFromGroup(const KeyGroup &group, int column) const = 0;

    virtual void doClear(ItemTypes types) = 0;

private:
    class Private;
    QScopedPointer<Private> const d;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Kleo::AbstractKeyListModel::ItemTypes)

#endif /* __KLEOPATRA_MODELS_KEYLISTMODEL_H__ */
