TEMPLATE = app
CONFIG += console

SOURCES += \
    main.cpp \
    ../../YACReader/image_decoders.cpp

HEADERS += \
    ../../YACReader/image_decoders.h

QT += core

LIBS += -lavif -ljxl -ljxl_threads

RESOURCES += \
    test_images.qrc
