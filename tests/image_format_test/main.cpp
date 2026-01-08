#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QImage>
#include "../../YACReader/image_decoders.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QFile avifFile(":/sample.avif");
    if (!avifFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open sample.avif";
        return 1;
    }
    QByteArray avifData = avifFile.readAll();
    QImage avifImage = decodeAvif(avifData);
    if (avifImage.isNull()) {
        qCritical() << "Failed to decode sample.avif";
        return 1;
    }

    QFile jxlFile(":/sample.jxl");
    if (!jxlFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open sample.jxl";
        return 1;
    }
    QByteArray jxlData = jxlFile.readAll();
    QImage jxlImage = decodeJxl(jxlData);
    if (jxlImage.isNull()) {
        qCritical() << "Failed to decode sample.jxl";
        return 1;
    }

    qDebug() << "Successfully decoded sample.avif and sample.jxl";
    return 0;
}
