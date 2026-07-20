#ifndef NORMALIZATIONPLANNER_H
#define NORMALIZATIONPLANNER_H

#include <QString>
#include <QStringList>

class NormalizationPlanner
{
public:
    static QStringList forms();
    static int formRank(const QString& form);
    static QString nextForm(const QString& activeForm);
    static QString previousForm(const QString& activeForm);
    static QStringList inputTables(
        const QString& targetForm,
        const QString& activeForm,
        const QStringList& originalTables,
        const QStringList& activeTables);
    static int insertionIndex(
        const QString& targetForm,
        const QStringList& existingForms);
};

#endif
