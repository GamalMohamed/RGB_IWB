#ifndef UBFOREIGHNOBJECTSHANDLER_H
#define UBFOREIGHNOBJECTSHANDLER_H

#include <QList>
#include <QUrl>
#include <algorithm>

class UBForeighnObjectsHandlerPrivate;

class UBForeighnObjectsHandler
{
public:
    UBForeighnObjectsHandler();
    ~UBForeighnObjectsHandler();

    void cure(const QList<QUrl> &dirs);
    void cure(const QUrl &dir);

    void copyPage(const QUrl &fromDir, int fromIndex,
                  const QUrl &toDir, int toIndex);

private:
    UBForeighnObjectsHandlerPrivate *d;

    friend class UBForeighnObjectsHandlerPrivate;
};

#endif // UBFOREIGHNOBJECTSHANDLER_H
