#ifndef UBDOCUMENTMANAGER_H_
#define UBDOCUMENTMANAGER_H_

#include <QtCore>

class UBExportAdaptor;
class UBImportAdaptor;
class UBDocumentProxy;

class UBDocumentManager : public QObject
{
    Q_OBJECT

    public:
        static UBDocumentManager* documentManager();
        virtual ~UBDocumentManager();

        /**
          * List all supported files in a filter for the importation.
          *
          * \param notUbx If true, the ubx format isn't put in the list of supported file.
          *
          * \return Return the filter of supported files for the importation.
          */
        QString importFileFilter(bool notUbx = false);

        /**
          * List all supported files for the importation.
          *
          * \param notUbx If true, the ubx format isn't put in the list of supported file.
          *
          * \return Return the lsit of supported files for the importation.
          */
        QStringList importFileExtensions(bool notUbx = false);

        QFileInfoList importUbx(const QString &Incomingfile, const QString &destination);
        UBDocumentProxy* importFile(const QFile& pFile, const QString& pGroup);

        int addFilesToDocument(UBDocumentProxy* pDocument, QStringList fileNames);

        UBDocumentProxy* importDir(const QDir& pDir, const QString& pGroup);
        int addImageDirToDocument(const QDir& pDir, UBDocumentProxy* pDocument);

        QList<UBExportAdaptor*> supportedExportAdaptors();
        void emitDocumentUpdated(UBDocumentProxy* pDocument);

    signals:
        void documentUpdated(UBDocumentProxy *pDocument);

    private:
        UBDocumentManager(QObject *parent = 0);
        QList<UBExportAdaptor*> mExportAdaptors;
        QList<UBImportAdaptor*> mImportAdaptors;

        static UBDocumentManager* sDocumentManager;
};

#endif /* UBDOCUMENTMANAGER_H_ */
