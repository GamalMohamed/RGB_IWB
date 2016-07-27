#ifndef UBDOCUMENTCONTAINER_H_
#define UBDOCUMENTCONTAINER_H_

#include <QtGui>
#include "UBDocumentProxy.h"

class UBDocumentContainer : public QObject
{
    Q_OBJECT

    public:
        UBDocumentContainer(QObject * parent = 0);
        virtual ~UBDocumentContainer();

        virtual void setDocument(UBDocumentProxy* document, bool forceReload = false);
        void pureSetDocument(UBDocumentProxy *document) {mCurrentDocument = document;}

        UBDocumentProxy* selectedDocument() {return mCurrentDocument;}
        int pageCount() {return mDocumentThumbs.size();}
        const QPixmap* pageAt(int index);

        static int pageFromSceneIndex(int sceneIndex);    
        static int sceneIndexFromPage(int sceneIndex); 

        void duplicatePages(QList<int>& pageIndexes);
        bool movePageToIndex(int source, int target);
        void deletePages(QList<int>& pageIndexes);
        void addPage(int index);
        void updatePage(int index);
        void reloadThumbnails();
        void addPixmapAt(const QPixmap *pix, int index);
        void insertThumbPage(int index);

    private:
        UBDocumentProxy* mCurrentDocument;
        QList<const QPixmap*> mDocumentThumbs;

    protected:
        void deleteThumbPage(int index);
        void updateThumbPage(int index);

    signals:
        void documentSet(UBDocumentProxy* document);
        void documentPageUpdated(int index);
        void documentThumbnailsUpdated(UBDocumentContainer* source);
};


#endif /* UBDOCUMENTPROXY_H_ */
