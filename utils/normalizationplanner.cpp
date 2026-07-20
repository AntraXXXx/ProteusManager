#include "normalizationplanner.h"

QStringList NormalizationPlanner::forms()
{
    return {
        "1NF",
        "2NF",
        "3NF",
        "BCNF",
        "4NF",
        "5NF"
    };
}

int NormalizationPlanner::formRank(const QString& form)
{
    const QStringList knownForms = forms();
    for (int index = 0; index < knownForms.size(); ++index)
    {
        if (knownForms.at(index).compare(
                form,
                Qt::CaseInsensitive) == 0)
        {
            return index;
        }
    }

    return -1;
}

QString NormalizationPlanner::nextForm(
    const QString& activeForm)
{
    const int nextRank = formRank(activeForm) + 1;
    const QStringList knownForms = forms();

    return nextRank >= 0 && nextRank < knownForms.size()
               ? knownForms.at(nextRank)
               : QString();
}

QString NormalizationPlanner::previousForm(
    const QString& activeForm)
{
    const int previousRank = formRank(activeForm) - 1;
    const QStringList knownForms = forms();

    return previousRank >= 0 && previousRank < knownForms.size()
               ? knownForms.at(previousRank)
               : QString();
}

QStringList NormalizationPlanner::inputTables(
    const QString& targetForm,
    const QString& activeForm,
    const QStringList& originalTables,
    const QStringList& activeTables)
{
    const int targetRank = formRank(targetForm);
    const int activeRank = formRank(activeForm);

    if (activeRank >= 0
        && targetRank > activeRank
        && !activeTables.isEmpty())
    {
        return activeTables;
    }

    return originalTables;
}

int NormalizationPlanner::insertionIndex(
    const QString& targetForm,
    const QStringList& existingForms)
{
    const int targetRank = formRank(targetForm);

    for (int index = 0; index < existingForms.size(); ++index)
    {
        if (formRank(existingForms.at(index)) > targetRank)
            return index;
    }

    return existingForms.size();
}
