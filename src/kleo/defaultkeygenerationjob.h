/*  This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2016 Klarälvdalens Datakonsult AB

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef LIBKLEO_DEFAULTKEYGENERATION_H
#define LIBKLEO_DEFAULTKEYGENERATION_H

#include "job.h"

#include <kleo_export.h>

namespace GpgME {
class KeyGenerationResult;
}

namespace Kleo {

/**
 * Generates a PGP RSA/2048 bit key pair for given name and email address.
 *
 * @since 5.4
 */
class KLEO_EXPORT DefaultKeyGenerationJob : public Job
{
    Q_OBJECT
public:
    explicit DefaultKeyGenerationJob(QObject *parent = Q_NULLPTR);
    ~DefaultKeyGenerationJob();

    GpgME::Error start(const QString &email, const QString &name);

    QString auditLogAsHtml() const Q_DECL_OVERRIDE;
    GpgME::Error auditLogError() const Q_DECL_OVERRIDE;


public Q_SLOTS:
    void slotCancel() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void result(const GpgME::KeyGenerationResult &result, const QByteArray &pubkeyData,
                const QString &auditLogAsHtml, const GpgME::Error &auditLogError);

protected:
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;

private:
    class Private;
    Private * const d;
};

}

#endif